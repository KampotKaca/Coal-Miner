#ifndef TERRAIN_MESHING_H
#define TERRAIN_MESHING_H

#include "coal_miner.h"
#include "terrainStructs.h"

void setup_terrain_meshing(VoxelTerrain* terrain);
void send_terrain_face_creation_job(uint32_t x, uint32_t y, uint32_t z);
void send_terrain_group_face_creation_job(uint32_t x, uint32_t z);
void create_terrain_chunk_faces(uint32_t xId, uint32_t yId, uint32_t zId);

#endif //TERRAIN_MESHING_H
