#include "terrain.h"
#include "terrainConfig.h"
#include "coal_miner_internal.h"
#include <FastNoiseLite.h>
#include "camera.h"
#include "coal_helper.h"

//region structures
#define TERRAIN_CHUNK_VOXEL_COUNT TERRAIN_CHUNK_SIZE * TERRAIN_CHUNK_SIZE * TERRAIN_CHUNK_SIZE
#define TERRAIN_CHUNK_COUNT TERRAIN_VIEW_RANGE * TERRAIN_VIEW_RANGE * TERRAIN_HEIGHT
#define TERRAIN_CHUNK_HORIZONTAL_SLICE TERRAIN_CHUNK_SIZE * TERRAIN_CHUNK_SIZE

const uint32_t BUFFER_SIZE_STAGES[] =
{
	0, 128, 512, TERRAIN_CHUNK_VOXEL_COUNT / 16,
	TERRAIN_CHUNK_VOXEL_COUNT / 8, TERRAIN_CHUNK_VOXEL_COUNT / 4,
	TERRAIN_CHUNK_VOXEL_COUNT / 2, TERRAIN_CHUNK_VOXEL_COUNT
};

typedef enum
{
	CHUNK_REQUIRES_FACES,
	CHUNK_CREATING_FACES,
	CHUNK_REQUIRES_UPLOAD,
	CHUNK_READY_TO_DRAW,
}ChunkState;

typedef enum
{
	CHUNK_GROUP_REQUIRES_NOISE_MAP,
	CHUNK_GROUP_GENERATING_NOISE_MAP,
	CHUNK_GROUP_READY,
}ChunkGroupState;

typedef struct
{
	uint32_t chunkId;
	ivec3 chunk;
} UniformData;

typedef struct
{
	ChunkState state;
	//1bit is uploaded
	//3bit buffer size
	//16bit faceCount
	uint32_t flags;
	uint32_t yId;

	uint32_t* buffer;
	uint8_t* voxels;
	uint64_t* fbOpaqueMask;
	uint64_t* rlOpaqueMask;
	uint64_t* tbOpaqueMask;
}TerrainChunk;

typedef struct
{
	TerrainChunk chunks[TERRAIN_HEIGHT];
	ChunkGroupState state;
	uint32_t id[2];
	uint32_t ssboId;
	uint8_t* heightMap;
	bool isAlive;
}TerrainChunkGroup;

typedef struct
{
	int u_chunkIndex;
	int u_surfaceTex;

	fnl_state caveNoise;
	fnl_state biomes[BIOME_COUNT];

	ThreadPool* pool;
	
	Shader shader;
	Texture textures[3];
	Ssbo voxelsSsbo;

	ivec2 loadedCenter;
	Vao chunkVaos[TERRAIN_CHUNK_COUNT];
	TerrainChunkGroup chunkGroups[TERRAIN_VIEW_RANGE * TERRAIN_VIEW_RANGE];
	TerrainChunkGroup* shiftGroups;
}VoxelTerrain;
//endregion

//region Local Functions
//General
static void LoadTerrainShader();
static void LoadTerrainTextures();
static void LoadBuffers();
static void InitTerrainNoise();
static void LoadTerrainChunks();

static TerrainChunkGroup InitializeChunkGroup(uint32_t ssboId);
static void RecreateChunkGroup(TerrainChunkGroup* group, uint32_t x, uint32_t z);
static void UnloadChunkGroup(TerrainChunkGroup* group);
static void DestroyChunkGroup(TerrainChunkGroup* group);

//Positional Loading
static void SetupInitialChunks(Camera3D camera);
static void ReloadChunks(Camera3D camera);

//Noise
static void SendNoiseJob(uint32_t x, uint32_t z);
static void T_GenerateNoise(void* args);
static void T_OnNoiseGenerationFinished(void* args);

static void GenerateHeightMap(const uint32_t sourceId[2], const uint32_t destination[2]);
static void GeneratePreChunk(uint32_t xId, uint32_t yId, uint32_t zId);
static void GeneratePostChunk(uint32_t xId, uint32_t yId, uint32_t zId);

//Face Creation
static void SendFaceCreationJob(uint32_t x, uint32_t y, uint32_t z);
static void T_CreateChunkFaces(void* args);
static void T_ChunkFacesCreationFinished(void* args);

static void SendGroupFaceCreationJob(uint32_t x, uint32_t z);
static void T_CreateGroupFaces(void* args);
static void T_GroupFacesCreationFinished(void* args);

static void CreateChunkFaces(uint32_t xId, uint32_t yId, uint32_t zId);

//Utils
static void PassTerrainDataToShader(UniformData* data);
static bool SurroundGroupsAreLoaded(uint32_t xId, uint32_t zId);
static bool GroupNeedsFaces(uint32_t xId, uint32_t zId);
static bool DelayedLoader();
static bool TryUploadGroup(TerrainChunkGroup* group);
static bool ChunkIsInFrustum();

//endregion

//region Variables

VoxelTerrain voxelTerrain = { 0 };
bool terrainIsWireMode;

//endregion

//region Callback Functions
void load_terrain()
{
	LoadTerrainShader();
	LoadTerrainTextures();
	InitTerrainNoise();
	
	voxelTerrain.shiftGroups = CM_MALLOC(TERRAIN_VIEW_RANGE * TERRAIN_VIEW_RANGE * sizeof(TerrainChunkGroup));
	
	for (int x = 0; x < TERRAIN_VIEW_RANGE; ++x)
		for (int z = 0; z < TERRAIN_VIEW_RANGE; ++z)
			voxelTerrain.chunkGroups[x * TERRAIN_VIEW_RANGE + z] = InitializeChunkGroup(x * TERRAIN_VIEW_RANGE + z);
	
	LoadBuffers();
	
	voxelTerrain.pool = cm_create_thread_pool(TERRAIN_NUM_WORKER_THREADS, 1024);

	SetupInitialChunks(get_camera());
}

bool loading_terrain()
{
	LoadTerrainChunks();
	
	for (uint32_t x = 0; x < TERRAIN_VIEW_RANGE; ++x)
		for (uint32_t z = 0; z < TERRAIN_VIEW_RANGE; ++z)
			TryUploadGroup(&voxelTerrain.chunkGroups[x * TERRAIN_VIEW_RANGE + z]);

#ifdef TERRAIN_DELAYED_LOAD
	return DelayedLoader();
#else
	return false;
#endif
}

void update_terrain()
{
	if(cm_is_key_pressed(KEY_B))
	{
		if(terrainIsWireMode)
		{
			cm_disable_wire_mode();
			cm_enable_backface_culling();
		}
		else
		{
			cm_disable_backface_culling();
			cm_enable_wire_mode();
		}

		terrainIsWireMode = !terrainIsWireMode;
	}
	
	ReloadChunks(get_camera());
	LoadTerrainChunks();
	
//	printf("FrameTime: %f\n", cm_frame_time() * 1000);
}

void draw_terrain()
{
	cm_begin_shader_mode(voxelTerrain.shader);
	
	for (int i = 0; i < 4; ++i)
		cm_set_texture(voxelTerrain.u_surfaceTex + i, voxelTerrain.textures[0].id, i);
	
	cm_set_texture(voxelTerrain.u_surfaceTex + 4, voxelTerrain.textures[1].id, 4);
	cm_set_texture(voxelTerrain.u_surfaceTex + 5, voxelTerrain.textures[2].id, 5);

	UniformData data = {0};
	int numUploadsLeft = TERRAIN_GROUP_UPLOAD_LIMIT;
	uint32_t drawCount = 0;

	for (uint32_t x = 0; x < TERRAIN_VIEW_RANGE; ++x)
	{
		for (uint32_t z = 0; z < TERRAIN_VIEW_RANGE; ++z)
		{
			TerrainChunkGroup* group = &voxelTerrain.chunkGroups[x * TERRAIN_VIEW_RANGE + z];
			if(numUploadsLeft > 0) numUploadsLeft -= TryUploadGroup(group);

			BoundingVolume volume =
			{
				.extents = { TERRAIN_CHUNK_SIZE * .5f, TERRAIN_CHUNK_SIZE * TERRAIN_HEIGHT * .5f, TERRAIN_CHUNK_SIZE * .5f }
			};
			vec3 chunkPos = { (float)group->id[0] - TERRAIN_WORLD_EDGE, 0, (float)group->id[1] - TERRAIN_WORLD_EDGE };
			glm_vec3_mul(chunkPos, (vec3){ TERRAIN_CHUNK_SIZE, 0, TERRAIN_CHUNK_SIZE }, chunkPos);
			glm_vec3_add(chunkPos, volume.extents, chunkPos);
			glm_vec3_copy(chunkPos, volume.center);

			if(!cm_is_in_main_frustum(&volume)) continue;

			volume.extents[1] = TERRAIN_CHUNK_SIZE * .5f;

			for (uint32_t y = 0; y < TERRAIN_HEIGHT; ++y)
			{
				TerrainChunk* chunk = &group->chunks[y];
				uint16_t faceCount = (chunk->flags & 0xffff0000) >> 16;

				if(faceCount > 0)
				{
					volume.center[1] = (float)y * TERRAIN_CHUNK_SIZE + TERRAIN_CHUNK_SIZE * .5f;
					if(!cm_is_in_main_frustum(&volume)) continue;

					data.chunk[0] = (int)group->id[0];
					data.chunk[1] = (int)y;
					data.chunk[2] = (int)group->id[1];

					data.chunkId = group->ssboId * TERRAIN_HEIGHT + y;

					if(chunk->flags & 1)
					{
						PassTerrainDataToShader(&data);
						cm_draw_vao(voxelTerrain.chunkVaos[data.chunkId], CM_TRIANGLES);
						drawCount++;
					}
				}
			}
		}
	}

	cm_end_shader_mode();

//	printf("Draw: %i\n", drawCount);
}

void dispose_terrain()
{
	cm_destroy_thread_pool(voxelTerrain.pool);
	voxelTerrain.pool = NULL;
	
	for (int x = 0; x < TERRAIN_VIEW_RANGE; ++x)
		for (int z = 0; z < TERRAIN_VIEW_RANGE; ++z)
			DestroyChunkGroup(&voxelTerrain.chunkGroups[x * TERRAIN_VIEW_RANGE + z]);
	
	CM_FREE(voxelTerrain.shiftGroups);
	
	for (int i = 0; i < 3; ++i) cm_unload_texture(voxelTerrain.textures[i]);
	
	for (int i = 0; i < TERRAIN_CHUNK_COUNT; ++i)
		cm_unload_vao(voxelTerrain.chunkVaos[i]);

	cm_unload_shader(voxelTerrain.shader);
}
//endregion

//region General

static void LoadTerrainShader()
{
	Path vsPath = TO_RES_PATH(vsPath, "shaders/voxel_terrain.vert");
	Path fsPath = TO_RES_PATH(fsPath, "shaders/voxel_terrain.frag");
	
	voxelTerrain.shader = cm_load_shader(vsPath, fsPath);
	voxelTerrain.u_chunkIndex = cm_get_uniform_location(voxelTerrain.shader, "u_chunkIndex");
	voxelTerrain.u_surfaceTex = cm_get_uniform_location(voxelTerrain.shader, "u_surfaceTex");
}

static void LoadTerrainTextures()
{
	Path mapPath0 = TO_RES_PATH(mapPath0, "2d/terrain/0.Map_Side.png");
	Path mapPath1 = TO_RES_PATH(mapPath1, "2d/terrain/1.Map_Top.png");
	Path mapPath2 = TO_RES_PATH(mapPath2, "2d/terrain/2.Map_Bottom.png");

	voxelTerrain.textures[0] = cm_load_texture(mapPath0, CM_TEXTURE_WRAP_CLAMP_TO_EDGE, CM_TEXTURE_FILTER_NEAREST, true);
	voxelTerrain.textures[1] = cm_load_texture(mapPath1, CM_TEXTURE_WRAP_CLAMP_TO_EDGE, CM_TEXTURE_FILTER_NEAREST, true);
	voxelTerrain.textures[2] = cm_load_texture(mapPath2, CM_TEXTURE_WRAP_CLAMP_TO_EDGE, CM_TEXTURE_FILTER_NEAREST, true);
}

static void LoadBuffers()
{
	for (int i = 0; i < TERRAIN_CHUNK_COUNT; ++i)
	{
		VaoAttribute attributes[] =
		{
			{ 1, CM_UINT, false, 1 * sizeof(unsigned int) },
		};
		
		Vbo vbo = { 0 };
		vbo.id = 0;
		vbo.data = NULL;
		vbo.vertexCount = 0;
		vbo.dataSize = 0;
		vbo.ebo = (Ebo){0};
		
		voxelTerrain.chunkVaos[i] = cm_load_vao(attributes, 1, vbo);
	}

	voxelTerrain.voxelsSsbo = cm_load_ssbo(TERRAIN_VOXELS_SSBO_BINDING,
	                                       TERRAIN_CHUNK_VOXEL_COUNT * TERRAIN_CHUNK_COUNT, NULL);
}

static void InitTerrainNoise()
{
	voxelTerrain.caveNoise = get_cave_noise();
	voxelTerrain.biomes[BIOME_FLAT] = get_biome_flat_noise();
	voxelTerrain.biomes[BIOME_SMALL_HILL] = get_biome_small_hill_noise();
	voxelTerrain.biomes[BIOME_HILL] = get_biome_hill_noise();
	voxelTerrain.biomes[BIOME_MOUNTAIN] = get_biome_mountain_noise();
	voxelTerrain.biomes[BIOME_HIGH_MOUNTAIN] = get_biome_high_mountain_noise();
}

static void LoadTerrainChunks()
{
	for (uint32_t x = 0; x < TERRAIN_VIEW_RANGE; ++x)
		for (uint32_t z = 0; z < TERRAIN_VIEW_RANGE; ++z)
		{
			if(SurroundGroupsAreLoaded(x, z))
			{
				if(GroupNeedsFaces(x, z)) SendGroupFaceCreationJob(x, z);
				else
				{
					TerrainChunkGroup group = voxelTerrain.chunkGroups[x * TERRAIN_VIEW_RANGE + z];
					for (int y = 0; y < TERRAIN_HEIGHT; ++y)
						if(group.chunks[y].state == CHUNK_REQUIRES_FACES) SendFaceCreationJob(x, y, z);
				}
			}
		}
}

static TerrainChunkGroup InitializeChunkGroup(uint32_t ssboId)
{
	TerrainChunkGroup group =
		{
			.state = CHUNK_GROUP_REQUIRES_NOISE_MAP,
			.id = { 0, 0 },
			.isAlive = true,
			.heightMap = CM_MALLOC(TERRAIN_CHUNK_HORIZONTAL_SLICE),
			.ssboId = ssboId
		};

	for (int y = 0; y < TERRAIN_HEIGHT; ++y)
	{
		TerrainChunk chunk =
			{
				.flags = 0,
				.buffer = NULL,
				.voxels = CM_MALLOC(TERRAIN_CHUNK_VOXEL_COUNT),
				.fbOpaqueMask = CM_MALLOC(TERRAIN_CHUNK_SIZE * TERRAIN_CHUNK_SIZE * sizeof(uint64_t)),
				.rlOpaqueMask = CM_MALLOC(TERRAIN_CHUNK_SIZE * TERRAIN_CHUNK_SIZE * sizeof(uint64_t)),
				.tbOpaqueMask = CM_MALLOC(TERRAIN_CHUNK_SIZE * TERRAIN_CHUNK_SIZE * sizeof(uint64_t)),
				.state = CHUNK_REQUIRES_FACES,
				.yId = y,
			};

		group.chunks[y] = chunk;
	}

	return group;
}

static void RecreateChunkGroup(TerrainChunkGroup* group, uint32_t x, uint32_t z)
{
	UnloadChunkGroup(group);
	group->id[0] = x;
	group->id[1] = z;
}

static void UnloadChunkGroup(TerrainChunkGroup* group)
{
	group->state = CHUNK_GROUP_REQUIRES_NOISE_MAP;
	memset(group->heightMap, 0, TERRAIN_CHUNK_HORIZONTAL_SLICE);
	for (int y = 0; y < TERRAIN_HEIGHT; ++y)
	{
		TerrainChunk* chunk = &group->chunks[y];
		chunk->state = CHUNK_REQUIRES_FACES;
		uint8_t bufferSize = (chunk->flags & 0b0111) >> 1;
		chunk->flags &= 0;
		chunk->flags |= bufferSize << 1;

		if(chunk->buffer) memset(chunk->buffer, 0, BUFFER_SIZE_STAGES[bufferSize] * sizeof(uint32_t));
		memset(chunk->voxels, 0, TERRAIN_CHUNK_VOXEL_COUNT);
		memset(chunk->fbOpaqueMask, 0, TERRAIN_CHUNK_SIZE * TERRAIN_CHUNK_SIZE * sizeof(uint64_t));
		memset(chunk->rlOpaqueMask, 0, TERRAIN_CHUNK_SIZE * TERRAIN_CHUNK_SIZE * sizeof(uint64_t));
		memset(chunk->tbOpaqueMask, 0, TERRAIN_CHUNK_SIZE * TERRAIN_CHUNK_SIZE * sizeof(uint64_t));
	}
}

static void DestroyChunkGroup(TerrainChunkGroup* group)
{
	if(!group->isAlive) return;
	group->isAlive = false;
	CM_FREE(group->heightMap);

	for (int y = 0; y < TERRAIN_HEIGHT; ++y)
	{
		CM_FREE(group->chunks[y].voxels);
		if(group->chunks[y].buffer) CM_FREE(group->chunks[y].buffer);
		CM_FREE(group->chunks[y].fbOpaqueMask);
		CM_FREE(group->chunks[y].rlOpaqueMask);
		CM_FREE(group->chunks[y].tbOpaqueMask);
	}
}

//endregion

//region Positional Loading

static void RecreateGroup(ivec2 id, int x, int z)
{
	ivec2 chunkId;
	glm_ivec2_add(id, (ivec2){x - TERRAIN_VIEW_RANGE / 2, z - TERRAIN_VIEW_RANGE / 2}, chunkId);

	RecreateChunkGroup(&voxelTerrain.chunkGroups[x * TERRAIN_VIEW_RANGE + z],
	                   chunkId[0], chunkId[1]);
	SendNoiseJob(x, z);
}

static void SetupInitialChunks(Camera3D camera)
{
	vec3 position;
	glm_vec3_copy(camera.position, position);
	ivec2 id;
	id[0] = (int)(position[0] / TERRAIN_CHUNK_SIZE) + TERRAIN_WORLD_EDGE;
	id[1] = (int)(position[2] / TERRAIN_CHUNK_SIZE) + TERRAIN_WORLD_EDGE;

	glm_ivec2_copy(id, voxelTerrain.loadedCenter);

	for (int x = 0; x < TERRAIN_VIEW_RANGE; ++x)
		for (int z = 0; z < TERRAIN_VIEW_RANGE; ++z)
			RecreateGroup(id, x, z);
}

static void SetRequiresFaces(uint32_t x, uint32_t z)
{
	TerrainChunkGroup* group = &voxelTerrain.chunkGroups[x * TERRAIN_VIEW_RANGE + z];

	for (uint32_t y = 0; y < TERRAIN_HEIGHT; ++y)
		group->chunks[y].state = CHUNK_REQUIRES_FACES;
}

static void ReloadChunks(Camera3D camera)
{
	if(voxelTerrain.pool->jobCount > 0) return;

	vec3 position = { 0 };
	glm_vec3_copy(camera.position, position);
	ivec2 id;
	id[0] = (int)(position[0] / TERRAIN_CHUNK_SIZE) + TERRAIN_WORLD_EDGE;
	id[1] = (int)(position[2] / TERRAIN_CHUNK_SIZE) + TERRAIN_WORLD_EDGE;

	if(!glm_ivec2_eqv(id, voxelTerrain.loadedCenter))
	{
		ivec2 dataShift;
		glm_ivec2_sub(id, voxelTerrain.loadedCenter, dataShift);

		if(dataShift[0] > 0)
		{
			id[1] = voxelTerrain.loadedCenter[1];

			size_t shiftSize = dataShift[0] * TERRAIN_VIEW_RANGE * sizeof(TerrainChunkGroup);
			size_t inverseShiftSize = TERRAIN_VIEW_RANGE * TERRAIN_VIEW_RANGE * sizeof(TerrainChunkGroup) - shiftSize;

			memcpy(voxelTerrain.shiftGroups, voxelTerrain.chunkGroups, shiftSize);
			memcpy(voxelTerrain.chunkGroups, &voxelTerrain.chunkGroups[dataShift[0] * TERRAIN_VIEW_RANGE],
			       inverseShiftSize);

			memcpy(&voxelTerrain.chunkGroups[(TERRAIN_VIEW_RANGE - dataShift[0]) * TERRAIN_VIEW_RANGE], voxelTerrain.shiftGroups, shiftSize);

			for (int x = TERRAIN_VIEW_RANGE - dataShift[0]; x < TERRAIN_VIEW_RANGE; ++x)
				for (int z = 0; z < TERRAIN_VIEW_RANGE; ++z)
					RecreateGroup(id, x, z);

			for (uint32_t x = TERRAIN_VIEW_RANGE - dataShift[0] - 1; x < TERRAIN_VIEW_RANGE - dataShift[0]; ++x)
				for (uint32_t z = 0; z < TERRAIN_VIEW_RANGE; ++z)
					SetRequiresFaces(x, z);

			voxelTerrain.loadedCenter[0] = id[0];
		}
		else if(dataShift[0] < 0)
		{
			id[1] = voxelTerrain.loadedCenter[1];

			dataShift[0] = -dataShift[0];
			size_t shiftSize = dataShift[0] * TERRAIN_VIEW_RANGE * sizeof(TerrainChunkGroup);
			size_t inverseShiftSize = TERRAIN_VIEW_RANGE * TERRAIN_VIEW_RANGE * sizeof(TerrainChunkGroup) - shiftSize;

			memcpy(voxelTerrain.shiftGroups, &voxelTerrain.chunkGroups[(TERRAIN_VIEW_RANGE - dataShift[0]) * TERRAIN_VIEW_RANGE], shiftSize);
			memcpy(&voxelTerrain.chunkGroups[dataShift[0] * TERRAIN_VIEW_RANGE], voxelTerrain.chunkGroups, inverseShiftSize);

			memcpy(voxelTerrain.chunkGroups, voxelTerrain.shiftGroups, shiftSize);

			for (int x = 0; x < dataShift[0]; ++x)
				for (int z = 0; z < TERRAIN_VIEW_RANGE; ++z)
					RecreateGroup(id, x, z);

			for (int x = dataShift[0]; x < dataShift[0] + 1; ++x)
				for (int z = 0; z < TERRAIN_VIEW_RANGE; ++z)
					SetRequiresFaces(x, z);

			voxelTerrain.loadedCenter[0] = id[0];
		}
		else if(dataShift[1] > 0)
		{
			id[0] = voxelTerrain.loadedCenter[0];

			size_t shiftSize = dataShift[1] * sizeof(TerrainChunkGroup);
			size_t inverseShiftSize = TERRAIN_VIEW_RANGE * sizeof(TerrainChunkGroup) - shiftSize;

			for (int x = 0; x < TERRAIN_VIEW_RANGE; ++x)
			{
				memcpy(voxelTerrain.shiftGroups, &voxelTerrain.chunkGroups[x * TERRAIN_VIEW_RANGE], shiftSize);
				memcpy(&voxelTerrain.chunkGroups[x * TERRAIN_VIEW_RANGE], &voxelTerrain.chunkGroups[x * TERRAIN_VIEW_RANGE + dataShift[1]],
				       inverseShiftSize);

				memcpy(&voxelTerrain.chunkGroups[x * TERRAIN_VIEW_RANGE + (TERRAIN_VIEW_RANGE - dataShift[1])], voxelTerrain.shiftGroups, shiftSize);
			}

			for (int x = 0; x < TERRAIN_VIEW_RANGE; ++x)
				for (int z = TERRAIN_VIEW_RANGE - dataShift[1]; z < TERRAIN_VIEW_RANGE; ++z)
					RecreateGroup(id, x, z);

			for (int x = 0; x < TERRAIN_VIEW_RANGE; ++x)
				for (int z = TERRAIN_VIEW_RANGE - dataShift[1] - 1; z < TERRAIN_VIEW_RANGE - dataShift[1]; ++z)
					SetRequiresFaces(x, z);

			voxelTerrain.loadedCenter[1] = id[1];
		}
		else if(dataShift[1] < 0)
		{
			dataShift[1] = -dataShift[1];
			id[0] = voxelTerrain.loadedCenter[0];

			size_t shiftSize = dataShift[1] * sizeof(TerrainChunkGroup);
			size_t inverseShiftSize = TERRAIN_VIEW_RANGE * sizeof(TerrainChunkGroup) - shiftSize;

			for (int x = 0; x < TERRAIN_VIEW_RANGE; ++x)
			{
				memcpy(voxelTerrain.shiftGroups, &voxelTerrain.chunkGroups[x * TERRAIN_VIEW_RANGE + (TERRAIN_VIEW_RANGE - dataShift[1])], shiftSize);
				memcpy(&voxelTerrain.chunkGroups[x * TERRAIN_VIEW_RANGE + dataShift[1]], &voxelTerrain.chunkGroups[x * TERRAIN_VIEW_RANGE],
				       inverseShiftSize);

				memcpy(&voxelTerrain.chunkGroups[x * TERRAIN_VIEW_RANGE], voxelTerrain.shiftGroups, shiftSize);
			}

			for (int x = 0; x < TERRAIN_VIEW_RANGE; ++x)
				for (int z = 0; z < dataShift[1]; ++z)
					RecreateGroup(id, x, z);

			for (int x = 0; x < TERRAIN_VIEW_RANGE; ++x)
				for (int z =  dataShift[1]; z < dataShift[1] + 1; ++z)
					SetRequiresFaces(x, z);

			voxelTerrain.loadedCenter[1] = id[1];
		}
	}
}

//endregion

//region Noise

static void SendNoiseJob(uint32_t x, uint32_t z)
{
	TerrainChunkGroup* group = &voxelTerrain.chunkGroups[x * TERRAIN_VIEW_RANGE + z];
	group->state = CHUNK_GROUP_GENERATING_NOISE_MAP;

	uint32_t * args = CM_MALLOC(4 * sizeof(uint32_t ));
	args[0] = group->id[0];
	args[1] = group->id[1];
	args[2] = x;
	args[3] = z;
	ThreadJob job = {0};
	job.args = args;
	job.job = T_GenerateNoise;
	job.callbackJob = T_OnNoiseGenerationFinished;
	cm_submit_job(voxelTerrain.pool, job, false);
}

static void T_GenerateNoise(void* args)
{
	uint32_t * cArgs = (uint32_t *)args;
	GenerateHeightMap((uint32_t [2]){cArgs[0], cArgs[1]}, (uint32_t [2]){cArgs[2], cArgs[3]});
	for (int y = 0; y < TERRAIN_HEIGHT; ++y)
	{
		GeneratePreChunk(cArgs[2], y, cArgs[3]);
		GeneratePostChunk(cArgs[2], y, cArgs[3]);
	}
}

static void T_OnNoiseGenerationFinished(void* args)
{
	uint32_t * cArgs = (uint32_t *)args;

	TerrainChunkGroup* group = &voxelTerrain.chunkGroups[cArgs[2] * TERRAIN_VIEW_RANGE + cArgs[3]];
	group->state = CHUNK_GROUP_READY;
}

static void GenerateHeightMap(const uint32_t sourceId[2], const uint32_t destination[2])
{
	uint8_t * heightMap = voxelTerrain.chunkGroups[destination[0] * TERRAIN_VIEW_RANGE + destination[1]].heightMap;
	fnl_state noise = voxelTerrain.biomes[BIOME_FLAT];

	for (uint32_t x = 0; x < TERRAIN_CHUNK_SIZE; ++x)
	{
		uint32_t px = sourceId[0] * TERRAIN_CHUNK_SIZE + x;
		for (uint32_t z = 0; z < TERRAIN_CHUNK_SIZE; ++z)
		{
			uint32_t pz = sourceId[1] * TERRAIN_CHUNK_SIZE + z;

			float val2D = fnlGetNoise2D(&noise, (double)(px), (double)(pz));
			val2D = (val2D + 1) * .5f;
			uint8_t height = (TERRAIN_LOWER_EDGE * TERRAIN_CHUNK_SIZE) +
			                       (uint8_t)(val2D * (TERRAIN_CHUNK_SIZE * (TERRAIN_UPPER_EDGE - TERRAIN_LOWER_EDGE) - 1));

			heightMap[x * TERRAIN_CHUNK_SIZE + z] = height;
		}
	}
}

static void GeneratePreChunk(uint32_t xId, uint32_t yId, uint32_t zId)
{
	TerrainChunkGroup* group = &voxelTerrain.chunkGroups[xId * TERRAIN_VIEW_RANGE + zId];
	uint8_t* heightMap = group->heightMap;
	uint8_t* cells = group->chunks[yId].voxels;
	uint64_t* fbMask = group->chunks[yId].fbOpaqueMask;
	uint64_t* rlMask = group->chunks[yId].rlOpaqueMask;
	uint64_t* tbMask = group->chunks[yId].tbOpaqueMask;
	uint32_t groupId[2] = { group->id[0], group->id[1] };

	for (uint32_t x = 0; x < TERRAIN_CHUNK_SIZE; ++x)
	{
		uint32_t px = groupId[0] * TERRAIN_CHUNK_SIZE + x;

		for (uint32_t z = 0; z < TERRAIN_CHUNK_SIZE; ++z)
		{
			uint32_t xzId = x * TERRAIN_CHUNK_SIZE + z;
			uint32_t pz = groupId[1] * TERRAIN_CHUNK_SIZE + z;
			int maxY = (int)heightMap[x * TERRAIN_CHUNK_SIZE + z] - (int)(yId * TERRAIN_CHUNK_SIZE);
			int yLimit = glm_imin((int)(TERRAIN_CHUNK_SIZE - 1), maxY);
			if(yLimit < 0) continue;

			for (uint32_t y = 0; y <= yLimit; ++y)
			{
				uint32_t py = yId * TERRAIN_CHUNK_SIZE + y;
				float caveValue = fnlGetNoise3D(&voxelTerrain.caveNoise, (double)(px), (double)(py), (double)(pz));

				if(caveValue >= TERRAIN_CAVE_EDGE)
				{
					caveValue = (caveValue - TERRAIN_CAVE_EDGE) / (1 - TERRAIN_CAVE_EDGE);

					uint32_t id = y * TERRAIN_CHUNK_HORIZONTAL_SLICE + xzId;
					uint8_t block = get_block_type(caveValue);
					cells[id] = block;
					fbMask[y * TERRAIN_CHUNK_SIZE + x] |= 1llu << z;
					rlMask[z * TERRAIN_CHUNK_SIZE + y] |= 1llu << x;
					tbMask[x * TERRAIN_CHUNK_SIZE + z] |= 1llu << y;
				}
			}
		}
	}

#undef SET_CHUNK
}

static void GeneratePostChunk(uint32_t xId, uint32_t yId, uint32_t zId)
{
	TerrainChunkGroup* group = &voxelTerrain.chunkGroups[xId * TERRAIN_VIEW_RANGE + zId];
	uint8_t* heightMap = group->heightMap;
	uint8_t* cells = group->chunks[yId].voxels;
	for (uint32_t x = 0; x < TERRAIN_CHUNK_SIZE; ++x)
	{
		for (uint32_t z = 0; z < TERRAIN_CHUNK_SIZE; ++z)
		{
			uint32_t xzId = x * TERRAIN_CHUNK_SIZE + z;
			int maxY = (int)heightMap[x * TERRAIN_CHUNK_SIZE + z] - (int)(yId * TERRAIN_CHUNK_SIZE);
			int yLimit = glm_imin((int)(TERRAIN_CHUNK_SIZE - 1), maxY);
			if(yLimit < 0) continue;

			for (uint32_t y = 0; y <= yLimit; ++y)
			{
				uint32_t id = y * TERRAIN_CHUNK_HORIZONTAL_SLICE + xzId;
				if(cells[id] == BLOCK_EMPTY) continue;
				
				if(y == maxY) cells[id] = BLOCK_GRASS;
				else if(maxY - y < 3) cells[id] = BLOCK_DIRT;
			}
		}
	}
}

//endregion

//region Face Creation

static void SendFaceCreationJob(uint32_t x, uint32_t y, uint32_t z)
{
	TerrainChunk* chunk = &voxelTerrain.chunkGroups[x * TERRAIN_VIEW_RANGE + z].chunks[y];
	chunk->state = CHUNK_CREATING_FACES;
	
	uint32_t * args = CM_MALLOC(3 * sizeof(uint32_t ));
	args[0] = x;
	args[1] = y;
	args[2] = z;

	ThreadJob job = {0};
	job.args = args;
	job.job = T_CreateChunkFaces;
	job.callbackJob = T_ChunkFacesCreationFinished;
	cm_submit_job(voxelTerrain.pool, job, true);
}

static void T_CreateChunkFaces(void* args)
{
	uint32_t * cArgs = (uint32_t *)args;
	CreateChunkFaces(cArgs[0], cArgs[1], cArgs[2]);
}

static void T_ChunkFacesCreationFinished(void* args)
{
	uint32_t * cArgs = (uint32_t *)args;
	voxelTerrain.chunkGroups[cArgs[0] * TERRAIN_VIEW_RANGE + cArgs[2]].chunks[cArgs[1]].state = CHUNK_REQUIRES_UPLOAD;
}

static void SendGroupFaceCreationJob(uint32_t x, uint32_t z)
{
	uint32_t* args = CM_MALLOC(2 * sizeof(uint32_t));
	args[0] = x;
	args[1] = z;
	
	ThreadJob job = {0};
	job.args = args;
	job.job = T_CreateGroupFaces;
	job.callbackJob = T_GroupFacesCreationFinished;
	
	TerrainChunkGroup* group = &voxelTerrain.chunkGroups[x * TERRAIN_VIEW_RANGE + z];
	
	for (int y = 0; y < TERRAIN_HEIGHT; ++y)
		group->chunks[y].state = CHUNK_CREATING_FACES;

	cm_submit_job(voxelTerrain.pool, job, true);
}

static void T_CreateGroupFaces(void* args)
{
	uint32_t * cArgs = (uint32_t *)args;
	
	for (int y = 0; y < TERRAIN_HEIGHT; ++y)
		CreateChunkFaces(cArgs[0], y, cArgs[1]);
}

static void T_GroupFacesCreationFinished(void* args)
{
	uint32_t * cArgs = (uint32_t *)args;
	
	TerrainChunkGroup* group = &voxelTerrain.chunkGroups[cArgs[0] * TERRAIN_VIEW_RANGE + cArgs[1]];
	
	for (int y = 0; y < TERRAIN_HEIGHT; ++y)
		group->chunks[y].state = CHUNK_REQUIRES_UPLOAD;
}

static inline void AddFace(uint32_t* buffer, uint32_t faceCount, uint32_t block)
{
	block <<= 2;
	buffer[faceCount * 6 + 0] = block | 0b00;
	buffer[faceCount * 6 + 1] = block | 0b01;
	buffer[faceCount * 6 + 2] = block | 0b11;
	buffer[faceCount * 6 + 3] = block | 0b00;
	buffer[faceCount * 6 + 4] = block | 0b11;
	buffer[faceCount * 6 + 5] = block | 0b10;
}

static inline void CreateFaceMask(const uint64_t* oMask, const uint64_t* fMask, const uint64_t* bMask,
                                  uint64_t* tf, uint64_t* tb, uint32_t id)
{
	const static uint64_t endMask = 1ull << (TERRAIN_CHUNK_SIZE - 1ull);
	uint64_t mask = oMask[id];
	tf[id] = (mask & ~(mask >> 1ull)) & ~((uint64_t)(fMask == NULL || (fMask[id] & 1ull)) << (TERRAIN_CHUNK_SIZE - 1ull));
	tb[id] = (mask & ~(mask << 1ull)) & ~(bMask == NULL || (bMask[id] & endMask));
}

static void CreateChunkFaces(uint32_t xId, uint32_t yId, uint32_t zId)
{
	//region defines
#define BUFFER_CHECK \
	if(bufferSize < faceCount + 6)\
	{\
		uint32_t oldBufferSize = bufferSize;\
		bufferSizeIndex++;\
		bufferSize = BUFFER_SIZE_STAGES[bufferSizeIndex];\
		void* newMemory = CM_REALLOC(chunk->buffer, bufferSize * sizeof(uint32_t) * 6);\
		if(newMemory == NULL)\
		{\
			perror("Unable to reallocate memory!!! exiting the program.\n");\
			exit(-1);\
		}\
		else\
		{\
			memset((uint32_t*)newMemory + oldBufferSize * 6, 0, (bufferSize - oldBufferSize) * sizeof(uint32_t) * 6);\
			chunk->buffer = newMemory;\
			buffer = newMemory;\
			chunk->flags &= 0xfffffff1;\
			chunk->flags |= bufferSizeIndex << 1;\
		}\
	}
//endregion

	uint32_t faceCount = 0;
	TerrainChunkGroup* group = &voxelTerrain.chunkGroups[xId * TERRAIN_VIEW_RANGE + zId];
	TerrainChunk* chunk = &group->chunks[yId];
	uint32_t* buffer = chunk->buffer;
	uint64_t* fbMask = chunk->fbOpaqueMask;
	uint64_t* rlMask = chunk->rlOpaqueMask;
	uint64_t* tbMask = chunk->tbOpaqueMask;

	uint64_t* frontChunkOpaqueMask = NULL;
	uint64_t* backChunkOpaqueMask = NULL;
	uint64_t* rightChunkOpaqueMask = NULL;
	uint64_t* leftChunkOpaqueMask = NULL;
	uint64_t* topChunkOpaqueMask = NULL;
	uint64_t* bottomChunkOpaqueMask = NULL;

	if(zId < TERRAIN_VIEW_RANGE - 1) frontChunkOpaqueMask = voxelTerrain.chunkGroups[xId * TERRAIN_VIEW_RANGE + zId + 1].chunks[yId].fbOpaqueMask;
	if(zId > 0) backChunkOpaqueMask = voxelTerrain.chunkGroups[xId * TERRAIN_VIEW_RANGE + zId - 1].chunks[yId].fbOpaqueMask;
	if(xId < TERRAIN_VIEW_RANGE - 1) rightChunkOpaqueMask = voxelTerrain.chunkGroups[(xId + 1) * TERRAIN_VIEW_RANGE + zId].chunks[yId].rlOpaqueMask;
	if(xId > 0) leftChunkOpaqueMask = voxelTerrain.chunkGroups[(xId - 1) * TERRAIN_VIEW_RANGE + zId].chunks[yId].rlOpaqueMask;
	if(yId < TERRAIN_HEIGHT - 1) topChunkOpaqueMask = group->chunks[yId + 1].tbOpaqueMask;
	if(yId > 0) bottomChunkOpaqueMask = group->chunks[yId - 1].tbOpaqueMask;

	uint8_t bufferSizeIndex = (chunk->flags & 0b1110) >> 1;
	uint32_t bufferSize = BUFFER_SIZE_STAGES[bufferSizeIndex];

	//region Front & Back
	uint64_t frontFaceMask[TERRAIN_CHUNK_SIZE * TERRAIN_CHUNK_SIZE];
	uint64_t backFaceMask[TERRAIN_CHUNK_SIZE * TERRAIN_CHUNK_SIZE];

	for (uint32_t y = 0; y < TERRAIN_CHUNK_SIZE; ++y)
	{
		for (uint32_t x = 0; x < TERRAIN_CHUNK_SIZE; ++x)
		{
			uint32_t id = y * TERRAIN_CHUNK_SIZE + x;
			CreateFaceMask(fbMask, frontChunkOpaqueMask, backChunkOpaqueMask,
						   frontFaceMask, backFaceMask, id);
		}
	}
	//endregion

	//region Right & Left
	uint64_t rightFaceMask[TERRAIN_CHUNK_SIZE * TERRAIN_CHUNK_SIZE];
	uint64_t leftFaceMask[TERRAIN_CHUNK_SIZE * TERRAIN_CHUNK_SIZE];

	for (uint32_t y = 0; y < TERRAIN_CHUNK_SIZE; ++y)
	{
		for (uint32_t z = 0; z < TERRAIN_CHUNK_SIZE; ++z)
		{
			uint32_t id = z * TERRAIN_CHUNK_SIZE + y;
			CreateFaceMask(rlMask, rightChunkOpaqueMask, leftChunkOpaqueMask,
			               rightFaceMask, leftFaceMask, id);
		}
	}
	//endregion

	//region Top & Bottom
	uint64_t topFaceMask[TERRAIN_CHUNK_SIZE * TERRAIN_CHUNK_SIZE];
	uint64_t bottomFaceMask[TERRAIN_CHUNK_SIZE * TERRAIN_CHUNK_SIZE];

	for (uint32_t y = 0; y < TERRAIN_CHUNK_SIZE; ++y)
	{
		for (uint32_t z = 0; z < TERRAIN_CHUNK_SIZE; ++z)
		{
			uint32_t id = y * TERRAIN_CHUNK_SIZE + z;
			CreateFaceMask(tbMask, topChunkOpaqueMask, bottomChunkOpaqueMask,
			               topFaceMask, bottomFaceMask, id);
		}
	}
	//endregion

	uint64_t* faces[] = { frontFaceMask, backFaceMask, rightFaceMask, leftFaceMask, topFaceMask, bottomFaceMask };

	for (uint32_t i = 0; i < 2; ++i)
	{
		uint64_t* currentFace = faces[i];

		for (uint32_t y = 0; y < TERRAIN_CHUNK_SIZE; ++y)
		{
			for (uint32_t x = 0; x < TERRAIN_CHUNK_SIZE; ++x)
			{
				uint64_t mask = currentFace[y * TERRAIN_CHUNK_SIZE + x];
				while(mask != 0llu)
				{
					uint32_t z = cm_trailing_zeros(mask);
					mask &= mask - 1;

					BUFFER_CHECK

					uint32_t blockIndex = (x << 12) | (y << 6) | z;
					blockIndex <<= 11;
					// FaceId | SizeX | SizeY
					blockIndex |= (i << 8) | (1 << 4) | 1;

					AddFace(buffer, faceCount, blockIndex);
					faceCount++;
				}
			}
		}
	}

	for (uint32_t i = 2; i < 4; ++i)
	{
		uint64_t* currentFace = faces[i];

		for (uint32_t z = 0; z < TERRAIN_CHUNK_SIZE; ++z)
		{
			for (uint32_t y = 0; y < TERRAIN_CHUNK_SIZE; ++y)
			{
				uint64_t mask = currentFace[z * TERRAIN_CHUNK_SIZE + y];
				while(mask != 0llu)
				{
					uint32_t x = cm_trailing_zeros(mask);
					mask &= mask - 1;

					BUFFER_CHECK

					uint32_t blockIndex = (x << 12) | (y << 6) | z;
					blockIndex <<= 11;
					// FaceId | SizeX | SizeY
					blockIndex |= (i << 8) | (1 << 4) | 1;

					AddFace(buffer, faceCount, blockIndex);
					faceCount++;
				}
			}
		}
	}

	for (uint32_t i = 4; i < 6; ++i)
	{
		uint64_t* currentFace = faces[i];

		for (uint32_t x = 0; x < TERRAIN_CHUNK_SIZE; ++x)
		{
			for (uint32_t z = 0; z < TERRAIN_CHUNK_SIZE; ++z)
			{
				uint64_t mask = currentFace[x * TERRAIN_CHUNK_SIZE + z];
				while(mask != 0llu)
				{
					uint32_t y = cm_trailing_zeros(mask);
					mask &= mask - 1;

					BUFFER_CHECK

					uint32_t blockIndex = (x << 12) | (y << 6) | z;
					blockIndex <<= 11;
					// FaceId | SizeX | SizeY
					blockIndex |= (i << 8) | (1 << 4) | 1;

					AddFace(buffer, faceCount, blockIndex);
					faceCount++;
				}
			}
		}
	}

#undef RECT_FACE
	chunk->flags &= 0x0000ffff;
	chunk->flags |= faceCount << 16;
	voxelTerrain.chunkVaos[group->ssboId * TERRAIN_HEIGHT + yId].vbo.vertexCount = faceCount * 6;

#undef BUFFER_CHECK
}

//endregion

//region Utils

static void PassTerrainDataToShader(UniformData* data)
{
	uint32_t index[4];
	memcpy(index, data->chunk, sizeof(ivec3));
	index[3] = data->chunkId;
	cm_set_uniform_uvec4(voxelTerrain.u_chunkIndex, index);
}

static bool SurroundGroupsAreLoaded(uint32_t xId, uint32_t zId)
{
	bool areLoaded = voxelTerrain.chunkGroups[xId * TERRAIN_VIEW_RANGE + zId].state == CHUNK_GROUP_READY;
	areLoaded = areLoaded && (xId == 0 || voxelTerrain.chunkGroups[(xId - 1) * TERRAIN_VIEW_RANGE + zId].state == CHUNK_GROUP_READY);
	areLoaded = areLoaded && (xId == (TERRAIN_VIEW_RANGE - 1) || voxelTerrain.chunkGroups[(xId + 1) * TERRAIN_VIEW_RANGE + zId].state == CHUNK_GROUP_READY);

	areLoaded = areLoaded && (zId == 0 || voxelTerrain.chunkGroups[xId * TERRAIN_VIEW_RANGE + zId - 1].state == CHUNK_GROUP_READY);
	areLoaded = areLoaded && (zId == (TERRAIN_VIEW_RANGE - 1) || voxelTerrain.chunkGroups[xId * TERRAIN_VIEW_RANGE + zId + 1].state == CHUNK_GROUP_READY);

	return areLoaded;
}

static bool GroupNeedsFaces(uint32_t xId, uint32_t zId)
{
	TerrainChunkGroup group = voxelTerrain.chunkGroups[xId * TERRAIN_VIEW_RANGE + zId];
	bool needsFaces = true;
	for (int y = 0; y < TERRAIN_HEIGHT; ++y)
		needsFaces = needsFaces && group.chunks[y].state == CHUNK_REQUIRES_FACES;
	return needsFaces;
}

static bool DelayedLoader()
{
	uint32_t start = TERRAIN_VIEW_RANGE / 2 - TERRAIN_LOADING_EDGE, end = start + TERRAIN_LOADING_EDGE * 2;
	
	for (uint32_t x = start; x < end; ++x)
	{
		for (uint32_t z = start; z < end; ++z)
		{
			if(voxelTerrain.chunkGroups[x * TERRAIN_VIEW_RANGE + z].state != CHUNK_GROUP_READY)
				return true;
		}
	}
	
	return false;
}

static bool TryUploadGroup(TerrainChunkGroup* group)
{
	bool uploaded = false;
	for (int y = 0; y < TERRAIN_HEIGHT; ++y)
	{
		TerrainChunk* chunk = &group->chunks[y];
		uint16_t faceCount = (chunk->flags & 0xffff0000) >> 16;
		if(faceCount == 0) continue;

		if(chunk->state == CHUNK_REQUIRES_UPLOAD)
		{
			uint32_t id = group->ssboId * TERRAIN_HEIGHT + y;
			cm_reupload_vbo(&voxelTerrain.chunkVaos[id].vbo, faceCount * sizeof(uint32_t) * 6, chunk->buffer);
			cm_upload_ssbo(voxelTerrain.voxelsSsbo, id * TERRAIN_CHUNK_VOXEL_COUNT, TERRAIN_CHUNK_VOXEL_COUNT, chunk->voxels);
			chunk->state = CHUNK_READY_TO_DRAW;
			chunk->flags |= 1;
			uploaded = true;
		}
	}
	return uploaded;
}

//endregion