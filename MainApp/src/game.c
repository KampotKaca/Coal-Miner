#include "game.h"
#include "config.h"
#include "coal_miner.h"
#include "coal_miner_internal.h"
#include "camera.h"
#include "terrainGeneration/terrain.h"
#include "grid.h"

void game_awake()
{
#if defined(DRAW_GRID)
	load_grid();
#endif

	load_terrain();
	load_camera();
	cm_disable_cursor();
}

void game_update()
{
	update_terrain();
	update_camera();
	
//	printf("target: %i, frame Rate: %i\n", cm_get_target_frame_rate(), cm_frame_rate());
}

void game_render()
{
	cm_begin_mode_3d(get_camera());

#if defined(DRAW_GRID)
	draw_grid();
#endif
	draw_terrain();
	
	cm_end_mode_3d();
}

void game_close()
{
	dispose_grid();
	dispose_camera();
	dispose_terrain();
}
