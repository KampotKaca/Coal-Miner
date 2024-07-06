#include "terrain.h"
#include "terrainConfig.h"
#include "coal_miner_internal.h"
#include <FastNoiseLite.h>
#include "camera.h"

//region structures
#define TERRAIN_CHUNK_CUBE_COUNT TERRAIN_CHUNK_SIZE * TERRAIN_CHUNK_SIZE * TERRAIN_CHUNK_SIZE
#define TERRAIN_CHUNK_COUNT TERRAIN_VIEW_RANGE * TERRAIN_VIEW_RANGE * TERRAIN_HEIGHT
#define CHUNK_HORIZONTAL_SLICE TERRAIN_CHUNK_SIZE * TERRAIN_CHUNK_SIZE

const unsigned int BUFFER_SIZE_STAGES[] =
{
	0, 128, 512, TERRAIN_CHUNK_CUBE_COUNT / 16,
	TERRAIN_CHUNK_CUBE_COUNT / 8, TERRAIN_CHUNK_CUBE_COUNT / 4,
	TERRAIN_CHUNK_CUBE_COUNT / 2, TERRAIN_CHUNK_CUBE_COUNT
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
	unsigned int chunkId;
	ivec3 chunk;
} UniformData;

typedef struct
{
	ChunkState state;
	//1bit is uploaded
	//3bit buffer size
	//16bit faceCount
	unsigned int flags;
	unsigned int yId;

	unsigned int* buffer;
	unsigned char* cells;
}TerrainChunk;

typedef struct
{
	TerrainChunk chunks[TERRAIN_HEIGHT];
	ChunkGroupState state;
	unsigned int id[2];
	unsigned int ssboId;
	unsigned char* heightMap;
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

	Vao quadVao;
	Ssbo ssbo;

	ivec2 loadedCenter;
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

static TerrainChunkGroup InitializeChunkGroup(unsigned int ssboId);
static void RecreateChunkGroup(TerrainChunkGroup* group, unsigned int x, unsigned int z);
static void UnloadChunkGroup(TerrainChunkGroup* group);
static void DestroyChunkGroup(TerrainChunkGroup* group);

//Positional Loading
static void SetupInitialChunks(Camera3D camera);
static void ReloadChunks(Camera3D camera);

//Noise
static void SendNoiseJob(unsigned int x, unsigned int z);
static void T_GenerateNoise(void* args);
static void T_OnNoiseGenerationFinished(void* args);

static void GenerateHeightMap(const unsigned int sourceId[2], const unsigned int destination[2]);
static void GeneratePreChunk(unsigned int xId, unsigned int yId, unsigned int zId);
static void GeneratePostChunk(unsigned int xId, unsigned int yId, unsigned int zId);

//Face Creation
static void SendFaceCreationJob(unsigned int x, unsigned int y, unsigned int z);
static void T_CreateChunkFaces(void* args);
static void T_ChunkFacesCreationFinished(void* args);

static void SendGroupFaceCreationJob(unsigned int x, unsigned int z);
static void T_CreateGroupFaces(void* args);
static void T_GroupFacesCreationFinished(void* args);

static void CreateChunkFaces(unsigned int xId, unsigned int yId, unsigned int zId);

//Utils
static void PassTerrainDataToShader(UniformData* data);
static bool SurroundGroupsAreLoaded(unsigned int xId, unsigned int zId);
static bool GroupNeedsFaces(unsigned int xId, unsigned int zId);
static bool DelayedLoader();
static bool TryUploadGroup(TerrainChunkGroup* group);

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
	LoadBuffers();
	InitTerrainNoise();
	
	voxelTerrain.shiftGroups = CM_MALLOC(TERRAIN_VIEW_RANGE * TERRAIN_VIEW_RANGE * sizeof(TerrainChunkGroup));
	
	for (int x = 0; x < TERRAIN_VIEW_RANGE; ++x)
		for (int z = 0; z < TERRAIN_VIEW_RANGE; ++z)
			voxelTerrain.chunkGroups[x * TERRAIN_VIEW_RANGE + z] = InitializeChunkGroup(x * TERRAIN_VIEW_RANGE + z);
	
	voxelTerrain.pool = cm_create_thread_pool(TERRAIN_NUM_WORKER_THREADS);

	SetupInitialChunks(get_camera());
}

bool loading_terrain()
{
	LoadTerrainChunks();
	
	for (unsigned int x = 0; x < TERRAIN_VIEW_RANGE; ++x)
		for (unsigned int z = 0; z < TERRAIN_VIEW_RANGE; ++z)
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
	
	for (unsigned int x = 0; x < TERRAIN_VIEW_RANGE; ++x)
	{
		for (unsigned int z = 0; z < TERRAIN_VIEW_RANGE; ++z)
		{
			TerrainChunkGroup* group = &voxelTerrain.chunkGroups[x * TERRAIN_VIEW_RANGE + z];
			if(numUploadsLeft > 0) numUploadsLeft -= TryUploadGroup(group);

			for (unsigned int y = 0; y < TERRAIN_HEIGHT; ++y)
			{
				TerrainChunk* chunk = &group->chunks[y];
				unsigned short faceCount = (chunk->flags & 0xffff0000) >> 16;

				if(faceCount > 0)
				{
					data.chunk[0] = (int)group->id[0];
					data.chunk[1] = (int)y;
					data.chunk[2] = (int)group->id[1];
					data.chunkId = group->ssboId * TERRAIN_HEIGHT + y;

					if(chunk->flags & 1)
					{
						PassTerrainDataToShader(&data);
						cm_draw_instanced_vao(voxelTerrain.quadVao, CM_TRIANGLES, faceCount);
					}
				}
			}
		}
	}

	cm_end_shader_mode();
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

	cm_unload_ssbo(voxelTerrain.ssbo);
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

	voxelTerrain.textures[0] = cm_load_texture(mapPath0, CM_TEXTURE_WRAP_CLAMP_TO_EDGE, CM_TEXTURE_FILTER_NEAREST, false);
	voxelTerrain.textures[1] = cm_load_texture(mapPath1, CM_TEXTURE_WRAP_CLAMP_TO_EDGE, CM_TEXTURE_FILTER_NEAREST, false);
	voxelTerrain.textures[2] = cm_load_texture(mapPath2, CM_TEXTURE_WRAP_CLAMP_TO_EDGE, CM_TEXTURE_FILTER_NEAREST, false);
}

static void LoadBuffers()
{
	voxelTerrain.quadVao = cm_get_unit_quad();
	voxelTerrain.ssbo = cm_load_ssbo(TERRAIN_SSBO_BINDING, sizeof(unsigned int) * TERRAIN_CHUNK_CUBE_COUNT * TERRAIN_CHUNK_COUNT, NULL);
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
	for (unsigned int x = 0; x < TERRAIN_VIEW_RANGE; ++x)
		for (unsigned int z = 0; z < TERRAIN_VIEW_RANGE; ++z)
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

static TerrainChunkGroup InitializeChunkGroup(unsigned int ssboId)
{
	TerrainChunkGroup group =
		{
			.state = CHUNK_GROUP_REQUIRES_NOISE_MAP,
			.id = { 0, 0 },
			.isAlive = true,
			.heightMap = CM_MALLOC(CHUNK_HORIZONTAL_SLICE),
			.ssboId = ssboId
		};

	for (int y = 0; y < TERRAIN_HEIGHT; ++y)
	{
		TerrainChunk chunk =
			{
				.flags = 0,
				.buffer = NULL,
				.cells = CM_MALLOC(TERRAIN_CHUNK_CUBE_COUNT),
				.state = CHUNK_REQUIRES_FACES,
				.yId = y,
			};

		group.chunks[y] = chunk;
	}

	return group;
}

static void RecreateChunkGroup(TerrainChunkGroup* group, unsigned int x, unsigned int z)
{
	UnloadChunkGroup(group);
	group->id[0] = x;
	group->id[1] = z;
}

static void UnloadChunkGroup(TerrainChunkGroup* group)
{
	group->state = CHUNK_GROUP_REQUIRES_NOISE_MAP;
	memset(group->heightMap, 0, CHUNK_HORIZONTAL_SLICE);
	for (int y = 0; y < TERRAIN_HEIGHT; ++y)
	{
		TerrainChunk* chunk = &group->chunks[y];
		chunk->state = CHUNK_REQUIRES_FACES;
		unsigned char bufferSize = (chunk->flags & 0b0111) >> 1;
		chunk->flags &= 0;
		chunk->flags |= bufferSize << 1;

		if(chunk->buffer) memset(chunk->buffer, 0, BUFFER_SIZE_STAGES[bufferSize] * sizeof(unsigned int));
		memset(chunk->cells, 0, TERRAIN_CHUNK_CUBE_COUNT);
	}
}

static void DestroyChunkGroup(TerrainChunkGroup* group)
{
	if(!group->isAlive) return;
	group->isAlive = false;
	CM_FREE(group->heightMap);

	for (int y = 0; y < TERRAIN_HEIGHT; ++y)
	{
		CM_FREE(group->chunks[y].cells);
		if(group->chunks[y].buffer) CM_FREE(group->chunks[y].buffer);
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
	glm_vec3(camera.position, position);
	ivec2 id;
	id[0] = (int)(position[0] / TERRAIN_CHUNK_SIZE) + TERRAIN_WORLD_EDGE;
	id[1] = (int)(position[2] / TERRAIN_CHUNK_SIZE) + TERRAIN_WORLD_EDGE;

	glm_ivec3(id, voxelTerrain.loadedCenter);

	for (int x = 0; x < TERRAIN_VIEW_RANGE; ++x)
		for (int z = 0; z < TERRAIN_VIEW_RANGE; ++z)
			RecreateGroup(id, x, z);
}

static void SetRequiresFaces(unsigned int x, unsigned int z)
{
	TerrainChunkGroup* group = &voxelTerrain.chunkGroups[x * TERRAIN_VIEW_RANGE + z];

	for (unsigned int y = 0; y < TERRAIN_HEIGHT; ++y)
		group->chunks[y].state = CHUNK_REQUIRES_FACES;
}

static void ReloadChunks(Camera3D camera)
{
	if(voxelTerrain.pool->jobCount > 0) return;

	vec3 position;
	glm_vec3(camera.position, position);
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

			for (unsigned int x = TERRAIN_VIEW_RANGE - dataShift[0] - 1; x < TERRAIN_VIEW_RANGE - dataShift[0]; ++x)
				for (unsigned int z = 0; z < TERRAIN_VIEW_RANGE; ++z)
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

static void SendNoiseJob(unsigned int x, unsigned int z)
{
	TerrainChunkGroup* group = &voxelTerrain.chunkGroups[x * TERRAIN_VIEW_RANGE + z];
	group->state = CHUNK_GROUP_GENERATING_NOISE_MAP;

	unsigned int* args = CM_MALLOC(4 * sizeof(unsigned int));
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
	unsigned int* cArgs = (unsigned int*)args;
	GenerateHeightMap((unsigned int[2]){cArgs[0], cArgs[1]}, (unsigned int[2]){cArgs[2], cArgs[3]});
	for (int y = 0; y < TERRAIN_HEIGHT; ++y)
	{
		GeneratePreChunk(cArgs[2], y, cArgs[3]);
		GeneratePostChunk(cArgs[2], y, cArgs[3]);
	}
}

static void T_OnNoiseGenerationFinished(void* args)
{
	unsigned int* cArgs = (unsigned int*)args;

	TerrainChunkGroup* group = &voxelTerrain.chunkGroups[cArgs[2] * TERRAIN_VIEW_RANGE + cArgs[3]];
	group->state = CHUNK_GROUP_READY;
}

static void GenerateHeightMap(const unsigned int sourceId[2], const unsigned int destination[2])
{
	unsigned char* heightMap = voxelTerrain.chunkGroups[destination[0] * TERRAIN_VIEW_RANGE + destination[1]].heightMap;
	fnl_state noise = voxelTerrain.biomes[BIOME_HILL];

	for (unsigned int x = 0; x < TERRAIN_CHUNK_SIZE; ++x)
	{
		unsigned int px = sourceId[0] * TERRAIN_CHUNK_SIZE + x;
		for (unsigned int z = 0; z < TERRAIN_CHUNK_SIZE; ++z)
		{
			unsigned int pz = sourceId[1] * TERRAIN_CHUNK_SIZE + z;

			float val2D = fnlGetNoise2D(&noise, (double)(px), (double)(pz));
			val2D = (val2D + 1) * .5f;
			unsigned char height = (TERRAIN_LOWER_EDGE * TERRAIN_CHUNK_SIZE) +
			                       (unsigned char)(val2D * (TERRAIN_CHUNK_SIZE * (TERRAIN_UPPER_EDGE - TERRAIN_LOWER_EDGE) - 1));

			heightMap[x * TERRAIN_CHUNK_SIZE + z] = height;
		}
	}
}

static void GeneratePreChunk(unsigned int xId, unsigned int yId, unsigned int zId)
{
	TerrainChunkGroup group = voxelTerrain.chunkGroups[xId * TERRAIN_VIEW_RANGE + zId];
	unsigned char* heightMap = group.heightMap;
	TerrainChunk chunk = group.chunks[yId];
	memset(chunk.cells, 0, TERRAIN_CHUNK_CUBE_COUNT);

	for (unsigned int x = 0; x < TERRAIN_CHUNK_SIZE; ++x)
	{
		unsigned int px = group.id[0] * TERRAIN_CHUNK_SIZE + x;

		for (unsigned int z = 0; z < TERRAIN_CHUNK_SIZE; ++z)
		{
			unsigned int xzId = x * TERRAIN_CHUNK_SIZE + z;
			unsigned int pz = group.id[1] * TERRAIN_CHUNK_SIZE + z;
			int maxY = (int)heightMap[x * TERRAIN_CHUNK_SIZE + z] - (int)(yId * TERRAIN_CHUNK_SIZE);
			int yLimit = glm_imin((int)(TERRAIN_CHUNK_SIZE - 1), maxY);
			if(yLimit < 0) continue;

			for (unsigned int y = 0; y <= yLimit; ++y)
			{
				unsigned int py = yId * TERRAIN_CHUNK_SIZE + y;
				float caveValue = fnlGetNoise3D(&voxelTerrain.caveNoise, (double)(px), (double)(py), (double)(pz));

				if(caveValue >= TERRAIN_CAVE_EDGE)
				{
					caveValue = (caveValue - TERRAIN_CAVE_EDGE) / (1 - TERRAIN_CAVE_EDGE);

					unsigned int id = y * CHUNK_HORIZONTAL_SLICE + xzId;
					chunk.cells[id] = get_block_type(caveValue);
				}
			}
		}
	}

#undef SET_CHUNK
}

static void GeneratePostChunk(unsigned int xId, unsigned int yId, unsigned int zId)
{
	unsigned char* heightMap = voxelTerrain.chunkGroups[xId * TERRAIN_VIEW_RANGE + zId].heightMap;
	TerrainChunk chunk = voxelTerrain.chunkGroups[xId * TERRAIN_VIEW_RANGE + zId].chunks[yId];
	for (unsigned int x = 0; x < TERRAIN_CHUNK_SIZE; ++x)
	{
		for (unsigned int z = 0; z < TERRAIN_CHUNK_SIZE; ++z)
		{
			unsigned int xzId = x * TERRAIN_CHUNK_SIZE + z;
			int maxY = (int)heightMap[x * TERRAIN_CHUNK_SIZE + z] - (int)(yId * TERRAIN_CHUNK_SIZE);
			int yLimit = glm_imin((int)(TERRAIN_CHUNK_SIZE - 1), maxY);
			if(yLimit < 0) continue;

			for (unsigned int y = 0; y <= yLimit; ++y)
			{
				unsigned int id = y * CHUNK_HORIZONTAL_SLICE + xzId;
				if(chunk.cells[id] == BLOCK_EMPTY) continue;

				if(y == maxY) chunk.cells[id] = BLOCK_GRASS;
				else if(maxY - y < 3) chunk.cells[id] = BLOCK_DIRT;
			}
		}
	}
}

//endregion

//region Face Creation

static void SendFaceCreationJob(unsigned int x, unsigned int y, unsigned int z)
{
	TerrainChunk* chunk = &voxelTerrain.chunkGroups[x * TERRAIN_VIEW_RANGE + z].chunks[y];
	chunk->state = CHUNK_CREATING_FACES;
	
	unsigned int* args = CM_MALLOC(3 * sizeof(unsigned int));
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
	unsigned int* cArgs = (unsigned int*)args;
	CreateChunkFaces(cArgs[0], cArgs[1], cArgs[2]);
}

static void T_ChunkFacesCreationFinished(void* args)
{
	unsigned int* cArgs = (unsigned int*)args;
	voxelTerrain.chunkGroups[cArgs[0] * TERRAIN_VIEW_RANGE + cArgs[2]].chunks[cArgs[1]].state = CHUNK_REQUIRES_UPLOAD;
}

static void SendGroupFaceCreationJob(unsigned int x, unsigned int z)
{
	unsigned int* args = CM_MALLOC(2 * sizeof(unsigned int));
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
	unsigned int* cArgs = (unsigned int*)args;
	
	for (int y = 0; y < TERRAIN_HEIGHT; ++y)
		CreateChunkFaces(cArgs[0], y, cArgs[1]);
}

static void T_GroupFacesCreationFinished(void* args)
{
	unsigned int* cArgs = (unsigned int*)args;
	
	TerrainChunkGroup* group = &voxelTerrain.chunkGroups[cArgs[0] * TERRAIN_VIEW_RANGE + cArgs[1]];
	
	for (int y = 0; y < TERRAIN_HEIGHT; ++y)
		group->chunks[y].state = CHUNK_REQUIRES_UPLOAD;
}

static void CreateChunkFaces(unsigned int xId, unsigned int yId, unsigned int zId)
{
	unsigned int faceCount = 0;
	TerrainChunkGroup* group = &voxelTerrain.chunkGroups[xId * TERRAIN_VIEW_RANGE + zId];
	TerrainChunk* chunk = &group->chunks[yId];
	unsigned int* buffer = chunk->buffer;
	unsigned char* cells = chunk->cells;

	unsigned char* frontChunkCells = NULL;
	unsigned char* backChunkCells = NULL;
	unsigned char* rightChunkCells = NULL;
	unsigned char* leftChunkCells = NULL;
	unsigned char* topChunkCells = NULL;
	unsigned char* bottomChunkCells = NULL;

	if(zId < TERRAIN_VIEW_RANGE - 1) frontChunkCells = voxelTerrain.chunkGroups[xId * TERRAIN_VIEW_RANGE + zId + 1].chunks[yId].cells;
	if(zId > 0) backChunkCells = voxelTerrain.chunkGroups[xId * TERRAIN_VIEW_RANGE + zId - 1].chunks[yId].cells;
	if(xId < TERRAIN_VIEW_RANGE - 1) rightChunkCells = voxelTerrain.chunkGroups[(xId + 1) * TERRAIN_VIEW_RANGE + zId].chunks[yId].cells;
	if(xId > 0) leftChunkCells = voxelTerrain.chunkGroups[(xId - 1) * TERRAIN_VIEW_RANGE + zId].chunks[yId].cells;
	if(yId < TERRAIN_HEIGHT - 1) topChunkCells = group->chunks[yId + 1].cells;
	if(yId > 0) bottomChunkCells = group->chunks[yId - 1].cells;

	unsigned char bufferSizeIndex = (chunk->flags & 0b1110) >> 1;
	unsigned int bufferSize = BUFFER_SIZE_STAGES[bufferSizeIndex];

	for (unsigned int y = 0; y < TERRAIN_CHUNK_SIZE; y++)
	{
		for (unsigned int x = 0; x < TERRAIN_CHUNK_SIZE; x++)
		{
			for (unsigned int z = 0; z < TERRAIN_CHUNK_SIZE; z++)
			{
				unsigned int cubeId = y * CHUNK_HORIZONTAL_SLICE + x * TERRAIN_CHUNK_SIZE + z;
				unsigned char currentCell = cells[cubeId];
				if(currentCell == BLOCK_EMPTY) continue;

				unsigned char faceMask = 0;

				//region define faces
				//Front
				faceMask |= (((z == TERRAIN_CHUNK_SIZE - 1) && (frontChunkCells != NULL && frontChunkCells[y * CHUNK_HORIZONTAL_SLICE + x * TERRAIN_CHUNK_SIZE] == BLOCK_EMPTY)) ||
					((z < TERRAIN_CHUNK_SIZE - 1) && cells[cubeId + 1] == BLOCK_EMPTY)) << 5;

				//Back
				faceMask |= (((z == 0) && (backChunkCells != NULL && backChunkCells[y * CHUNK_HORIZONTAL_SLICE + x * TERRAIN_CHUNK_SIZE + TERRAIN_CHUNK_SIZE - 1] == BLOCK_EMPTY)) ||
				             ((z > 0) && cells[cubeId - 1] == BLOCK_EMPTY)) << 4;

				//Right
				faceMask |= (((x == TERRAIN_CHUNK_SIZE - 1) && (rightChunkCells != NULL && rightChunkCells[y * CHUNK_HORIZONTAL_SLICE + z] == BLOCK_EMPTY)) ||
				             ((x < TERRAIN_CHUNK_SIZE - 1) && (cells[cubeId + TERRAIN_CHUNK_SIZE] == BLOCK_EMPTY))) << 3;

				//Left
				faceMask |= (((x == 0) && (leftChunkCells != NULL && leftChunkCells[y * CHUNK_HORIZONTAL_SLICE + (TERRAIN_CHUNK_SIZE - 1) * TERRAIN_CHUNK_SIZE + z] == BLOCK_EMPTY)) ||
				             ((x > 0) && (cells[cubeId - TERRAIN_CHUNK_SIZE] == BLOCK_EMPTY))) << 2;

				//Top
				faceMask |= (((y == TERRAIN_CHUNK_SIZE - 1) && (topChunkCells != NULL && topChunkCells[x * TERRAIN_CHUNK_SIZE + z] == BLOCK_EMPTY)) ||
				             ((y < TERRAIN_CHUNK_SIZE - 1) && (cells[cubeId + CHUNK_HORIZONTAL_SLICE] == BLOCK_EMPTY))) << 1;

				//Bottom
				faceMask |= (((y == 0) && (bottomChunkCells != NULL && bottomChunkCells[(TERRAIN_CHUNK_SIZE - 1) * CHUNK_HORIZONTAL_SLICE + x * TERRAIN_CHUNK_SIZE + z] == BLOCK_EMPTY)) ||
				             ((y > 0) && (cells[cubeId - CHUNK_HORIZONTAL_SLICE] == BLOCK_EMPTY)));

				//endregion

				if(faceMask == 0) continue;

				if(bufferSize < faceCount + 6)
				{
					unsigned int oldBufferSize = bufferSize;
					bufferSizeIndex++;
					bufferSize = BUFFER_SIZE_STAGES[bufferSizeIndex];
					void* newMemory = CM_REALLOC(chunk->buffer, bufferSize * sizeof(unsigned int));

					if(newMemory == NULL)
					{
						perror("Unable to reallocate memory!!! exiting the program.\n");
						exit(-1);
					}
					else
					{
						chunk->buffer = newMemory;
						buffer = newMemory;
						memset(&newMemory[oldBufferSize * sizeof(unsigned int)], 0, (bufferSize - oldBufferSize) * sizeof(unsigned int));
						chunk->flags &= 0xfffffff1;
						chunk->flags |= bufferSizeIndex << 1;
					}
				}

				currentCell--;
				unsigned int blockIndex = (currentCell / TERRAIN_MAX_AXIS_BLOCK_TYPES) << 4 |
				                          (currentCell % TERRAIN_MAX_AXIS_BLOCK_TYPES);

				blockIndex <<= 15;
				blockIndex |= (x << 10) | (y << 5) | z;
				blockIndex <<= 3;

				for (int i = 0; i < 6; i++)
				{
					buffer[faceCount] = blockIndex | (5 - i);
					faceCount += (faceMask & (1 << i)) >> i;
				}
			}
		}
	}

#undef RECT_FACE
	chunk->flags &= 0x0000ffff;
	chunk->flags |= faceCount << 16;
}

//endregion

//region Utils

static void PassTerrainDataToShader(UniformData* data)
{
	unsigned int index[4];
	memcpy_s(index, sizeof(index), data->chunk, sizeof(data->chunk));
	index[3] = data->chunkId;
	cm_set_uniform_uvec4(voxelTerrain.u_chunkIndex, index);
}

static bool SurroundGroupsAreLoaded(unsigned int xId, unsigned int zId)
{
	bool areLoaded = voxelTerrain.chunkGroups[xId * TERRAIN_VIEW_RANGE + zId].state == CHUNK_GROUP_READY;
	areLoaded = areLoaded && (xId == 0 || voxelTerrain.chunkGroups[(xId - 1) * TERRAIN_VIEW_RANGE + zId].state == CHUNK_GROUP_READY);
	areLoaded = areLoaded && (xId == (TERRAIN_VIEW_RANGE - 1) || voxelTerrain.chunkGroups[(xId + 1) * TERRAIN_VIEW_RANGE + zId].state == CHUNK_GROUP_READY);

	areLoaded = areLoaded && (zId == 0 || voxelTerrain.chunkGroups[xId * TERRAIN_VIEW_RANGE + zId - 1].state == CHUNK_GROUP_READY);
	areLoaded = areLoaded && (zId == (TERRAIN_VIEW_RANGE - 1) || voxelTerrain.chunkGroups[xId * TERRAIN_VIEW_RANGE + zId + 1].state == CHUNK_GROUP_READY);

	return areLoaded;
}

static bool GroupNeedsFaces(unsigned int xId, unsigned int zId)
{
	TerrainChunkGroup group = voxelTerrain.chunkGroups[xId * TERRAIN_VIEW_RANGE + zId];
	bool needsFaces = true;
	for (int y = 0; y < TERRAIN_HEIGHT; ++y)
		needsFaces = needsFaces && group.chunks[y].state == CHUNK_REQUIRES_FACES;
	return needsFaces;
}

static bool DelayedLoader()
{
	unsigned int start = TERRAIN_VIEW_RANGE / 2 - TERRAIN_LOADING_EDGE, end = start + TERRAIN_LOADING_EDGE * 2;
	
	for (unsigned int x = start; x < end; ++x)
	{
		for (unsigned int z = start; z < end; ++z)
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
		unsigned short faceCount = (chunk->flags & 0xffff0000) >> 16;
		if(faceCount == 0) continue;

		if(chunk->state == CHUNK_REQUIRES_UPLOAD)
		{
			unsigned int id = group->ssboId * TERRAIN_HEIGHT + y;
			cm_upload_ssbo(voxelTerrain.ssbo, id * TERRAIN_CHUNK_CUBE_COUNT * sizeof(unsigned int),
			               faceCount * sizeof(unsigned int), chunk->buffer);
			chunk->state = CHUNK_READY_TO_DRAW;
			chunk->flags |= 1;
			uploaded = true;
		}
	}
	return uploaded;
}

//endregion