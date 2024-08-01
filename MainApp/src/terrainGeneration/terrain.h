#ifndef TERRAIN_H
#define TERRAIN_H

#include "coal_miner.h"

typedef enum
{
	BLOCK_EMPTY,
	BLOCK_GRASS,
	BLOCK_DIRT,
	BLOCK_STONE,
	BLOCK_IRON,
}BlockType;

typedef enum
{
	BLOCK_GEN_EMPTY,
	BLOCK_GEN_STONE,
	BLOCK_GEN_DIRT,
	BLOCK_GEN_IRON,
	BLOCK_GEN_COUNT
}GeneratedBlockType;

typedef enum
{
	BIOME_FLAT,
	BIOME_SMALL_HILL,
	BIOME_HILL,
	BIOME_MOUNTAIN,
	BIOME_HIGH_MOUNTAIN,
	BIOME_COUNT,
}BiomeElevationType;

void load_terrain();
bool loading_terrain();
void update_terrain();
void draw_terrain();
void dispose_terrain();

#endif //TERRAIN_H
