#include "terrainStructs.h"
#include "terrain_noise.h"
#include "terrain_meshing.h"
#include "terrain_blocks.h"
#include "coal_miner_internal.h"
#include "camera.h"
#include "coal_helper.h"

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

//Reloading
static void SetupInitialChunks(Camera3D camera);
static void ReloadChunks(Camera3D camera);

//Utils
static void PassTerrainDataToShader(UniformData* data);
static bool AllChunksAreLoaded();
static bool SurroundGroupsAreLoaded(uint32_t xId, uint32_t zId);
static bool GroupNeedsFaces(uint32_t xId, uint32_t zId);
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
	InitTerrainNoise();
	
	voxelTerrain.shiftGroups = CM_MALLOC(TERRAIN_VIEW_RANGE * sizeof(TerrainChunkGroup));
	setup_terrain_noise(&voxelTerrain);
	setup_terrain_meshing(&voxelTerrain);

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

	if(cm_is_key_pressed(KEY_T))
	{
		uint32_t size = 0;
		for (uint32_t i = 0; i < TERRAIN_VIEW_RANGE * TERRAIN_VIEW_RANGE; ++i)
		{
			for (uint32_t y = 0; y < TERRAIN_HEIGHT; ++y)
				size += voxelTerrain.chunkGroups[i].chunks[y].buffer.size;
		}

		log_info("Allocated Buffer Bytes: %u\n", size);
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
				uint16_t faceCount = chunk->flags.faceCount;

				if(faceCount > 0)
				{
					volume.center[1] = (float)y * TERRAIN_CHUNK_SIZE + TERRAIN_CHUNK_SIZE * .5f;
					if(!cm_is_in_main_frustum(&volume)) continue;

					data.chunk[0] = (int)group->id[0];
					data.chunk[1] = (int)y;
					data.chunk[2] = (int)group->id[1];

					data.chunkId = group->ssboId * TERRAIN_HEIGHT + y;

					if(chunk->flags.isUploaded)
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
			{ .size = 2, .type = CM_UINT, .normalized = false, .stride = 2 * sizeof(uint32_t) },
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
	voxelTerrain.caveNoise = get_terrain_cave_noise();
	voxelTerrain.biomes[BIOME_FLAT] = get_terrain_biome_flat_noise();
	voxelTerrain.biomes[BIOME_SMALL_HILL] = get_terrain_biome_small_hill_noise();
	voxelTerrain.biomes[BIOME_HILL] = get_terrain_biome_hill_noise();
	voxelTerrain.biomes[BIOME_MOUNTAIN] = get_terrain_biome_mountain_noise();
	voxelTerrain.biomes[BIOME_HIGH_MOUNTAIN] = get_terrain_biome_high_mountain_noise();
}

static void LoadTerrainChunks()
{
	for (uint32_t x = 0; x < TERRAIN_VIEW_RANGE; ++x)
		for (uint32_t z = 0; z < TERRAIN_VIEW_RANGE; ++z)
		{
			if(SurroundGroupsAreLoaded(x, z))
			{
				if(GroupNeedsFaces(x, z)) send_terrain_group_face_creation_job(x, z);
				else
				{
					TerrainChunkGroup group = voxelTerrain.chunkGroups[x * TERRAIN_VIEW_RANGE + z];
					for (int y = 0; y < TERRAIN_HEIGHT; ++y)
						if(group.chunks[y].flags.state == CHUNK_REQUIRES_FACES)
							send_terrain_face_creation_job(x, y, z);
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
				.buffer = list_create(0),
				.voxels = CM_MALLOC(TERRAIN_CHUNK_VOXEL_COUNT),
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
		chunk->flags.state = CHUNK_REQUIRES_FACES;
		list_clear(&chunk->buffer);

		chunk->flags.isUploaded = 0;
		chunk->flags.faceCount = 0;

		memset(chunk->voxels, 0, TERRAIN_CHUNK_VOXEL_COUNT);
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
		list_clear(&group->chunks[y].buffer);
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
	send_terrain_noise_job(x, z);
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
		group->chunks[y].flags.state = CHUNK_REQUIRES_FACES;
}

static void ReloadChunks(Camera3D camera)
{
	if(voxelTerrain.pool->jobCount > 0 || !AllChunksAreLoaded()) return;

	vec3 position = { 0 };
	glm_vec3_copy(camera.position, position);
	ivec2 id;
	id[0] = (int32_t)glm_clamp((position[0] / TERRAIN_CHUNK_SIZE) + TERRAIN_WORLD_EDGE,
							   (float)(voxelTerrain.loadedCenter[0] - 1), (float)(voxelTerrain.loadedCenter[0] + 1));
	id[1] = (int32_t)glm_clamp((position[2] / TERRAIN_CHUNK_SIZE) + TERRAIN_WORLD_EDGE,
							   (float)(voxelTerrain.loadedCenter[1] - 1), (float)(voxelTerrain.loadedCenter[1] + 1));

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

//region Utils

static void PassTerrainDataToShader(UniformData* data)
{
	uint32_t index[4];
	memcpy(index, data->chunk, sizeof(ivec3));
	index[3] = data->chunkId;
	cm_set_uniform_uvec4(voxelTerrain.u_chunkIndex, index);
}

static bool AllChunksAreLoaded()
{
	for (uint32_t i = 0; i < TERRAIN_VIEW_RANGE * TERRAIN_VIEW_RANGE; ++i)
	{
		if(voxelTerrain.chunkGroups[i].state == CHUNK_GROUP_READY)
		{
			for (uint32_t y = 0; y < TERRAIN_HEIGHT; ++y)
			{
				if(voxelTerrain.chunkGroups[i].chunks[y].flags.state < CHUNK_REQUIRES_UPLOAD)
					return false;
			}
		}
		else return false;
	}

	return true;
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
		needsFaces = needsFaces && group.chunks[y].flags.state == CHUNK_REQUIRES_FACES;
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
		uint16_t faceCount = chunk->flags.faceCount;
		if(faceCount == 0) continue;

		if(chunk->flags.state == CHUNK_REQUIRES_UPLOAD)
		{
			uint32_t id = group->ssboId * TERRAIN_HEIGHT + y;
			cm_reupload_vbo(&voxelTerrain.chunkVaos[id].vbo, chunk->buffer.position, chunk->buffer.data);
			cm_upload_ssbo(voxelTerrain.voxelsSsbo, id * TERRAIN_CHUNK_VOXEL_COUNT, TERRAIN_CHUNK_VOXEL_COUNT, chunk->voxels);
			chunk->flags.state = CHUNK_READY_TO_DRAW;
			chunk->flags.isUploaded = true;
			uploaded = true;
		}
	}
	return uploaded;
}

//endregion