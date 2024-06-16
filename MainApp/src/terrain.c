#include "terrain.h"
#include "config.h"

Shader terrainShader;
Vao vao;

typedef struct Chunk
{
	unsigned char cells[TERRAIN_CHUNK_SIZE * TERRAIN_CHUNK_SIZE * TERRAIN_HEIGHT];
	float vertices[TERRAIN_CHUNK_SIZE * TERRAIN_CHUNK_SIZE * TERRAIN_HEIGHT * 8 * 3];
	unsigned short indices[TERRAIN_CHUNK_SIZE * TERRAIN_CHUNK_SIZE * TERRAIN_HEIGHT * 12 * 3];
	bool chunkChanged;
}Chunk;

Chunk terrainChunk;

static void CreateShader();
static void CreateVao(unsigned int indexCount);

//region Callback Functions
void load_terrain()
{
	for (unsigned char y = 0; y < TERRAIN_HEIGHT; ++y)
	{
		for (unsigned char x = 0; x < TERRAIN_CHUNK_SIZE; ++x)
		{
			for (unsigned char z = 0; z < TERRAIN_CHUNK_SIZE; ++z)
			{
				int id = y * TERRAIN_CHUNK_SIZE * TERRAIN_CHUNK_SIZE + x * TERRAIN_CHUNK_SIZE + z;
				terrainChunk.cells[id] = 1;
			}
		}
	}

	unsigned int vStart = 0;
	unsigned int indexCount = 0;
	unsigned int idStart = 0;

	for (unsigned int y = 0; y < TERRAIN_HEIGHT; ++y)
	{
		for (unsigned int x = 0; x < TERRAIN_CHUNK_SIZE; ++x)
		{
			for (unsigned int z = 0; z < TERRAIN_CHUNK_SIZE; ++z)
			{
				unsigned int frontId = vStart + 1;
				unsigned int backId = vStart - 1;
				unsigned int rightId = vStart + TERRAIN_CHUNK_SIZE;
				unsigned int leftId = vStart - TERRAIN_CHUNK_SIZE;
				unsigned int topId = vStart + TERRAIN_CHUNK_SIZE * TERRAIN_CHUNK_SIZE;
				unsigned int bottomId = vStart - TERRAIN_CHUNK_SIZE * TERRAIN_CHUNK_SIZE;

				unsigned int sIndexCount = indexCount;

				//front face
				if(z < TERRAIN_CHUNK_SIZE - 1 && terrainChunk.cells[frontId] > 0)
				{
					terrainChunk.indices[indexCount]      = idStart + 0;
					terrainChunk.indices[indexCount + 1]  = idStart + 2;
					terrainChunk.indices[indexCount + 2]  = idStart + 1;

					terrainChunk.indices[indexCount + 3]  = idStart + 0;
					terrainChunk.indices[indexCount + 4]  = idStart + 3;
					terrainChunk.indices[indexCount + 5]  = idStart + 2;
					indexCount += 6;
				}

				//back face
				if(z > 0 && terrainChunk.cells[backId] > 0)
				{
					terrainChunk.indices[indexCount] = idStart + 5;
					terrainChunk.indices[indexCount + 1] = idStart + 7;
					terrainChunk.indices[indexCount + 2] = idStart + 4;

					terrainChunk.indices[indexCount + 3] = idStart + 5;
					terrainChunk.indices[indexCount + 4] = idStart + 6;
					terrainChunk.indices[indexCount + 5] = idStart + 7;
					indexCount += 6;
				}

				printf("%i\n", rightId);
				//right face
				if(x < TERRAIN_CHUNK_SIZE - 1 && terrainChunk.cells[rightId] == 0)
				{
					terrainChunk.indices[indexCount]  = idStart + 1;
					terrainChunk.indices[indexCount + 1]  = idStart + 6;
					terrainChunk.indices[indexCount + 2]  = idStart + 5;

					terrainChunk.indices[indexCount + 3]  = idStart + 1;
					terrainChunk.indices[indexCount + 4] = idStart + 2;
					terrainChunk.indices[indexCount + 5] = idStart + 6;
					indexCount += 6;
				}

				//left face
				if(x > 0 && terrainChunk.cells[leftId] > 0)
				{
					terrainChunk.indices[indexCount] = idStart + 4;
					terrainChunk.indices[indexCount + 1] = idStart + 3;
					terrainChunk.indices[indexCount + 2] = idStart + 0;

					terrainChunk.indices[indexCount + 3] = idStart + 4;
					terrainChunk.indices[indexCount + 4] = idStart + 7;
					terrainChunk.indices[indexCount + 5] = idStart + 3;
					indexCount += 6;
				}

				//top face
				if(y < TERRAIN_HEIGHT - 1 && terrainChunk.cells[topId] > 0)
				{
					terrainChunk.indices[indexCount] = idStart + 3;
					terrainChunk.indices[indexCount + 1] = idStart + 6;
					terrainChunk.indices[indexCount + 2] = idStart + 2;

					terrainChunk.indices[indexCount + 3] = idStart + 3;
					terrainChunk.indices[indexCount + 4] = idStart + 7;
					terrainChunk.indices[indexCount + 5] = idStart + 6;
					indexCount += 6;
				}

				//bottom face
				if(y > 0 && terrainChunk.cells[bottomId] > 0)
				{
					terrainChunk.indices[indexCount] = idStart + 4;
					terrainChunk.indices[indexCount + 1] = idStart + 1;
					terrainChunk.indices[indexCount + 2] = idStart + 5;

					terrainChunk.indices[indexCount + 3] = idStart + 4;
					terrainChunk.indices[indexCount + 4] = idStart + 0;
					terrainChunk.indices[indexCount + 5] = idStart + 1;
					indexCount += 6;
				}

				if(sIndexCount != indexCount)
				{
					//region vertices
					terrainChunk.vertices[vStart]     = (float)x;
					terrainChunk.vertices[vStart + 1] = (float)y;
					terrainChunk.vertices[vStart + 2] = (float)z;

					terrainChunk.vertices[vStart + 3] = (float)x + 1;
					terrainChunk.vertices[vStart + 4] = (float)y;
					terrainChunk.vertices[vStart + 5] = (float)z;

					terrainChunk.vertices[vStart + 6] = (float)x + 1;
					terrainChunk.vertices[vStart + 7] = (float)y + 1;
					terrainChunk.vertices[vStart + 8] = (float)z;

					terrainChunk.vertices[vStart + 9]  = (float)x;
					terrainChunk.vertices[vStart + 10] = (float)y + 1;
					terrainChunk.vertices[vStart + 11] = (float)z;

					terrainChunk.vertices[vStart + 12] = (float)x;
					terrainChunk.vertices[vStart + 13] = (float)y;
					terrainChunk.vertices[vStart + 14] = (float)z + 1;

					terrainChunk.vertices[vStart + 15] = (float)x + 1;
					terrainChunk.vertices[vStart + 16] = (float)y;
					terrainChunk.vertices[vStart + 17] = (float)z + 1;

					terrainChunk.vertices[vStart + 18] = (float)x + 1;
					terrainChunk.vertices[vStart + 19] = (float)y + 1;
					terrainChunk.vertices[vStart + 20] = (float)z + 1;

					terrainChunk.vertices[vStart + 21] = (float)x;
					terrainChunk.vertices[vStart + 22] = (float)y + 1;
					terrainChunk.vertices[vStart + 23] = (float)z + 1;

					vStart += 24;
					//endregion
				}

				idStart += 8;
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
		{ 3, CM_FLOAT, false, 3 * sizeof(float) },
//		{ 2, CM_FLOAT, false, 3 * sizeof(float) }
	};
	
	Vbo vbo = { 0 };
	vbo.id = 0;
	vbo.isStatic = false;
	vbo.data = terrainChunk.vertices;
	vbo.vertexCount = TERRAIN_CHUNK_SIZE * TERRAIN_CHUNK_SIZE * TERRAIN_HEIGHT * 24;
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