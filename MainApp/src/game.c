#include "game.h"
#include "coal_miner.h"
#include "coal_miner_internal.h"
#include "camera.h"
#include "terrain.h"

void game_awake()
{
	load_terrain();
	load_camera();
	cm_disable_cursor();
}

void game_update()
{
	update_terrain();
	update_camera();
}

void game_render()
{
	cm_begin_mode_3d(get_camera());
	
	draw_terrain();
	
	cm_end_mode_3d();
}

void game_close()
{
	dispose_camera();
	dispose_terrain();
}
