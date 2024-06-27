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
	CHUNK_READY,
	CHUNK_GENERATING,
	CHUNK_REQUIRES_REDRAW,
	CHUNK_REDRAWING,
}ChunkState;

typedef struct
{
	unsigned int voxelFaces[TERRAIN_CHUNK_CUBE_COUNT * TERRAIN_CHUNK_COUNT];
}VoxelBuffer;

typedef struct {
	unsigned int chunkId;
	unsigned int chunk[3];
} UniformData;

typedef struct
{
	ChunkState state;
	unsigned int faceCount;
	unsigned int id[3];
} ChunkData;

typedef struct
{
	int u_chunkIndex;
	int u_surfaceTex;
	int u_outlineColor;

	fnl_state noise2D;
	fnl_state noise3D;

	Shader shader;
	Texture textures[6];

	Vao quadVao;
	Ssbo ssbo;
	VoxelBuffer voxelBuffer;
	
	unsigned char cells[TERRAIN_CHUNK_CUBE_COUNT * TERRAIN_CHUNK_COUNT];
	unsigned char heightMap[FULL_HORIZONTAL_SLICE];
	bool chunksChanged[TERRAIN_CHUNK_COUNT];
	unsigned short faceCounts[TERRAIN_CHUNK_COUNT];
}VoxelTerrain;

VoxelTerrain voxelTerrain = { 0 };
bool terrainIsWireMode;

static void CreateShader();
static void InitNoise();
static void GenerateChunk(const unsigned int chunkId[3], const unsigned int destination[3]);
static void GenerateHeightMap(const unsigned int chunkId[2], const unsigned int destination[2]);
static void CreateChunkFaces(const unsigned int chunk[3]);
static void PassTerrainDataToShader(UniformData* data);
static unsigned int GetChunkId(const unsigned int id[3]);

//region Callback Functions
void load_terrain()
{
	CreateShader();
	InitNoise();
	
	for (int i = 0; i < TERRAIN_CHUNK_COUNT; ++i) voxelTerrain.chunksChanged[i] = true;

	for (unsigned int x = 0; x < TERRAIN_VIEW_RANGE; ++x)
		for (unsigned int z = 0; z < TERRAIN_VIEW_RANGE; ++z)
			GenerateHeightMap((unsigned int[]){x, z}, (unsigned int[]){x, z});

	for (unsigned int y = 0; y < TERRAIN_HEIGHT; ++y)
		for (unsigned int x = 0; x < TERRAIN_VIEW_RANGE; ++x)
			for (unsigned int z = 0; z < TERRAIN_VIEW_RANGE; ++z)
				GenerateChunk((unsigned int[]) {x, y, z}, (unsigned int[]) {x, y, z});
	
	for (unsigned int y = 0; y < TERRAIN_HEIGHT; ++y)
		for (unsigned int x = 0; x < TERRAIN_VIEW_RANGE; ++x)
			for (unsigned int z = 0; z < TERRAIN_VIEW_RANGE; ++z)
				CreateChunkFaces((unsigned int[]){x, y, z});
	
	voxelTerrain.quadVao = cm_get_unit_quad();
	voxelTerrain.ssbo = cm_load_ssbo(64, sizeof(VoxelBuffer), &voxelTerrain.voxelBuffer, false);
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
	cm_begin_shader_mode(voxelTerrain.shader);

	for (int i = 0; i < 6; ++i)
		cm_set_texture(voxelTerrain.u_surfaceTex + i, voxelTerrain.textures[i].id, i);

	UniformData data = {0};

	for (unsigned int y = 0; y < TERRAIN_HEIGHT; ++y)
	{
		for (unsigned int x = 0; x < TERRAIN_VIEW_RANGE; ++x)
		{
			for (unsigned int z = 0; z < TERRAIN_VIEW_RANGE; ++z)
			{
				data.chunk[0] = x;
				data.chunk[1] = y;
				data.chunk[2] = z;
				data.chunkId = GetChunkId((unsigned int[]){x, y, z});

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
	for (int i = 0; i < 6; ++i) cm_unload_texture(voxelTerrain.textures[i]);

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
	voxelTerrain.u_outlineColor = cm_get_uniform_location(voxelTerrain.shader, "u_outlineColor");

	Path mapPath0 = TO_RES_PATH(mapPath0, "2d/terrain/0.Map_Front.png");
	Path mapPath1 = TO_RES_PATH(mapPath1, "2d/terrain/1.Map_Back.png");
	Path mapPath2 = TO_RES_PATH(mapPath2, "2d/terrain/2.Map_Right.png");
	Path mapPath3 = TO_RES_PATH(mapPath3, "2d/terrain/3.Map_Left.png");
	Path mapPath4 = TO_RES_PATH(mapPath4, "2d/terrain/4.Map_Top.png");
	Path mapPath5 = TO_RES_PATH(mapPath5, "2d/terrain/5.Map_Bottom.png");

	voxelTerrain.textures[0] = cm_load_texture(mapPath0, CM_TEXTURE_WRAP_REPEAT, CM_TEXTURE_FILTER_LINEAR);
	voxelTerrain.textures[1] = cm_load_texture(mapPath1, CM_TEXTURE_WRAP_REPEAT, CM_TEXTURE_FILTER_LINEAR);
	voxelTerrain.textures[2] = cm_load_texture(mapPath2, CM_TEXTURE_WRAP_REPEAT, CM_TEXTURE_FILTER_LINEAR);
	voxelTerrain.textures[3] = cm_load_texture(mapPath3, CM_TEXTURE_WRAP_REPEAT, CM_TEXTURE_FILTER_LINEAR);
	voxelTerrain.textures[4] = cm_load_texture(mapPath4, CM_TEXTURE_WRAP_REPEAT, CM_TEXTURE_FILTER_LINEAR);
	voxelTerrain.textures[5] = cm_load_texture(mapPath5, CM_TEXTURE_WRAP_REPEAT, CM_TEXTURE_FILTER_LINEAR);
}

static void InitNoise()
{
	//3D
	voxelTerrain.noise3D = fnlCreateState();
	voxelTerrain.noise3D.noise_type = FNL_NOISE_PERLIN;
	voxelTerrain.noise3D.seed = rand() % 10000000;
	voxelTerrain.noise3D.fractal_type = FNL_FRACTAL_PINGPONG;
	voxelTerrain.noise3D.gain = TERRAIN_3D_GAIN;
	voxelTerrain.noise3D.lacunarity = TERRAIN_3D_LACUNARITY;
	voxelTerrain.noise3D.frequency = TERRAIN_3D_PERLIN_FREQUENCY;

	//2D
	voxelTerrain.noise2D = fnlCreateState();
	voxelTerrain.noise2D.noise_type = FNL_NOISE_PERLIN;
	voxelTerrain.noise2D.seed = rand() % 10000000;
	voxelTerrain.noise2D.fractal_type = FNL_FRACTAL_FBM;
	voxelTerrain.noise2D.octaves = TERRAIN_2D_OCTAVES;
	voxelTerrain.noise2D.gain = TERRAIN_2D_GAIN;
	voxelTerrain.noise2D.lacunarity = TERRAIN_2D_LACUNARITY;
	voxelTerrain.noise2D.frequency = TERRAIN_2D_PERLIN_FREQUENCY;
}

static void CreateChunkFaces(const unsigned int chunk[3])
{
	unsigned int chunkId = GetChunkId(chunk);

	unsigned int xStart = chunk[0] * TERRAIN_CHUNK_SIZE, xEnd = xStart + TERRAIN_CHUNK_SIZE;
	unsigned int yStart = chunk[1] * TERRAIN_CHUNK_SIZE, yEnd = yStart + TERRAIN_CHUNK_SIZE;
	unsigned int zStart = chunk[2] * TERRAIN_CHUNK_SIZE, zEnd = zStart + TERRAIN_CHUNK_SIZE;

	unsigned int faceCount = 0;
	unsigned int faceOffset = chunkId * TERRAIN_CHUNK_CUBE_COUNT;
	
	for (unsigned int y = yStart; y < yEnd; y++)
	{
		for (unsigned int x = xStart; x < xEnd; x++)
		{
			for (unsigned int z = zStart; z < zEnd; z++)
			{
				unsigned int cubeId = y * FULL_HORIZONTAL_SLICE + x * FULL_AXIS_SIZE + z;
				unsigned char currentCell = voxelTerrain.cells[cubeId];
				if(currentCell == 0) continue;

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

				unsigned int blockIndex = (currentCell / TERRAIN_MAX_AXIS_BLOCK_TYPES) << 4 |
										  (currentCell % TERRAIN_MAX_AXIS_BLOCK_TYPES);

				blockIndex <<= 15;
				blockIndex |= ((x % TERRAIN_CHUNK_SIZE) << 10) |
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

static void GenerateChunk(const unsigned int chunkId[3], const unsigned int destination[3])
{
	//region defines
#define SET_CHUNK(t) for (unsigned int y = yStart; y < yEnd; ++y)\
				     {\
				     	for (unsigned int x = xStart; x < xEnd; ++x)\
				     	{\
				     		unsigned int xyId = y * FULL_HORIZONTAL_SLICE + x * FULL_AXIS_SIZE;\
				     		memset(&voxelTerrain.cells[xyId + zStart], t, TERRAIN_CHUNK_SIZE);\
				     	}\
				     }
	//endregion

	unsigned int xStart = destination[0] * TERRAIN_CHUNK_SIZE, xEnd = xStart + TERRAIN_CHUNK_SIZE;
	unsigned int yStart = destination[1] * TERRAIN_CHUNK_SIZE, yEnd = yStart + TERRAIN_CHUNK_SIZE;
	unsigned int zStart = destination[2] * TERRAIN_CHUNK_SIZE, zEnd = zStart + TERRAIN_CHUNK_SIZE;

	//clean Chunk

	SET_CHUNK(0)

	for (unsigned int x = xStart; x < xEnd; ++x)
	{
		unsigned int px = chunkId[0] * TERRAIN_CHUNK_SIZE + (x % TERRAIN_CHUNK_SIZE);

		for (unsigned int z = zStart; z < zEnd; ++z)
		{
			unsigned int pz = chunkId[2] * TERRAIN_CHUNK_SIZE + (z % TERRAIN_CHUNK_SIZE);
			unsigned int xzId = x * FULL_AXIS_SIZE + z;
			unsigned int yLimit = glm_imin((int)yEnd, voxelTerrain.heightMap[x * FULL_AXIS_SIZE + z]);

			for (unsigned int y = yStart; y < yLimit; ++y)
			{
				unsigned int py = chunkId[1] * TERRAIN_CHUNK_SIZE + (y % TERRAIN_CHUNK_SIZE);
				float val3D = fnlGetNoise3D(&voxelTerrain.noise3D, (float)(px), (float)(py), (float)(pz));
				val3D = (val3D + 1) * .5f;

				if(val3D >= TERRAIN_CAVE_EDGE)
				{
					unsigned int id = y * FULL_HORIZONTAL_SLICE + xzId;
					voxelTerrain.cells[id] = 1;
				}
			}
		}
	}
}

static void GenerateHeightMap(const unsigned int chunkId[2], const unsigned int destination[2])
{
	unsigned int xStart = destination[0] * TERRAIN_CHUNK_SIZE, xEnd = xStart + TERRAIN_CHUNK_SIZE;
	unsigned int zStart = destination[1] * TERRAIN_CHUNK_SIZE, zEnd = zStart + TERRAIN_CHUNK_SIZE;

	for (unsigned int x = xStart; x < xEnd; ++x)
	{
		unsigned int px = chunkId[0] * TERRAIN_CHUNK_SIZE + (x % TERRAIN_CHUNK_SIZE);
		for (unsigned int z = zStart; z < zEnd; ++z)
		{
			unsigned int pz = chunkId[1] * TERRAIN_CHUNK_SIZE + (z % TERRAIN_CHUNK_SIZE);

			float val2D = fnlGetNoise2D(&voxelTerrain.noise2D, (float)(px), (float)(pz));
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
	cm_set_uniform_vec4(voxelTerrain.u_outlineColor, TERRAIN_OUTLINE_COLOR);
}

static unsigned int GetChunkId(const unsigned int id[3])
{
	return id[1] * TERRAIN_VIEW_RANGE * TERRAIN_VIEW_RANGE + id[0] * TERRAIN_VIEW_RANGE + id[2];
}

//endregion