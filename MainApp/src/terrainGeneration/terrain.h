#ifndef TERRAIN_H
#define TERRAIN_H

#include "coal_miner.h"

typedef enum BlockTypes
{
	BLOCK_EMPTY,
	BLOCK_GRASS,
	BLOCK_DIRT,
	BLOCK_STONE,
	BLOCK_IRON,
}BlockTypes;

typedef enum GeneratedBlockTypes
{
	BLOCK_GEN_EMPTY,
	BLOCK_GEN_STONE,
	BLOCK_GEN_DIRT,
	BLOCK_GEN_IRON,
	BLOCK_GEN_COUNT
}GeneratedBlockTypes;

void load_terrain();
void update_terrain();
void draw_terrain();
void dispose_terrain();

void load_block_types();
unsigned char get_block_type(float caveValue);

#endif //TERRAIN_H
