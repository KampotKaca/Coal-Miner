#ifndef CONFIG_H
#define CONFIG_H

#define CAM_FREE_MOVE_SPEED 20
#define CAM_FREE_ROTATE_SPEED 10
#define CAM_FREE_ROTATE_MIN_LIMIT 10
#define CAM_FREE_ROTATE_MAX_LIMIT 170

#define DRAW_GRID
#define GRID_SIZE 512
#define GRID_AXIS_SIZE 1
#define GRID_COLOR ((vec4){ 1, 1, 1, 0.3 })

//region TERRAIN
#define TERRAIN_CHUNK_SIZE 32
#define TERRAIN_VIEW_RANGE 16
#define TERRAIN_HEIGHT 8
#define TERRAIN_LOWER_EDGE 3
#define TERRAIN_UPPER_EDGE 7

#define TERRAIN_HEIGHT_FREQUENCY .01f
#define TERRAIN_HEIGHT_OCTAVES 6
#define TERRAIN_HEIGHT_GAIN .5f
#define TERRAIN_HEIGHT_LACUNARITY 2

#define TERRAIN_CAVE_FREQUENCY .01f
#define TERRAIN_CAVE_GAIN .5f
#define TERRAIN_CAVE_LACUNARITY 2

#define TERRAIN_MAX_AXIS_BLOCK_TYPES 16
#define TERRAIN_MAX_BLOCK_TYPES (TERRAIN_MAX_AXIS_BLOCK_TYPES * TERRAIN_MAX_AXIS_BLOCK_TYPES)

#define TERRAIN_CAVE_EDGE (-.22f)

//endregion

#endif //CONFIG_H
