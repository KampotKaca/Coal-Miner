#ifndef TERRAIN_STRUCTS_H
#define TERRAIN_STRUCTS_H

#include <FastNoiseLite.h>
#include "coal_miner.h"
#include "terrain.h"
#include "terrainConfig.h"

#define TERRAIN_CHUNK_VOXEL_COUNT TERRAIN_CHUNK_SIZE * TERRAIN_CHUNK_SIZE * TERRAIN_CHUNK_SIZE
#define TERRAIN_CHUNK_COUNT TERRAIN_VIEW_RANGE * TERRAIN_VIEW_RANGE * TERRAIN_HEIGHT
#define TERRAIN_CHUNK_HORIZONTAL_SLICE TERRAIN_CHUNK_SIZE * TERRAIN_CHUNK_SIZE
#define TERRAIN_MIN_BUFFER_SIZE 128

typedef enum
{
	CHUNK_REQUIRES_FACES,
	CHUNK_CREATING_FACES,
	CHUNK_REQUIRES_UPLOAD,
	CHUNK_READY_TO_DRAW,
}ChunkState;

typedef enum
{
	CHUNK_GROUP_REQUIRES_NOISE_MAP,
	CHUNK_GROUP_GENERATING_NOISE_MAP,
	CHUNK_GROUP_READY,
}ChunkGroupState;

typedef struct
{
	uint32_t chunkId;
	ivec3 chunk;
} UniformData;

struct TerrainChunkFlags
{
	uint32_t isUploaded:1;
	uint32_t yId:4;
	uint32_t state:5;
	uint32_t faceCount:16;
}__attribute__((packed));
typedef struct TerrainChunkFlags TerrainChunkFlags;

typedef struct
{
	TerrainChunkFlags flags;
	List buffer;
	uint8_t* voxels;
}TerrainChunk;

typedef struct
{
	TerrainChunk chunks[TERRAIN_HEIGHT];
	ChunkGroupState state;
	uint32_t id[2];
	uint32_t ssboId;
	uint8_t* heightMap;
	bool isAlive;
}TerrainChunkGroup;

typedef struct
{
	int u_chunkIndex;
	int u_surfaceTex;
	
	int u_ambientColor;
	
}TerrainShaderUniforms;

typedef struct
{
	TerrainShaderUniforms uniforms;
	
	fnl_state caveNoise;
	fnl_state biomes[BIOME_COUNT];

	ThreadPool* pool;

	Shader shader;
	Texture textures[3];
	Ssbo voxelsSsbo;

	ivec2 loadedCenter;
	Vao chunkVaos[TERRAIN_CHUNK_COUNT];
	TerrainChunkGroup chunkGroups[TERRAIN_VIEW_RANGE * TERRAIN_VIEW_RANGE];
	TerrainChunkGroup* shiftGroups;
}VoxelTerrain;

#endif //TERRAIN_STRUCTS_H
