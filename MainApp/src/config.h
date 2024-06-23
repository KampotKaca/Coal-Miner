#ifndef CONFIG_H
#define CONFIG_H

#define CAM_FREE_MOVE_SPEED 20
#define CAM_FREE_ROTATE_SPEED 10
#define CAM_FREE_ROTATE_MIN_LIMIT 10
#define CAM_FREE_ROTATE_MAX_LIMIT 170

#define GRID_SIZE 512
#define GRID_AXIS_SIZE 1
#define GRID_COLOR ((vec4){ 1, 1, 1, 0.3 })

//region TERRAIN
#define TERRAIN_CHUNK_SIZE 32
#define TERRAIN_VIEW_RANGE 8
#define TERRAIN_HEIGHT 4
#define TERRAIN_MAX_SOLID_VERTICAL_CHUNKS 3
#define TERRAIN_MIN_SOLID_VERTICAL_CHUNKS 1

#define TERRAIN_3D_PERLIN_STEP 12
#define TERRAIN_2D_PERLIN_STEP 20
//endregion

#endif //CONFIG_H
