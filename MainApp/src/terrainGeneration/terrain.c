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

typedef enum
{
	CHUNK_REQUIRES_NOISE_MAP,
	CHUNK_GENERATING_NOISE_MAP,
	CHUNK_REQUIRES_FACES,
	CHUNK_CREATING_FACES,
	CHUNK_REQUIRES_UPLOAD,
	CHUNK_READY_TO_DRAW,
}ChunkState;

typedef struct {
	unsigned int chunkId;
	unsigned int chunk[3];
} UniformData;

typedef struct TerrainChunk
{
	ChunkState state;
	unsigned short faceCount;
	bool waitingForResponse;
	bool isAlive;
	unsigned int id[3];
	
	unsigned int* buffer;
	unsigned char* cells;
}TerrainChunk;

typedef struct
{
	int u_chunkIndex;
	int u_surfaceTex;

	fnl_state heightNoise;
	fnl_state caveNoise;

	ThreadPool* pool;
	
	Shader shader;
	Texture textures[3];

	Vao quadVao;
	Ssbo ssbo;
	
	TerrainChunk chunks[TERRAIN_CHUNK_COUNT];
	unsigned char heightMap[FULL_HORIZONTAL_SLICE];
}VoxelTerrain;

VoxelTerrain voxelTerrain = { 0 };
bool terrainIsWireMode;

static void CreateShader();
static void InitNoise();
static void GeneratePreChunk(unsigned int xId, unsigned int yId, unsigned int zId);
static void GeneratePostChunk(unsigned int xId, unsigned int yId, unsigned int zId);
static void GenerateHeightMap(const unsigned int sourceId[2], const unsigned int destination[2]);

static void T_GenerateVerticalNoise(void* args);
static void T_OnVerticalGenerationFinished(void* args);

static void CreateChunkFaces(unsigned int xId, unsigned int yId, unsigned int zId);
static void PassTerrainDataToShader(UniformData* data);
static unsigned int GetChunkId(unsigned int x, unsigned int y, unsigned int z);
static void SendVerticalJob(unsigned int x, unsigned int z);

static void SendFaceCreationJob(unsigned int x, unsigned int y, unsigned int z);
static void T_CreateChunkFaces(void* args);
static void T_ChunkFacesCreationFinished(void* args);

static void SendVerticalFaceCreationJob(unsigned int x, unsigned int z);
static void T_CreateChunkFacesVertical(void* args);
static void T_ChunkFacesVerticalCreationFinished(void* args);

static TerrainChunk CreateChunk(unsigned int x, unsigned int y, unsigned int z);
static void DestroyChunk(TerrainChunk* chunk);

static bool SurroundsAreLoaded(unsigned int xId, unsigned int zId);

//region Callback Functions
void load_terrain()
{
	load_block_types();
	CreateShader();
	InitNoise();
	
	for (int x = 0; x < TERRAIN_VIEW_RANGE; ++x)
	{
		for (int z = 0; z < TERRAIN_VIEW_RANGE; ++z)
		{
			for (int y = 0; y < TERRAIN_HEIGHT; ++y)
			{
				unsigned int id = y * TERRAIN_VIEW_RANGE * TERRAIN_VIEW_RANGE + x * TERRAIN_VIEW_RANGE + z;
				voxelTerrain.chunks[id] = CreateChunk(x, y, z);
			}
		}
	}
	
	voxelTerrain.quadVao = cm_get_unit_quad();
	voxelTerrain.ssbo = cm_load_ssbo(64, sizeof(unsigned int) * TERRAIN_CHUNK_CUBE_COUNT * TERRAIN_CHUNK_COUNT, NULL);
	
	voxelTerrain.pool = cm_create_thread_pool(TERRAIN_NUM_WORKER_THREADS);
	
	for (unsigned int x = 0; x < TERRAIN_VIEW_RANGE; ++x)
		for (unsigned int z = 0; z < TERRAIN_VIEW_RANGE; ++z)
			SendVerticalJob(TERRAIN_VIEW_RANGE - 1 - x, TERRAIN_VIEW_RANGE - 1 - z);
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

//	printf("FrameTime: %f\n", cm_frame_time() * 1000);
}

void draw_terrain()
{
	for (unsigned int x = 0; x < TERRAIN_VIEW_RANGE; ++x)
		for (unsigned int z = 0; z < TERRAIN_VIEW_RANGE; ++z)
		{
			if(SurroundsAreLoaded(x, z))
			{
				bool wholeVerticalNeedsFaces = true;
				for (int y = 0; y < TERRAIN_HEIGHT; ++y)
				{
					unsigned int chunkId = GetChunkId(x, y, z);
					wholeVerticalNeedsFaces = wholeVerticalNeedsFaces &&
					                          (voxelTerrain.chunks[chunkId].state == CHUNK_REQUIRES_FACES &&
					                           !voxelTerrain.chunks[chunkId].waitingForResponse);
				}
				
				if(wholeVerticalNeedsFaces)
				{
					unsigned int chunkId = GetChunkId(x, 0, z);
					if(voxelTerrain.chunks[chunkId].state == CHUNK_REQUIRES_FACES && !voxelTerrain.chunks[chunkId].waitingForResponse)
						SendVerticalFaceCreationJob(x, z);
				}
				else
				{
					for (int y = 0; y < TERRAIN_HEIGHT; ++y)
					{
						unsigned int chunkId = GetChunkId(x, y, z);
						if(voxelTerrain.chunks[chunkId].state == CHUNK_REQUIRES_FACES && !voxelTerrain.chunks[chunkId].waitingForResponse)
							SendFaceCreationJob(x, y, z);
					}
				}
			}
		}
	
	cm_begin_shader_mode(voxelTerrain.shader);
	
	for (int i = 0; i < 4; ++i)
		cm_set_texture(voxelTerrain.u_surfaceTex + i, voxelTerrain.textures[0].id, i);
	
	cm_set_texture(voxelTerrain.u_surfaceTex + 4, voxelTerrain.textures[1].id, 4);
	cm_set_texture(voxelTerrain.u_surfaceTex + 5, voxelTerrain.textures[2].id, 5);

	UniformData data = {0};
	int numUploadsLeft = TERRAIN_CHUNK_UPLOAD_LIMIT;
	
	for (unsigned int x = 0; x < TERRAIN_VIEW_RANGE; ++x)
	{
		for (unsigned int z = 0; z < TERRAIN_VIEW_RANGE; ++z)
		{
			for (unsigned int y = 0; y < TERRAIN_HEIGHT; ++y)
			{
				data.chunk[0] = x;
				data.chunk[1] = y;
				data.chunk[2] = z;
				data.chunkId = GetChunkId(x, y, z);
				
				TerrainChunk* chunk = &voxelTerrain.chunks[data.chunkId];
				
				if(!chunk->waitingForResponse && chunk->faceCount > 0)
				{
					if(chunk->state == CHUNK_REQUIRES_UPLOAD)
					{
						if(numUploadsLeft <= 0) continue;
						cm_upload_ssbo(voxelTerrain.ssbo, data.chunkId * TERRAIN_CHUNK_CUBE_COUNT * sizeof(unsigned int),
						               chunk->faceCount * sizeof(unsigned int), chunk->buffer);
						chunk->state = CHUNK_READY_TO_DRAW;
						numUploadsLeft--;
					}
					
					if(chunk->state == CHUNK_READY_TO_DRAW)
					{
						PassTerrainDataToShader(&data);
						cm_draw_instanced_vao(voxelTerrain.quadVao, CM_TRIANGLES, chunk->faceCount);
					}
				}
			}
			
//			if(uploadCount > 0) shouldUpload = false;
		}
	}

	cm_end_shader_mode();
}

void dispose_terrain()
{
	cm_destroy_thread_pool(voxelTerrain.pool);
	for (int i = 0; i < TERRAIN_CHUNK_COUNT; ++i) DestroyChunk(&voxelTerrain.chunks[i]);
	
	voxelTerrain.pool = NULL;
	for (int i = 0; i < 3; ++i) cm_unload_texture(voxelTerrain.textures[i]);

	cm_unload_ssbo(voxelTerrain.ssbo);
	cm_unload_shader(voxelTerrain.shader);
}
//endregion

//region Local Functions

static void CreateShader()
{
	Path vsPath = TO_RES_PATH(vsPath, "shaders/voxel_terrain.vert");
	Path fsPath = TO_RES_PATH(fsPath, "shaders/voxel_terrain.frag");
	
	voxelTerrain.shader = cm_load_shader(vsPath, fsPath);
	voxelTerrain.u_chunkIndex = cm_get_uniform_location(voxelTerrain.shader, "u_chunkIndex");
	voxelTerrain.u_surfaceTex = cm_get_uniform_location(voxelTerrain.shader, "u_surfaceTex");

	Path mapPath0 = TO_RES_PATH(mapPath0, "2d/terrain/0.Map_Side.png");
	Path mapPath1 = TO_RES_PATH(mapPath1, "2d/terrain/1.Map_Top.png");
	Path mapPath2 = TO_RES_PATH(mapPath2, "2d/terrain/2.Map_Bottom.png");

	voxelTerrain.textures[0] = cm_load_texture(mapPath0, CM_TEXTURE_WRAP_CLAMP_TO_EDGE, CM_TEXTURE_FILTER_NEAREST, false);
	voxelTerrain.textures[1] = cm_load_texture(mapPath1, CM_TEXTURE_WRAP_CLAMP_TO_EDGE, CM_TEXTURE_FILTER_NEAREST, false);
	voxelTerrain.textures[2] = cm_load_texture(mapPath2, CM_TEXTURE_WRAP_CLAMP_TO_EDGE, CM_TEXTURE_FILTER_NEAREST, false);
}

static void InitNoise()
{
	//3D
	voxelTerrain.caveNoise = fnlCreateState();
	voxelTerrain.caveNoise.noise_type = FNL_NOISE_PERLIN;
	voxelTerrain.caveNoise.seed = rand() % 10000000;
	voxelTerrain.caveNoise.fractal_type = FNL_FRACTAL_PINGPONG;
	voxelTerrain.caveNoise.gain = TERRAIN_CAVE_GAIN;
	voxelTerrain.caveNoise.lacunarity = TERRAIN_CAVE_LACUNARITY;
	voxelTerrain.caveNoise.frequency = TERRAIN_CAVE_FREQUENCY;

	//2D
	voxelTerrain.caveNoise = fnlCreateState();
	voxelTerrain.caveNoise.noise_type = FNL_NOISE_PERLIN;
	voxelTerrain.caveNoise.seed = rand() % 10000000;
	voxelTerrain.caveNoise.fractal_type = FNL_FRACTAL_FBM;
	voxelTerrain.caveNoise.octaves = TERRAIN_HEIGHT_OCTAVES;
	voxelTerrain.caveNoise.gain = TERRAIN_HEIGHT_GAIN;
	voxelTerrain.caveNoise.lacunarity = TERRAIN_HEIGHT_LACUNARITY;
	voxelTerrain.caveNoise.frequency = TERRAIN_HEIGHT_FREQUENCY;
}

static void SendFaceCreationJob(unsigned int x, unsigned int y, unsigned int z)
{
	unsigned int chunkId = GetChunkId(x, y, z);
	voxelTerrain.chunks[chunkId].waitingForResponse = true;
	voxelTerrain.chunks[chunkId].faceCount = 0;
	voxelTerrain.chunks[chunkId].state = CHUNK_CREATING_FACES;
	
	unsigned int* args = CM_MALLOC(3 * sizeof(unsigned int));
	args[0] = x;
	args[1] = y;
	args[2] = z;

	ThreadJob job = {0};
	job.args = args;
	job.job = T_CreateChunkFaces;
	job.callbackJob = T_ChunkFacesCreationFinished;
	cm_submit_job(voxelTerrain.pool, job);
}

static void SendVerticalFaceCreationJob(unsigned int x, unsigned int z)
{
	unsigned int* args = CM_MALLOC(2 * sizeof(unsigned int));
	args[0] = x;
	args[1] = z;
	
	ThreadJob job = {0};
	job.args = args;
	job.job = T_CreateChunkFacesVertical;
	job.callbackJob = T_ChunkFacesVerticalCreationFinished;
	
	for (int y = 0; y < TERRAIN_HEIGHT; ++y)
	{
		unsigned int chunkId = GetChunkId(x, y, z);
		voxelTerrain.chunks[chunkId].waitingForResponse = true;
		voxelTerrain.chunks[chunkId].faceCount = 0;
		voxelTerrain.chunks[chunkId].state = CHUNK_CREATING_FACES;
	}
	
	cm_submit_job(voxelTerrain.pool, job);
}

static void T_CreateChunkFacesVertical(void* args)
{
	unsigned int* cArgs = (unsigned int*)args;
	
	for (int y = 0; y < TERRAIN_HEIGHT; ++y)
		CreateChunkFaces(cArgs[0], y, cArgs[1]);
}

static void T_ChunkFacesVerticalCreationFinished(void* args)
{
	unsigned int* cArgs = (unsigned int*)args;
	
	for (int y = 0; y < TERRAIN_HEIGHT; ++y)
	{
		unsigned int chunkId = GetChunkId(cArgs[0], y, cArgs[1]);
		voxelTerrain.chunks[chunkId].waitingForResponse = false;
		voxelTerrain.chunks[chunkId].state = CHUNK_REQUIRES_UPLOAD;
	}
}

static void T_CreateChunkFaces(void* args)
{
	unsigned int* cArgs = (unsigned int*)args;
	CreateChunkFaces(cArgs[0], cArgs[1], cArgs[2]);
}

static void T_ChunkFacesCreationFinished(void* args)
{
	unsigned int* cArgs = (unsigned int*)args;
	unsigned int chunkId = GetChunkId(cArgs[0], cArgs[1], cArgs[2]);
	voxelTerrain.chunks[chunkId].waitingForResponse = false;
	voxelTerrain.chunks[chunkId].state = CHUNK_REQUIRES_UPLOAD;
}

static unsigned int GetBlockIndex(unsigned char currentCell,
                                  unsigned int x, unsigned int y, unsigned int z)
{
	currentCell--;
	unsigned int blockIndex = (currentCell / TERRAIN_MAX_AXIS_BLOCK_TYPES) << 4 |
	                          (currentCell % TERRAIN_MAX_AXIS_BLOCK_TYPES);

	blockIndex <<= 15;
	blockIndex |= (x << 10) | (y << 5) | z;
	blockIndex <<= 3;
	return blockIndex;
}

static unsigned int AddBlockFaces(unsigned int* buffer, unsigned int fCount,
								  unsigned char faceMask, unsigned int blockIndex)
{
	unsigned int faceCount = fCount;

	//front face
	if((faceMask & 0b100000) > 0)
	{
		buffer[faceCount] = blockIndex | 0;
		faceCount++;
	}

	//back face
	if((faceMask & 0b010000) > 0)
	{
		buffer[faceCount] = blockIndex | 1;
		faceCount++;
	}

	//right face
	if((faceMask & 0b001000) > 0)
	{
		buffer[faceCount] = blockIndex | 2;
		faceCount++;
	}

	//left face
	if((faceMask & 0b000100) > 0)
	{
		buffer[faceCount] = blockIndex | 3;
		faceCount++;
	}

	//top face
	if((faceMask & 0b000010) > 0)
	{
		buffer[faceCount] = blockIndex | 4;
		faceCount++;
	}

	if((faceMask & 0b000001) > 0)
	{
		buffer[faceCount] = blockIndex | 5;
		faceCount++;
	}

	return faceCount;
}

static void CreateChunkFaces(unsigned int xId, unsigned int yId, unsigned int zId)
{
	unsigned int faceCount = 0;
	unsigned int chunkId = GetChunkId(xId, yId, zId);
	TerrainChunk* chunk = &voxelTerrain.chunks[chunkId];
	unsigned int* buffer = chunk->buffer;
	unsigned char* cells = chunk->cells;

	unsigned char* frontChunkCells = NULL;
	unsigned char* backChunkCells = NULL;
	unsigned char* rightChunkCells = NULL;
	unsigned char* leftChunkCells = NULL;
	unsigned char* topChunkCells = NULL;
	unsigned char* bottomChunkCells = NULL;

	if(zId < TERRAIN_CHUNK_SIZE - 1) frontChunkCells = voxelTerrain.chunks[GetChunkId(xId, yId, zId + 1)].cells;
	if(zId > 0) backChunkCells = voxelTerrain.chunks[GetChunkId(xId, yId, zId - 1)].cells;
	if(xId < TERRAIN_CHUNK_SIZE - 1) rightChunkCells = voxelTerrain.chunks[GetChunkId(xId + 1, yId, zId)].cells;
	if(xId > 0) leftChunkCells = voxelTerrain.chunks[GetChunkId(xId - 1, yId, zId)].cells;
	if(yId < TERRAIN_CHUNK_SIZE - 1) topChunkCells = voxelTerrain.chunks[GetChunkId(xId, yId + 1, zId)].cells;
	if(yId > 0) bottomChunkCells = voxelTerrain.chunks[GetChunkId(xId, yId - 1, zId)].cells;

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
				if(z == TERRAIN_CHUNK_SIZE - 1)
					faceMask |= (frontChunkCells != NULL && frontChunkCells[y * CHUNK_HORIZONTAL_SLICE + x * TERRAIN_CHUNK_SIZE] == BLOCK_EMPTY) << 5;
				else faceMask |= (cells[cubeId + 1] == BLOCK_EMPTY) << 5;

				//Back
				if(z == 0)
					faceMask |= (backChunkCells != NULL && backChunkCells[y * CHUNK_HORIZONTAL_SLICE + x * TERRAIN_CHUNK_SIZE + TERRAIN_CHUNK_SIZE - 1] == BLOCK_EMPTY) << 4;
				else faceMask |= (cells[cubeId - 1] == BLOCK_EMPTY) << 4;

				//Right
				if(x == TERRAIN_CHUNK_SIZE - 1)
					faceMask |= (rightChunkCells != NULL && rightChunkCells[y * CHUNK_HORIZONTAL_SLICE + z] == BLOCK_EMPTY) << 3;
				else faceMask |= (cells[cubeId + TERRAIN_CHUNK_SIZE] == BLOCK_EMPTY) << 3;

				//Left
				if(x == 0)
					faceMask |= (leftChunkCells != NULL && leftChunkCells[y * CHUNK_HORIZONTAL_SLICE + (TERRAIN_CHUNK_SIZE - 1) * TERRAIN_CHUNK_SIZE + z] == BLOCK_EMPTY) << 2;
				else faceMask |= (cells[cubeId - TERRAIN_CHUNK_SIZE] == BLOCK_EMPTY) << 2;

				//Top
				if(y == TERRAIN_CHUNK_SIZE - 1)
					faceMask |= (topChunkCells != NULL && topChunkCells[x * TERRAIN_CHUNK_SIZE + z] == BLOCK_EMPTY) << 1;
				else faceMask |= (cells[cubeId + CHUNK_HORIZONTAL_SLICE] == BLOCK_EMPTY) << 1;

				//Bottom
				if(y == 0)
					faceMask |= (bottomChunkCells != NULL && bottomChunkCells[(TERRAIN_CHUNK_SIZE - 1) * CHUNK_HORIZONTAL_SLICE + x * TERRAIN_CHUNK_SIZE + z] == BLOCK_EMPTY);
				else faceMask |= (cells[cubeId - CHUNK_HORIZONTAL_SLICE] == BLOCK_EMPTY);

				//endregion

				if(faceMask == 0) continue;

				faceCount = AddBlockFaces(buffer, faceCount, faceMask, GetBlockIndex(currentCell, x, y, z));
			}
		}
	}

#undef RECT_FACE
	chunk->faceCount = faceCount;
}

static void GeneratePreChunk(unsigned int xId, unsigned int yId, unsigned int zId)
{
	TerrainChunk chunk = voxelTerrain.chunks[GetChunkId(xId, yId, zId)];
	memset(chunk.cells, 0, TERRAIN_CHUNK_CUBE_COUNT);

	for (unsigned int x = 0; x < TERRAIN_CHUNK_SIZE; ++x)
	{
		unsigned int px = chunk.id[0] * TERRAIN_CHUNK_SIZE + x;

		for (unsigned int z = 0; z < TERRAIN_CHUNK_SIZE; ++z)
		{
			unsigned int xzId = x * TERRAIN_CHUNK_SIZE + z;
			unsigned int pz = chunk.id[2] * TERRAIN_CHUNK_SIZE + z;
			int maxY = (int)voxelTerrain.heightMap[(xId * TERRAIN_CHUNK_SIZE + x) * FULL_AXIS_SIZE + z + zId * TERRAIN_CHUNK_SIZE] - (int)(chunk.id[1] * TERRAIN_CHUNK_SIZE);
			int yLimit = glm_imin((int)(TERRAIN_CHUNK_SIZE - 1), maxY);
			if(yLimit < 0) continue;

			for (unsigned int y = 0; y <= yLimit; ++y)
			{
				unsigned int py = chunk.id[1] * TERRAIN_CHUNK_SIZE + y;
				float caveValue = fnlGetNoise3D(&voxelTerrain.caveNoise, (float)(px), (float)(py), (float)(pz));
				
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
	TerrainChunk chunk = voxelTerrain.chunks[GetChunkId(xId, yId, zId)];
	for (unsigned int x = 0; x < TERRAIN_CHUNK_SIZE; ++x)
	{
		for (unsigned int z = 0; z < TERRAIN_CHUNK_SIZE; ++z)
		{
			unsigned int xzId = x * TERRAIN_CHUNK_SIZE + z;
			int maxY = (int)voxelTerrain.heightMap[(xId * TERRAIN_CHUNK_SIZE + x) * FULL_AXIS_SIZE + z + zId * TERRAIN_CHUNK_SIZE] - (int)(chunk.id[1] * TERRAIN_CHUNK_SIZE);
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

static void SendVerticalJob(unsigned int x, unsigned int z)
{
	for (int y = 0; y < TERRAIN_HEIGHT; ++y)
	{
		unsigned int id = GetChunkId(x, y, z);
		voxelTerrain.chunks[id].state = CHUNK_GENERATING_NOISE_MAP;
		voxelTerrain.chunks[id].waitingForResponse = true;
	}
	
	unsigned int* args = CM_MALLOC(4 * sizeof(unsigned int));
	args[0] = x;
	args[1] = z;
	args[2] = x;
	args[3] = z;
	ThreadJob job = {0};
	job.args = args;
	job.job = T_GenerateVerticalNoise;
	job.callbackJob = T_OnVerticalGenerationFinished;
	cm_submit_job(voxelTerrain.pool, job);
}

static void T_GenerateVerticalNoise(void* args)
{
	unsigned int* cArgs = (unsigned int*)args;
	GenerateHeightMap((unsigned int[2]){cArgs[0], cArgs[1]}, (unsigned int[2]){cArgs[2], cArgs[3]});
	for (int y = 0; y < TERRAIN_HEIGHT; ++y)
	{
		GeneratePreChunk(cArgs[2], y, cArgs[3]);
		GeneratePostChunk(cArgs[2], y, cArgs[3]);
	}
}

static void T_OnVerticalGenerationFinished(void* args)
{
	unsigned int* cArgs = (unsigned int*)args;
	
	for (int y = 0; y < TERRAIN_HEIGHT; ++y)
	{
		unsigned int id = GetChunkId(cArgs[2], y, cArgs[3]);
		voxelTerrain.chunks[id].waitingForResponse = false;
		voxelTerrain.chunks[id].state = CHUNK_REQUIRES_FACES;
	}
}

static void GenerateHeightMap(const unsigned int sourceId[2], const unsigned int destination[2])
{
	unsigned int xStart = destination[0] * TERRAIN_CHUNK_SIZE, xEnd = xStart + TERRAIN_CHUNK_SIZE;
	unsigned int zStart = destination[1] * TERRAIN_CHUNK_SIZE, zEnd = zStart + TERRAIN_CHUNK_SIZE;

	for (unsigned int x = xStart; x < xEnd; ++x)
	{
		unsigned int px = sourceId[0] * TERRAIN_CHUNK_SIZE + (x % TERRAIN_CHUNK_SIZE);
		for (unsigned int z = zStart; z < zEnd; ++z)
		{
			unsigned int pz = sourceId[1] * TERRAIN_CHUNK_SIZE + (z % TERRAIN_CHUNK_SIZE);

			float val2D = fnlGetNoise2D(&voxelTerrain.caveNoise, (float)(px), (float)(pz));
			val2D = (val2D + 1) * .5f;
			unsigned char height = (TERRAIN_LOWER_EDGE * TERRAIN_CHUNK_SIZE) +
				(unsigned char)(val2D * (TERRAIN_CHUNK_SIZE * (TERRAIN_UPPER_EDGE - TERRAIN_LOWER_EDGE) - 1));
			voxelTerrain.heightMap[x * FULL_AXIS_SIZE + z] = height;
		}
	}
}

static void PassTerrainDataToShader(UniformData* data)
{
	unsigned int index[4];
	memcpy_s(index, sizeof(index), data->chunk, sizeof(data->chunk));
	index[3] = data->chunkId;
	cm_set_uniform_uvec4(voxelTerrain.u_chunkIndex, index);
}

static unsigned int GetChunkId(unsigned int x, unsigned int y, unsigned int z)
{
	return y * TERRAIN_VIEW_RANGE * TERRAIN_VIEW_RANGE + x * TERRAIN_VIEW_RANGE + z;
}

static TerrainChunk CreateChunk(unsigned int x, unsigned int y, unsigned int z)
{
	TerrainChunk chunk =
	{
		.faceCount = 0,
		.buffer = CM_MALLOC(TERRAIN_CHUNK_CUBE_COUNT * sizeof(unsigned int)),
		.cells = CM_MALLOC(TERRAIN_CHUNK_CUBE_COUNT),
		.state = CHUNK_REQUIRES_NOISE_MAP,
		.id = { x, y, z },
		.waitingForResponse = false,
		.isAlive = true
	};
	
	return chunk;
}

static void DestroyChunk(TerrainChunk* chunk)
{
	if(!chunk->isAlive) return;
	chunk->isAlive = false;
	CM_FREE(chunk->cells);
	CM_FREE(chunk->buffer);
}

static bool SurroundsAreLoaded(unsigned int xId, unsigned int zId)
{
	bool areLoaded = true;
	for (unsigned int x = glm_imax(0, (int)xId - 1); x < glm_imin((int)(xId + 2), TERRAIN_VIEW_RANGE); ++x)
	{
		for (unsigned int z = glm_imax(0, (int)zId - 1); z < glm_imin((int)(zId + 2), TERRAIN_VIEW_RANGE); ++z)
			areLoaded = areLoaded && voxelTerrain.chunks[GetChunkId(x, 0, z)].state > CHUNK_GENERATING_NOISE_MAP;
	}
	return areLoaded;
}

//endregion