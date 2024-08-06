#include "terrain_noise.h"
#include "terrain_blocks.h"

static void T_GenerateTerrainNoise(void* args);
static void T_OnTerrainNoiseGenerationFinished(void* args);

VoxelTerrain* n_terrain;

void setup_terrain_noise(VoxelTerrain* terrain)
{
	n_terrain = terrain;
}

void send_terrain_noise_job(uint32_t x, uint32_t z)
{
	TerrainChunkGroup* group = &n_terrain->chunkGroups[x * TERRAIN_VIEW_RANGE + z];
	group->state = CHUNK_GROUP_GENERATING_NOISE_MAP;

	uint32_t * args = CM_MALLOC(4 * sizeof(uint32_t ));
	args[0] = group->id[0];
	args[1] = group->id[1];
	args[2] = x;
	args[3] = z;
	ThreadJob job = {0};
	job.args = args;
	job.job = T_GenerateTerrainNoise;
	job.callbackJob = T_OnTerrainNoiseGenerationFinished;
	cm_submit_job(n_terrain->pool, job, false);
}

static void T_GenerateTerrainNoise(void* args)
{
	uint32_t * cArgs = (uint32_t *)args;
	generate_terrain_height_map((uint32_t[2]) {cArgs[0], cArgs[1]}, (uint32_t[2]) {cArgs[2], cArgs[3]});
	for (int y = 0; y < TERRAIN_HEIGHT; ++y)
	{
		generate_terrain_pre_chunk(cArgs[2], y, cArgs[3]);
		generate_terrain_post_chunk(cArgs[2], y, cArgs[3]);
	}
}

void T_OnTerrainNoiseGenerationFinished(void* args)
{
	uint32_t * cArgs = (uint32_t *)args;

	TerrainChunkGroup* group = &n_terrain->chunkGroups[cArgs[2] * TERRAIN_VIEW_RANGE + cArgs[3]];
	group->state = CHUNK_GROUP_READY;
}

void generate_terrain_height_map(const uint32_t sourceId[2], const uint32_t destination[2])
{
	uint8_t * heightMap = n_terrain->chunkGroups[destination[0] * TERRAIN_VIEW_RANGE + destination[1]].heightMap;
	fnl_state noise = n_terrain->biomes[BIOME_FLAT];

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

void generate_terrain_pre_chunk(uint32_t xId, uint32_t yId, uint32_t zId)
{
	TerrainChunkGroup* group = &n_terrain->chunkGroups[xId * TERRAIN_VIEW_RANGE + zId];
	uint8_t* heightMap = group->heightMap;
	uint8_t* cells = group->chunks[yId].voxels;
	uint32_t groupId[2] = { group->id[0], group->id[1] };
	fnl_state* caveNoise = &n_terrain->caveNoise;

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
				float caveValue = fnlGetNoise3D(caveNoise, (double)(px), (double)(py), (double)(pz));

				if(caveValue >= TERRAIN_CAVE_EDGE)
				{
					caveValue = (caveValue - TERRAIN_CAVE_EDGE) / (1 - TERRAIN_CAVE_EDGE);

					uint32_t id = y * TERRAIN_CHUNK_HORIZONTAL_SLICE + xzId;
					uint8_t block = get_terrain_block_type(caveValue);
					cells[id] = block;
				}
			}
		}
	}
}

void generate_terrain_post_chunk(uint32_t xId, uint32_t yId, uint32_t zId)
{
	TerrainChunkGroup* group = &n_terrain->chunkGroups[xId * TERRAIN_VIEW_RANGE + zId];
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