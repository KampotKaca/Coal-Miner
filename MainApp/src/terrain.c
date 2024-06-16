#include "terrain.h"
#include "config.h"
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
	unsigned int vertices[TERRAIN_CHUNK_CUBE_COUNT * 2];
	unsigned int indices[TERRAIN_CHUNK_CUBE_COUNT * 3];
	bool chunkChanged;
}Chunk;

Shader terrainShader;
Vao terrainVao;

TerrainData terrainData = { 0 };
Chunk terrainChunk;

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
	unsigned int cubeId = 0;
	
	unsigned int indexSpace = sizeof(chunk->indices) / 4;
	unsigned int vertexSpace = sizeof(chunk->vertices) / 4;
	
	for (unsigned int y = 0; y < TERRAIN_HEIGHT; ++y)
	{
		for (unsigned int x = 0; x < TERRAIN_CHUNK_SIZE; ++x)
		{
			for (unsigned int z = 0; z < TERRAIN_CHUNK_SIZE; ++z)
			{
				if(chunk->cells[cubeId] == 0) goto ending;

				if(indexSpace - indexCount < 36 || vertexSpace - vertCount < 8)
				{
					printf("Space is too low, terminating the execution!!! vFree: %i, iFree: %i",
						   indexSpace - indexCount, vertexSpace - vertCount);
					break;
				}
				
				unsigned int sIndexCount = indexCount;
				unsigned int frontId = cubeId + 1;
				unsigned int backId = cubeId - 1;
				unsigned int rightId = cubeId + TERRAIN_CHUNK_SIZE;
				unsigned int leftId = cubeId - TERRAIN_CHUNK_SIZE;
				unsigned int topId = cubeId + terrainData.chunkHorizontalSlice;
				unsigned int bottomId = cubeId - terrainData.chunkHorizontalSlice;
				
				//front face
				if(z == TERRAIN_CHUNK_SIZE - 1 || chunk->cells[frontId] == 0)
				{
					chunk->indices[indexCount] = vertCount + 5;
					chunk->indices[indexCount + 1] = vertCount + 4;
					chunk->indices[indexCount + 2] = vertCount + 7;

					chunk->indices[indexCount + 3] = vertCount + 5;
					chunk->indices[indexCount + 4] = vertCount + 7;
					chunk->indices[indexCount + 5] = vertCount + 6;
					indexCount += 6;
				}
				
				//back face
				if(z == 0 || chunk->cells[backId] == 0)
				{
					chunk->indices[indexCount]      = vertCount + 0;
					chunk->indices[indexCount + 1]  = vertCount + 1;
					chunk->indices[indexCount + 2]  = vertCount + 2;
					
					chunk->indices[indexCount + 3]  = vertCount + 0;
					chunk->indices[indexCount + 4]  = vertCount + 2;
					chunk->indices[indexCount + 5]  = vertCount + 3;
					
					indexCount += 6;
				}
				
				//right face
				if(x == TERRAIN_CHUNK_SIZE - 1 || chunk->cells[rightId] == 0)
				{
					chunk->indices[indexCount]  = vertCount + 1;
					chunk->indices[indexCount + 1]  = vertCount + 5;
					chunk->indices[indexCount + 2]  = vertCount + 6;

					chunk->indices[indexCount + 3]  = vertCount + 1;
					chunk->indices[indexCount + 4] = vertCount + 6;
					chunk->indices[indexCount + 5] = vertCount + 2;
					indexCount += 6;
				}
				
				//left face
				if(x == 0 || chunk->cells[leftId] == 0)
				{
					chunk->indices[indexCount] = vertCount + 4;
					chunk->indices[indexCount + 1] = vertCount + 0;
					chunk->indices[indexCount + 2] = vertCount + 3;

					chunk->indices[indexCount + 3] = vertCount + 4;
					chunk->indices[indexCount + 4] = vertCount + 3;
					chunk->indices[indexCount + 5] = vertCount + 7;
					indexCount += 6;
				}
				
				//top face
				if(y == TERRAIN_HEIGHT - 1 || chunk->cells[topId] == 0)
				{
					chunk->indices[indexCount] = vertCount + 3;
					chunk->indices[indexCount + 1] = vertCount + 2;
					chunk->indices[indexCount + 2] = vertCount + 6;
					
					chunk->indices[indexCount + 3] = vertCount + 3;
					chunk->indices[indexCount + 4] = vertCount + 6;
					chunk->indices[indexCount + 5] = vertCount + 7;
					indexCount += 6;
				}
				
				//bottom face
				if(y == 0 || chunk->cells[bottomId] == 0)
				{
					chunk->indices[indexCount] = vertCount + 4;
					chunk->indices[indexCount + 1] = vertCount + 5;
					chunk->indices[indexCount + 2] = vertCount + 1;
					
					chunk->indices[indexCount + 3] = vertCount + 4;
					chunk->indices[indexCount + 4] = vertCount + 1;
					chunk->indices[indexCount + 5] = vertCount + 0;
					indexCount += 6;
				}
				
				if(sIndexCount != indexCount)
				{
					//region vertices
					unsigned int blockIndex = (x << 12) | (y << 4) | z;
					blockIndex = blockIndex << 3;
					
					chunk->vertices[vertCount]     = blockIndex | 0;
					chunk->vertices[vertCount + 1] = blockIndex | 4;
					chunk->vertices[vertCount + 2] = blockIndex | 6;
					chunk->vertices[vertCount + 3] = blockIndex | 2;
					
					chunk->vertices[vertCount + 4] = blockIndex | 1;
					chunk->vertices[vertCount + 5] = blockIndex | 5;
					chunk->vertices[vertCount + 6] = blockIndex | 7;
					chunk->vertices[vertCount + 7] = blockIndex | 3;

					vertCount += 8;
					//endregion
				}
				
				ending:
				cubeId++;
			}
		}
	}
	
	*vCount = vertCount;
	*iCount = indexCount;
	printf("allSpace: %i, vertices %i\n", vertexSpace, vertCount);
	printf("allSpace: %i, indices %i\n", indexSpace, indexCount);
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
			float nVal = (fnlGetNoise2D(&noise, (float)(terrainData.xSeed + x), (float)(terrainData.zSeed + z)) + 1) * .5f;
			unsigned int xzId = x * TERRAIN_CHUNK_SIZE + z;
			unsigned int till = (unsigned int)(minHeight + (int)(nVal * TERRAIN_RANDOMNESS_AREA));
			
			for (unsigned int y = 0; y < till; ++y)
				chunk->cells[y * terrainData.chunkHorizontalSlice + xzId] = 1;

			for (unsigned int y = till; y < TERRAIN_HEIGHT; ++y)
				chunk->cells[y * terrainData.chunkHorizontalSlice + xzId] = 0;
		}
	}
}

//endregion