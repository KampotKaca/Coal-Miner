#ifndef TERRAIN_H
#define TERRAIN_H

#include "coal_miner.h"
#include "FastNoiseLite.h"

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

unsigned char get_block_type(float caveValue);
fnl_state get_cave_noise();
fnl_state get_biome_flat_noise();
fnl_state get_biome_small_hill_noise();
fnl_state get_biome_hill_noise();
fnl_state get_biome_mountain_noise();
fnl_state get_biome_high_mountain_noise();

#endif //TERRAIN_H
