#include "game.h"
#include "coal_miner.h"

Shader shader;

void game_awake()
{
	char vsPath[MAX_PATH_SIZE] = RES_PATH;
	strcat_s(vsPath, MAX_PATH_SIZE, "shaders/default.vert");

	char fsPath[MAX_PATH_SIZE] = RES_PATH;
	strcat_s(fsPath, MAX_PATH_SIZE, "shaders/default.frag");

	shader = cm_load_shader(vsPath, fsPath);
}

void game_update()
{

}

void game_render()
{

}

void game_frame_end()
{

}

void game_close()
{
	cm_unload_shader(shader);
}
