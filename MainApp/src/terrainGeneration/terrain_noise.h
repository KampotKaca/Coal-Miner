#ifndef TERRAIN_NOISE_H
#define TERRAIN_NOISE_H

#include <stdint-gcc.h>
#include "coal_miner.h"
#include "terrainStructs.h"

void setup_terrain_noise(VoxelTerrain* terrain);
void send_terrain_noise_job(uint32_t x, uint32_t z);

void generate_terrain_height_map(const uint32_t sourceId[2], const uint32_t destination[2]);
void generate_terrain_pre_chunk(uint32_t xId, uint32_t yId, uint32_t zId);
void generate_terrain_post_chunk(uint32_t xId, uint32_t yId, uint32_t zId);

#endif //TERRAIN_NOISE_H
