#include "terrain.h"
#include "config.h"
#include "coal_miner_internal.h"
#include <FastNoiseLite.h>

#define TERRAIN_CHUNK_CUBE_COUNT TERRAIN_CHUNK_SIZE * TERRAIN_CHUNK_SIZE * TERRAIN_CHUNK_SIZE

typedef struct VoxelBuffer
{
	unsigned int voxelFaces[TERRAIN_CHUNK_CUBE_COUNT];
}VoxelBuffer;

typedef struct TerrainData
{
	int xSeed, zSeed;
	unsigned int chunkCubeCount;
	unsigned int chunkHorizontalSlice;
	unsigned int faceCount;
}TerrainData;

typedef struct Chunk
{
	unsigned char cells[TERRAIN_CHUNK_CUBE_COUNT];
	unsigned int vertices[TERRAIN_CHUNK_CUBE_COUNT * 4];
	unsigned int indices[TERRAIN_CHUNK_CUBE_COUNT * 6];
	bool chunkChanged;
}Chunk;

Shader terrainShader;
Vao quadVao;
Ssbo terrainSsbo;

TerrainData terrainData = { 0 };
Chunk terrainChunk;
VoxelBuffer voxelBuffer;
bool terrainIsWireMode;

static void CreateShader();
static void CreateChunkCells(Chunk* chunk);
static unsigned int CreateChunkFaces(Chunk* chunk);

//region Callback Functions
void load_terrain()
{
	terrainData.xSeed = rand() % 10000000;
	terrainData.zSeed = rand() % 10000000;
	terrainData.chunkHorizontalSlice = TERRAIN_CHUNK_SIZE * TERRAIN_CHUNK_SIZE;
	terrainData.chunkCubeCount = terrainData.chunkHorizontalSlice * TERRAIN_CHUNK_SIZE;
	CreateShader();
	CreateChunkCells(&terrainChunk);
	
	terrainData.faceCount = CreateChunkFaces(&terrainChunk);
	quadVao = cm_get_unit_quad();
	terrainSsbo = cm_load_ssbo(64, sizeof(VoxelBuffer), &voxelBuffer, false);
}

void update_terrain()
{
	if(cm_is_key_pressed(KEY_B))
	{
		if(terrainIsWireMode) cm_disable_wire_mode();
		else cm_enable_wire_mode();

		terrainIsWireMode = !terrainIsWireMode;
	}

	printf("FrameTime: %f\n", cm_frame_time() * 1000);
}

void draw_terrain()
{
	cm_begin_shader_mode(terrainShader);

	cm_draw_instanced_vao(quadVao, CM_TRIANGLES, terrainData.faceCount);

	cm_end_shader_mode();
}

void dispose_terrain()
{
	cm_unload_ssbo(terrainSsbo);
	cm_unload_shader(terrainShader);
}
//endregion

//region Local Functions

static void CreateShader()
{
	Path vsPath = TO_RES_PATH(vsPath, "shaders/voxel_terrain.vert");
	Path fsPath = TO_RES_PATH(fsPath, "shaders/voxel_terrain.frag");

	terrainShader = cm_load_shader(vsPath, fsPath);
}

static unsigned int CreateChunkFaces(Chunk* chunk)
{
	unsigned int faceCount = 0;

	for (unsigned int y = 0; y < TERRAIN_CHUNK_SIZE; y++)
	{
		for (unsigned int x = 0; x < TERRAIN_CHUNK_SIZE; x++)
		{
			for (unsigned int z = 0; z < TERRAIN_CHUNK_SIZE; z++)
			{
				unsigned int cubeId = y * terrainData.chunkHorizontalSlice + x * TERRAIN_CHUNK_SIZE + z;
				if(chunk->cells[cubeId] == 0) continue;

				unsigned int temp = 0;
				unsigned char faceMask = 0;

				//region define faces
				//front
				temp = (z == TERRAIN_CHUNK_SIZE - 1 || chunk->cells[cubeId + 1] == 0);
				faceMask |= temp << 5;

				//back
				temp = (z == 0 || chunk->cells[cubeId - 1] == 0);
				faceMask |= temp << 4;

				//right
				temp = (x == TERRAIN_CHUNK_SIZE - 1 || chunk->cells[cubeId + TERRAIN_CHUNK_SIZE] == 0);
				faceMask |= temp << 3;

				//left
				temp = (x == 0 || chunk->cells[cubeId - TERRAIN_CHUNK_SIZE] == 0);
				faceMask |= temp << 2;

				//top
				temp = (y == TERRAIN_CHUNK_SIZE - 1 || chunk->cells[cubeId + terrainData.chunkHorizontalSlice] == 0);
				faceMask |= temp << 1;

				//bottom
				temp = (y == 0 || chunk->cells[cubeId - terrainData.chunkHorizontalSlice] == 0);
				faceMask |= temp;
				//endregion

				if(faceMask == 0) continue;

				unsigned int blockIndex = (x << 10) | (y << 5) | z;
				blockIndex <<= 3;

				//front face
				if((faceMask & 0b100000) > 0)
				{
					voxelBuffer.voxelFaces[faceCount] = blockIndex | 0;
					faceCount++;
				}

				//back face
				if((faceMask & 0b010000) > 0)
				{
					voxelBuffer.voxelFaces[faceCount] = blockIndex | 1;
					faceCount++;
				}

				//right face
				if((faceMask & 0b001000) > 0)
				{
					voxelBuffer.voxelFaces[faceCount] = blockIndex | 2;
					faceCount++;
				}

				//left face
				if((faceMask & 0b000100) > 0)
				{
					voxelBuffer.voxelFaces[faceCount] = blockIndex | 3;
					faceCount++;
				}

				//top face
				if((faceMask & 0b000010) > 0)
				{
					voxelBuffer.voxelFaces[faceCount] = blockIndex | 4;
					faceCount++;
				}

				if((faceMask & 0b000001) > 0)
				{
					voxelBuffer.voxelFaces[faceCount] = blockIndex | 5;
					faceCount++;
				}
			}
		}
	}

#undef RECT_FACE

	printf("allSpace: %i, faces %i\n", TERRAIN_CHUNK_CUBE_COUNT, faceCount);
}

static void CreateChunkCells(Chunk* chunk)
{
	fnl_state noise = fnlCreateState();
	noise.noise_type = FNL_NOISE_PERLIN;

	unsigned int minHeight = (TERRAIN_CHUNK_SIZE - TERRAIN_RANDOMNESS_AREA) / 2;

	for (unsigned int x = 0; x < TERRAIN_CHUNK_SIZE; ++x)
	{
		for (unsigned int z = 0; z < TERRAIN_CHUNK_SIZE; ++z)
		{
			float nVal = (fnlGetNoise2D(&noise, (float)(terrainData.xSeed + x), (float)(terrainData.zSeed + z)) + 1) * .5f;
			unsigned int xzId = x * TERRAIN_CHUNK_SIZE + z;
			unsigned int till = (unsigned int)(minHeight + (unsigned int)(nVal * TERRAIN_RANDOMNESS_AREA));

			for (unsigned int y = 0; y < till; ++y)
				chunk->cells[y * terrainData.chunkHorizontalSlice + xzId] = 1;

			for (unsigned int y = till; y < TERRAIN_CHUNK_SIZE; ++y)
				chunk->cells[y * terrainData.chunkHorizontalSlice + xzId] = 0;

//			for (unsigned int y = 0; y < TERRAIN_CHUNK_SIZE; ++y)
//				chunk->cells[y * terrainData.chunkHorizontalSlice + xzId] = x % 2 > 0 || y % 2 > 0 || z % 2 > 0;
		}
	}
}

//endregion