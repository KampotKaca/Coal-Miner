#include "game.h"
#include "config.h"
#include "coal_miner.h"
#include "coal_miner_internal.h"
#include "terrainGeneration/terrain.h"
#include "grid.h"

#include "camera.h"
#include "light.h"

void game_awake()
{
	log_set_level(LOG_INFO);

#if defined(DRAW_GRID)
	load_grid();
#endif

	load_light();
	load_camera();
	load_terrain();
	cm_disable_cursor();
}

bool game_loading()
{
	return loading_terrain();
}

void game_update()
{
	update_terrain();
	update_light();
	update_camera();

//	log_info("target: %i, frame Rate: %i", cm_get_target_frame_rate(), cm_frame_rate());
}

void game_render()
{
	cm_begin_mode_3d(get_camera());
	cm_set_global_light(get_global_light());
	cm_upload_ubos();
	
#if defined(DRAW_GRID)
	draw_grid();
#endif
	draw_terrain();
	
	cm_end_mode_3d();
}

void game_close()
{
	dispose_grid();
	dispose_light();
	dispose_camera();
	dispose_terrain();
}
