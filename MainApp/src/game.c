#include "game.h"
#include "coal_miner.h"

Shader shader;
Camera3D camera;

void game_awake()
{
	char vsPath[MAX_PATH_SIZE] = RES_PATH;
	strcat_s(vsPath, MAX_PATH_SIZE, "shaders/default.vert");

	char fsPath[MAX_PATH_SIZE] = RES_PATH;
	strcat_s(fsPath, MAX_PATH_SIZE, "shaders/default.frag");

	shader = cm_load_shader(vsPath, fsPath);
	camera = (Camera3D){ 0 };
	camera.nearPlane = .01f;
	camera.farPlane = 100;
	camera.target = v3_zero();
	camera.position = (V3) { 0, 5, -5 };
	camera.projection = CAMERA_PERSPECTIVE;
	camera.fov = 45;
}

void game_update()
{

}

void game_render()
{
	cm_begin_mode_3d(camera);
	cm_end_mode_3d();
}

void game_frame_end()
{

}

void game_close()
{
	cm_unload_shader(shader);
}
