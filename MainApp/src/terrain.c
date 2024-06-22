#include "terrain.h"
#include "config.h"
#include "coal_miner_internal.h"
#include <FastNoiseLite.h>

#define TERRAIN_CHUNK_CUBE_COUNT TERRAIN_CHUNK_SIZE * TERRAIN_CHUNK_SIZE * TERRAIN_CHUNK_SIZE
#define TERRAIN_CHUNK_COUNT TERRAIN_VIEW_RANGE * TERRAIN_VIEW_RANGE * TERRAIN_HEIGHT
#define CHUNK_HORIZONTAL_SLICE TERRAIN_CHUNK_SIZE * TERRAIN_CHUNK_SIZE

#define FULL_VERTICAL_SIZE TERRAIN_CHUNK_SIZE * TERRAIN_HEIGHT
#define FULL_AXIS_SIZE TERRAIN_CHUNK_SIZE * TERRAIN_VIEW_RANGE
#define FULL_HORIZONTAL_SLICE CHUNK_HORIZONTAL_SLICE * TERRAIN_VIEW_RANGE * TERRAIN_VIEW_RANGE

typedef struct
{
	unsigned int voxelFaces[TERRAIN_CHUNK_CUBE_COUNT * TERRAIN_CHUNK_COUNT];
}VoxelBuffer;

typedef struct {
	unsigned int chunkId;
	unsigned int chunk[3];
	unsigned int chunkSize;
	unsigned int chunkBlockCount;
	unsigned int verticalSize;
	unsigned int viewRange;
} TerrainData;

typedef struct
{
	int xSeed, ySeed, zSeed;
	int u_TerrainDataId;
	
	Shader terrainShader;
	Vao quadVao;
	Ssbo terrainSsbo;
	VoxelBuffer voxelBuffer;
	
	unsigned char cells[TERRAIN_CHUNK_CUBE_COUNT * TERRAIN_CHUNK_COUNT];
	bool chunksChanged[TERRAIN_CHUNK_COUNT];
	unsigned short faceCounts[TERRAIN_CHUNK_COUNT];
}VoxelTerrain;

VoxelTerrain voxelTerrain = { 0 };
bool terrainIsWireMode;

static void CreateShader();
static void CreateChunkCells(unsigned int xChunk, unsigned int yChunk, unsigned int zChunk);
static void CreateChunkFaces(unsigned int xChunk, unsigned int yChunk, unsigned int zChunk);
static void PassTerrainDataToShader(TerrainData* data);
static unsigned int GetChunkId(unsigned int x, unsigned int y, unsigned int z);

//region Callback Functions
void load_terrain()
{
	CreateShader();
	
	voxelTerrain.xSeed = rand() % 10000000;
	voxelTerrain.ySeed = rand() % 10000000;
	voxelTerrain.zSeed = rand() % 10000000;
	
	for (int i = 0; i < TERRAIN_CHUNK_COUNT; ++i) voxelTerrain.chunksChanged[i] = true;
	
	for (unsigned int y = 0; y < TERRAIN_HEIGHT; ++y)
		for (unsigned int x = 0; x < TERRAIN_VIEW_RANGE; ++x)
			for (unsigned int z = 0; z < TERRAIN_VIEW_RANGE; ++z)
				CreateChunkCells(x, y, z);
	
	for (unsigned int y = 0; y < TERRAIN_HEIGHT; ++y)
		for (unsigned int x = 0; x < TERRAIN_VIEW_RANGE; ++x)
			for (unsigned int z = 0; z < TERRAIN_VIEW_RANGE; ++z)
				CreateChunkFaces(x, y, z);
	
	voxelTerrain.quadVao = cm_get_unit_quad();
	voxelTerrain.terrainSsbo = cm_load_ssbo(64, sizeof(VoxelBuffer), &voxelTerrain.voxelBuffer, false);
}

void update_terrain()
{
	if(cm_is_key_pressed(KEY_B))
	{
		if(terrainIsWireMode) cm_disable_wire_mode();
		else cm_enable_wire_mode();

		terrainIsWireMode = !terrainIsWireMode;
	}

//	printf("FrameTime: %f\n", cm_frame_time() * 1000);
}

void draw_terrain()
{
	cm_begin_shader_mode(voxelTerrain.terrainShader);
	TerrainData data = {0};
	data.chunkSize = TERRAIN_CHUNK_SIZE;
	data.chunkBlockCount = TERRAIN_CHUNK_CUBE_COUNT;
	data.verticalSize = TERRAIN_HEIGHT;
	data.viewRange = TERRAIN_VIEW_RANGE;
	
	for (unsigned int y = 0; y < TERRAIN_HEIGHT; ++y)
	{
		for (unsigned int x = 0; x < TERRAIN_VIEW_RANGE; ++x)
		{
			for (unsigned int z = 0; z < TERRAIN_VIEW_RANGE; ++z)
			{
				data.chunk[0] = x;
				data.chunk[1] = y;
				data.chunk[2] = z;
				data.chunkId = GetChunkId(x, y, z);
				
				if(voxelTerrain.faceCounts[data.chunkId] > 0)
				{
					PassTerrainDataToShader(&data);
					cm_draw_instanced_vao(voxelTerrain.quadVao, CM_TRIANGLES, voxelTerrain.faceCounts[data.chunkId]);
				}
			}
		}
	}

	cm_end_shader_mode();
}

void dispose_terrain()
{
	cm_unload_ssbo(voxelTerrain.terrainSsbo);
	cm_unload_shader(voxelTerrain.terrainShader);
}
//endregion

//region Local Functions

static void CreateShader()
{
	Path vsPath = TO_RES_PATH(vsPath, "shaders/voxel_terrain.vert");
	Path fsPath = TO_RES_PATH(fsPath, "shaders/voxel_terrain.frag");
	
	voxelTerrain.terrainShader = cm_load_shader(vsPath, fsPath);
	voxelTerrain.u_TerrainDataId = cm_get_uniform_location(voxelTerrain.terrainShader, "u_TerrainData");
}

static void CreateChunkFaces(unsigned int xChunk, unsigned int yChunk, unsigned int zChunk)
{
	unsigned int chunkId = GetChunkId(xChunk, yChunk, zChunk);
	if(!voxelTerrain.chunksChanged[chunkId]) return;
	
	unsigned int xStart = xChunk * TERRAIN_CHUNK_SIZE, xEnd = xStart + TERRAIN_CHUNK_SIZE;
	unsigned int yStart = yChunk * TERRAIN_CHUNK_SIZE, yEnd = yStart + TERRAIN_CHUNK_SIZE;
	unsigned int zStart = zChunk * TERRAIN_CHUNK_SIZE, zEnd = zStart + TERRAIN_CHUNK_SIZE;
	
	unsigned int faceCount = 0;
	unsigned int faceOffset = chunkId * TERRAIN_CHUNK_CUBE_COUNT;
	
	for (unsigned int y = yStart; y < yEnd; y++)
	{
		for (unsigned int x = xStart; x < xEnd; x++)
		{
			for (unsigned int z = zStart; z < zEnd; z++)
			{
				unsigned int cubeId = y * FULL_HORIZONTAL_SLICE + x * FULL_AXIS_SIZE + z;
				if(voxelTerrain.cells[cubeId] == 0) continue;

				unsigned char faceMask = 0;
				
				//region define faces
				faceMask |= (z == FULL_AXIS_SIZE - 1 || voxelTerrain.cells[cubeId + 1] == 0) << 5;                         //front
				faceMask |= (z == 0 || voxelTerrain.cells[cubeId - 1] == 0) << 4;                                          //back
				faceMask |= (x == FULL_AXIS_SIZE - 1 || voxelTerrain.cells[cubeId + FULL_AXIS_SIZE] == 0) << 3;            //right
				faceMask |= (x == 0 || voxelTerrain.cells[cubeId - FULL_AXIS_SIZE] == 0) << 2;                             //left
				faceMask |= (y == FULL_VERTICAL_SIZE - 1 || voxelTerrain.cells[cubeId + FULL_HORIZONTAL_SLICE] == 0) << 1; //top
				faceMask |= (y == 0 || voxelTerrain.cells[cubeId - FULL_HORIZONTAL_SLICE] == 0);                           //bottom
				//endregion

				if(faceMask == 0) continue;

				unsigned int blockIndex = ((x % TERRAIN_CHUNK_SIZE) << 10) |
						                  ((y % TERRAIN_CHUNK_SIZE) << 5) | (z % TERRAIN_CHUNK_SIZE);
				blockIndex <<= 3;
				
				//front face
				if((faceMask & 0b100000) > 0)
				{
					voxelTerrain.voxelBuffer.voxelFaces[faceOffset + faceCount] = blockIndex | 0;
					faceCount++;
				}

				//back face
				if((faceMask & 0b010000) > 0)
				{
					voxelTerrain.voxelBuffer.voxelFaces[faceOffset + faceCount] = blockIndex | 1;
					faceCount++;
				}

				//right face
				if((faceMask & 0b001000) > 0)
				{
					voxelTerrain.voxelBuffer.voxelFaces[faceOffset + faceCount] = blockIndex | 2;
					faceCount++;
				}

				//left face
				if((faceMask & 0b000100) > 0)
				{
					voxelTerrain.voxelBuffer.voxelFaces[faceOffset + faceCount] = blockIndex | 3;
					faceCount++;
				}

				//top face
				if((faceMask & 0b000010) > 0)
				{
					voxelTerrain.voxelBuffer.voxelFaces[faceOffset + faceCount] = blockIndex | 4;
					faceCount++;
				}

				if((faceMask & 0b000001) > 0)
				{
					voxelTerrain.voxelBuffer.voxelFaces[faceOffset + faceCount] = blockIndex | 5;
					faceCount++;
				}
			}
		}
	}

#undef RECT_FACE

	voxelTerrain.faceCounts[chunkId] = faceCount;
}

static void CreateChunkCells(unsigned int xChunk, unsigned int yChunk, unsigned int zChunk)
{
	fnl_state noise = fnlCreateState();
	noise.noise_type = FNL_NOISE_PERLIN;

	unsigned int chunkId = yChunk * TERRAIN_VIEW_RANGE * TERRAIN_VIEW_RANGE +
						   xChunk * TERRAIN_VIEW_RANGE + zChunk;
	
	for (unsigned int y = 0; y < TERRAIN_CHUNK_SIZE; ++y)
	{
		unsigned int yId = yChunk * TERRAIN_CHUNK_SIZE + y;
		
		for (unsigned int x = 0; x < TERRAIN_CHUNK_SIZE; ++x)
		{
			unsigned int xId = xChunk * TERRAIN_CHUNK_SIZE + x;
			
			for (unsigned int z = 0; z < TERRAIN_CHUNK_SIZE; ++z)
			{
				unsigned int zId = zChunk * TERRAIN_CHUNK_SIZE + z;
//				float nVal = fnlGetNoise3D(&noise, (float)(voxelTerrain.xSeed + xId),
//							                       (float)(voxelTerrain.ySeed + yId),
//									               (float)(voxelTerrain.zSeed + zId));
//
//				if(nVal >= .5f)
//				{
//
//				}
				
				unsigned int id = chunkId * TERRAIN_CHUNK_CUBE_COUNT +
				                  y * CHUNK_HORIZONTAL_SLICE + x * TERRAIN_CHUNK_SIZE + z;
				voxelTerrain.cells[id] = 1;
			}
		}
	}
}

static void PassTerrainDataToShader(TerrainData* data)
{
	cm_set_uniform_u(voxelTerrain.u_TerrainDataId + 0, data->chunkId);
	cm_set_uniform_uvec3(voxelTerrain.u_TerrainDataId + 1, data->chunk);
	cm_set_uniform_u(voxelTerrain.u_TerrainDataId + 4, data->chunkSize);
	cm_set_uniform_u(voxelTerrain.u_TerrainDataId + 5, data->chunkBlockCount);
	cm_set_uniform_u(voxelTerrain.u_TerrainDataId + 6, data->verticalSize);
	cm_set_uniform_u(voxelTerrain.u_TerrainDataId + 7, data->viewRange);
}

static unsigned int GetChunkId(unsigned int x, unsigned int y, unsigned int z)
{
	return y * TERRAIN_VIEW_RANGE * TERRAIN_VIEW_RANGE + x * TERRAIN_VIEW_RANGE + z;
}

//endregion