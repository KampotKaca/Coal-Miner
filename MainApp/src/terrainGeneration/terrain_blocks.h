#ifndef TERRAIN_BLOCKS_H
#define TERRAIN_BLOCKS_H

#include <FastNoiseLite.h>
#include "terrainStructs.h"

unsigned char get_terrain_block_type(float caveValue);
fnl_state get_terrain_cave_noise();
fnl_state get_terrain_biome_flat_noise();
fnl_state get_terrain_biome_small_hill_noise();
fnl_state get_terrain_biome_hill_noise();
fnl_state get_terrain_biome_mountain_noise();
fnl_state get_terrain_biome_high_mountain_noise();

#endif //TERRAIN_BLOCKS_H
