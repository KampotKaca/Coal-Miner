#include "terrain.h"
#include "config.h"

Shader terrainShader;
Vao vao;

typedef struct Chunk
{
	unsigned int vertices[TERRAIN_CHUNK_SIZE * TERRAIN_CHUNK_SIZE * TERRAIN_HEIGHT * 8];
	unsigned short indices[TERRAIN_CHUNK_SIZE * TERRAIN_CHUNK_SIZE * TERRAIN_HEIGHT * 12 * 3];
	bool chunkChanged;
}Chunk;

Chunk terrainChunk;

static void CreateShader();
static void CreateVao(unsigned int indexCount);

//region Callback Functions
void load_terrain()
{
	unsigned int indexCount = 0;
	unsigned int idStart = 0;
	unsigned int cubeId = 0;

	for (unsigned int y = 0; y < TERRAIN_HEIGHT; ++y)
	{
		for (unsigned int x = 0; x < TERRAIN_CHUNK_SIZE; ++x)
		{
			for (unsigned int z = 0; z < TERRAIN_CHUNK_SIZE; ++z)
			{
				unsigned int sIndexCount = indexCount;
				
				//front face
				if(z == TERRAIN_CHUNK_SIZE - 1)
				{
					terrainChunk.indices[indexCount] = idStart + 5;
					terrainChunk.indices[indexCount + 1] = idStart + 4;
					terrainChunk.indices[indexCount + 2] = idStart + 7;
					
					terrainChunk.indices[indexCount + 3] = idStart + 5;
					terrainChunk.indices[indexCount + 4] = idStart + 7;
					terrainChunk.indices[indexCount + 5] = idStart + 6;
					indexCount += 6;
				}
				
				//back face
				if(z == 0)
				{
					terrainChunk.indices[indexCount]      = idStart + 0;
					terrainChunk.indices[indexCount + 1]  = idStart + 1;
					terrainChunk.indices[indexCount + 2]  = idStart + 2;

					terrainChunk.indices[indexCount + 3]  = idStart + 0;
					terrainChunk.indices[indexCount + 4]  = idStart + 2;
					terrainChunk.indices[indexCount + 5]  = idStart + 3;

					indexCount += 6;
				}

				//right face
				if(x == TERRAIN_CHUNK_SIZE - 1)
				{
					terrainChunk.indices[indexCount]  = idStart + 1;
					terrainChunk.indices[indexCount + 1]  = idStart + 5;
					terrainChunk.indices[indexCount + 2]  = idStart + 6;

					terrainChunk.indices[indexCount + 3]  = idStart + 1;
					terrainChunk.indices[indexCount + 4] = idStart + 6;
					terrainChunk.indices[indexCount + 5] = idStart + 2;
					indexCount += 6;
				}

				//left face
				if(x == 0)
				{
					terrainChunk.indices[indexCount] = idStart + 4;
					terrainChunk.indices[indexCount + 1] = idStart + 0;
					terrainChunk.indices[indexCount + 2] = idStart + 3;

					terrainChunk.indices[indexCount + 3] = idStart + 4;
					terrainChunk.indices[indexCount + 4] = idStart + 3;
					terrainChunk.indices[indexCount + 5] = idStart + 7;
					indexCount += 6;
				}

				//top face
				if(y == TERRAIN_HEIGHT - 1)
				{
					terrainChunk.indices[indexCount] = idStart + 3;
					terrainChunk.indices[indexCount + 1] = idStart + 2;
					terrainChunk.indices[indexCount + 2] = idStart + 6;

					terrainChunk.indices[indexCount + 3] = idStart + 3;
					terrainChunk.indices[indexCount + 4] = idStart + 6;
					terrainChunk.indices[indexCount + 5] = idStart + 7;
					indexCount += 6;
				}

				//bottom face
				if(y == 0)
				{
					terrainChunk.indices[indexCount] = idStart + 4;
					terrainChunk.indices[indexCount + 1] = idStart + 5;
					terrainChunk.indices[indexCount + 2] = idStart + 1;

					terrainChunk.indices[indexCount + 3] = idStart + 4;
					terrainChunk.indices[indexCount + 4] = idStart + 1;
					terrainChunk.indices[indexCount + 5] = idStart + 0;
					indexCount += 6;
				}
				
				if(sIndexCount != indexCount)
				{
					//region vertices
					
					terrainChunk.vertices[idStart]     = 0;
					terrainChunk.vertices[idStart + 1] = 4;
					terrainChunk.vertices[idStart + 2] = 6;
					terrainChunk.vertices[idStart + 3] = 2;

					terrainChunk.vertices[idStart + 4] = 1;
					terrainChunk.vertices[idStart + 5] = 5;
					terrainChunk.vertices[idStart + 6] = 7;
					terrainChunk.vertices[idStart + 7] = 3;
					
					idStart += 8;
					//endregion
				}
				
				cubeId++;
			}
		}
	}

	CreateShader();
	CreateVao(indexCount);
}

void update_terrain()
{

}

void draw_terrain()
{
	cm_begin_shader_mode(terrainShader);

	cm_draw_vao(vao, CM_TRIANGLES);
	
	cm_end_shader_mode();
}

void dispose_terrain()
{
	cm_unload_vao(vao);
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

static void CreateVao(unsigned int indexCount)
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
	vbo.vertexCount = TERRAIN_CHUNK_SIZE * TERRAIN_CHUNK_SIZE * TERRAIN_HEIGHT * 8;
	vbo.dataSize = sizeof(terrainChunk.vertices);
	Ebo ebo = {0};
	ebo.id = 0;
	ebo.isStatic = false;
	ebo.dataSize = sizeof(terrainChunk.indices);
	ebo.data = terrainChunk.indices;
	ebo.type = CM_USHORT;
	ebo.indexCount = indexCount;
	vbo.ebo = ebo;
	
	vao = cm_load_vao(attributes, 1, vbo);
}

//endregion