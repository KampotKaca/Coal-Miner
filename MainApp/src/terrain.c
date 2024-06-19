#include "terrain.h"
#include "config.h"
#include "coal_miner_internal.h"
#include <FastNoiseLite.h>

#define TERRAIN_CHUNK_CUBE_COUNT TERRAIN_CHUNK_SIZE * TERRAIN_CHUNK_SIZE * TERRAIN_HEIGHT

typedef struct TerrainData
{
	int xSeed, zSeed;
	unsigned int chunkCubeCount;
	unsigned int chunkHorizontalSlice;
}TerrainData;

typedef struct Chunk
{
	unsigned char cells[TERRAIN_CHUNK_CUBE_COUNT];
	unsigned int vertices[TERRAIN_CHUNK_CUBE_COUNT * 12];
	unsigned int indices[TERRAIN_CHUNK_CUBE_COUNT * 18];
	bool chunkChanged;
}Chunk;

Shader terrainShader;
Vao terrainVao;

TerrainData terrainData = { 0 };
Chunk terrainChunk;
bool terrainIsWireMode;

static void CreateShader();
static void CreateVao(unsigned int indexCount, unsigned int vertexCount);
static void CreateChunkCells(Chunk* chunk);
static void CreateChunkVI(Chunk* chunk, unsigned int* vCount, unsigned int* iCount);

//region Callback Functions
void load_terrain()
{
	terrainData.xSeed = rand() % 10000000;
	terrainData.zSeed = rand() % 10000000;
	terrainData.chunkHorizontalSlice = TERRAIN_CHUNK_SIZE * TERRAIN_CHUNK_SIZE;
	terrainData.chunkCubeCount = terrainData.chunkHorizontalSlice * TERRAIN_HEIGHT;
	CreateChunkCells(&terrainChunk);
	
	unsigned int vCount = 0, iCount = 0;
	CreateChunkVI(&terrainChunk, &vCount, &iCount);
	CreateShader();
	CreateVao(vCount, iCount);
}

void update_terrain()
{
	if(cm_is_key_pressed(KEY_B))
	{
		if(terrainIsWireMode) cm_disable_wire_mode();
		else cm_enable_wire_mode();

		terrainIsWireMode = !terrainIsWireMode;
	}
}

void draw_terrain()
{
	cm_begin_shader_mode(terrainShader);

	cm_draw_vao(terrainVao, CM_TRIANGLES);
	
	cm_end_shader_mode();
}

void dispose_terrain()
{
	cm_unload_vao(terrainVao);
	cm_unload_shader(terrainShader);
}
//endregion

//region Local Functions

static void CreateShader()
{
	char vsPath[MAX_PATH_SIZE] = RES_PATH;
	strcat_s(vsPath, MAX_PATH_SIZE, "shaders/voxel_terrain.vert");
	
	char fsPath[MAX_PATH_SIZE] = RES_PATH;
	strcat_s(fsPath, MAX_PATH_SIZE, "shaders/voxel_terrain.frag");

	terrainShader = cm_load_shader(vsPath, fsPath);
}

static void CreateVao(unsigned int indexCount, unsigned int vertexCount)
{
	VaoAttribute attributes[] =
	{
		{ 1, CM_UINT, false, 1 * sizeof(unsigned int) },
//		{ 2, CM_FLOAT, false, 3 * sizeof(float) }
	};
	
	Vbo vbo = { 0 };
	vbo.id = 0;
	vbo.isStatic = false;
	vbo.data = terrainChunk.vertices;
	vbo.vertexCount = vertexCount;
	vbo.dataSize = sizeof(terrainChunk.vertices);
	Ebo ebo = {0};
	ebo.id = 0;
	ebo.isStatic = false;
	ebo.dataSize = sizeof(terrainChunk.indices);
	ebo.data = terrainChunk.indices;
	ebo.type = CM_UINT;
	ebo.indexCount = indexCount;
	vbo.ebo = ebo;

	terrainVao = cm_load_vao(attributes, 1, vbo);
}

static void CreateChunkVI(Chunk* chunk, unsigned int* vCount, unsigned int* iCount)
{
	unsigned int indexCount = 0;
	unsigned int vertCount = 0;
	unsigned int execCount = 0;

	unsigned int indexSpace = sizeof(chunk->indices) / 4;
	unsigned int vertexSpace = sizeof(chunk->vertices) / 4;
	
	for (unsigned int y = 0; y < TERRAIN_HEIGHT; y++)
	{
		for (unsigned int x = 0; x < TERRAIN_CHUNK_SIZE; x++)
		{
			for (unsigned int z = 0; z < TERRAIN_CHUNK_SIZE; z++)
			{
				unsigned int cubeId = y * terrainData.chunkHorizontalSlice + x * TERRAIN_CHUNK_SIZE + z;
				if(chunk->cells[cubeId] == 0) continue;

				if(indexSpace - indexCount < 36 || vertexSpace - vertCount < 8)
				{
					printf("Space is too low, terminating the execution!!! vFree: %i, iFree: %i",
						   indexSpace - indexCount, vertexSpace - vertCount);
					break;
				}

				unsigned int temp = 0;
				unsigned char faceMask = 0;

				//region define faces
				//front
//				temp = (z == TERRAIN_CHUNK_SIZE - 1 || chunk->cells[cubeId + 1] == 0);
				temp = (z == TERRAIN_CHUNK_SIZE - 1);
				faceMask |= temp << 5;

				//back
//				temp = (z == 0 || chunk->cells[cubeId - 1] == 0);
				temp = (z == 0);
				faceMask |= temp << 4;

				//right
//				temp = (x == TERRAIN_CHUNK_SIZE - 1 || chunk->cells[cubeId + TERRAIN_CHUNK_SIZE] == 0);
				temp = (x == TERRAIN_CHUNK_SIZE - 1);
				faceMask |= temp << 3;

				//left
//				temp = (x == 0 || chunk->cells[cubeId - TERRAIN_CHUNK_SIZE] == 0);
				temp = (x == 0);
				faceMask |= temp << 2;

				//top
//				temp = (y == TERRAIN_HEIGHT - 1 || chunk->cells[cubeId + terrainData.chunkHorizontalSlice] == 0);
				temp = (y == TERRAIN_HEIGHT - 1);
				faceMask |= temp << 1;

				//bottom
//				temp = (y == 0 || chunk->cells[cubeId - terrainData.chunkHorizontalSlice] == 0);
				temp = (y == 0);
				faceMask |= temp;
				//endregion

				if(faceMask == 0) continue;

				//region definitions

#define RECT_FACE chunk->indices[indexCount]     = vertCount + 0;\
				  chunk->indices[indexCount + 1] = vertCount + 1;\
				  chunk->indices[indexCount + 2] = vertCount + 2;\
				  chunk->indices[indexCount + 3] = vertCount + 0;\
				  chunk->indices[indexCount + 4] = vertCount + 2;\
				  chunk->indices[indexCount + 5] = vertCount + 3;\
				  indexCount += 6
				//endregion

				unsigned int blockIndex = (x << 15) | (y << 7) | (z << 3);

				//front face
				if((faceMask & 0b100000) > 0)
				{
					RECT_FACE;
					chunk->vertices[vertCount]     = blockIndex | 0b001;
					chunk->vertices[vertCount + 1] = blockIndex | 0b101;
					chunk->vertices[vertCount + 2] = blockIndex | 0b111;
					chunk->vertices[vertCount + 3] = blockIndex | 0b011;
					vertCount += 4;
				}

				//back face
				if((faceMask & 0b010000) > 0)
				{
					RECT_FACE;
					chunk->vertices[vertCount]     = blockIndex | 0b000;
					chunk->vertices[vertCount + 1] = blockIndex | 0b010;
					chunk->vertices[vertCount + 2] = blockIndex | 0b110;
					chunk->vertices[vertCount + 3] = blockIndex | 0b100;
					vertCount += 4;
				}

				//right face
				if((faceMask & 0b001000) > 0)
				{
					RECT_FACE;
					chunk->vertices[vertCount]     = blockIndex | 0b100;
					chunk->vertices[vertCount + 1] = blockIndex | 0b110;
					chunk->vertices[vertCount + 2] = blockIndex | 0b111;
					chunk->vertices[vertCount + 3] = blockIndex | 0b101;
					vertCount += 4;
				}

				printf("x: %i, y: %i, z: %i, c: %i,\n", x, y, z, cubeId);

				//left face
				if((faceMask & 0b000100) > 0)
				{
					RECT_FACE;
					chunk->vertices[vertCount]     = blockIndex | 0b001;
					chunk->vertices[vertCount + 1] = blockIndex | 0b011;
					chunk->vertices[vertCount + 2] = blockIndex | 0b010;
					chunk->vertices[vertCount + 3] = blockIndex | 0b000;
					vertCount += 4;
				}

				//top face
				if((faceMask & 0b000010) > 0)
				{
					RECT_FACE;
					chunk->vertices[vertCount]     = blockIndex | 0b010;
					chunk->vertices[vertCount + 1] = blockIndex | 0b011;
					chunk->vertices[vertCount + 2] = blockIndex | 0b111;
					chunk->vertices[vertCount + 3] = blockIndex | 0b110;

					printf("size: %i, id: %i\n", (int)sizeof(chunk->vertices)/4, vertCount);

					vertCount += 4;
				}

				if((faceMask & 0b000001) > 0)
				{
					RECT_FACE;
					chunk->vertices[vertCount]     = blockIndex | 0b001;
					chunk->vertices[vertCount + 1] = blockIndex | 0b000;
					chunk->vertices[vertCount + 2] = blockIndex | 0b100;
					chunk->vertices[vertCount + 3] = blockIndex | 0b101;
					vertCount += 4;
				}
			}
		}
	}

#undef RECT_FACE

	*vCount = vertCount;
	*iCount = indexCount;
	printf("allSpace: %i, vertices %i\n", vertexSpace, vertCount);
	printf("allSpace: %i, indices %i\n", indexSpace, indexCount);
	printf("exeCount: %i\n", execCount);
}

static void CreateChunkCells(Chunk* chunk)
{
	fnl_state noise = fnlCreateState();
	noise.noise_type = FNL_NOISE_PERLIN;

	int minHeight = (TERRAIN_HEIGHT - TERRAIN_RANDOMNESS_AREA) / 2;

	for (unsigned int x = 0; x < TERRAIN_CHUNK_SIZE; ++x)
	{
		for (unsigned int z = 0; z < TERRAIN_CHUNK_SIZE; ++z)
		{
//			float nVal = (fnlGetNoise2D(&noise, (float)(terrainData.xSeed + x), (float)(terrainData.zSeed + z)) + 1) * .5f;
			unsigned int xzId = x * TERRAIN_CHUNK_SIZE + z;
//			unsigned int till = (unsigned int)(minHeight + (int)(nVal * TERRAIN_RANDOMNESS_AREA));
			
//			for (unsigned int y = 0; y < till; ++y)
//				chunk->cells[y * terrainData.chunkHorizontalSlice + xzId] = 1;
//
//			for (unsigned int y = till; y < TERRAIN_HEIGHT; ++y)
//				chunk->cells[y * terrainData.chunkHorizontalSlice + xzId] = 0;

			for (unsigned int y = 0; y < TERRAIN_HEIGHT; ++y)
				chunk->cells[y * terrainData.chunkHorizontalSlice + xzId] = 1;
		}
	}
}

//endregion