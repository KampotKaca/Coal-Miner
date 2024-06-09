#include "game.h"
#include "coal_miner.h"

Shader shader;
Camera3D camera;
Vao vao;

float vertices[] =
{
	0.5f,  0.5f, 0.0f,  // top right
	0.5f, -0.5f, 0.0f,  // bottom right
	-0.5f, -0.5f, 0.0f,  // bottom left
};

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
	camera.up = (V3){ 0, 1, 0 };
	camera.fov = 45;

	VaoAttribute attributes[] =
	{
		{ 3, CM_FLOAT, false, 3 * sizeof(float) }
	};

	Vbo vbos[] =
	{
		{ 0, 9 * sizeof(float), true, vertices }
	};
	vao = cm_load_vao(attributes, 1, vbos, 1);
}

void game_update()
{

}

void game_render()
{
//	cm_begin_mode_3d(camera);
	cm_begin_shader_mode(shader);

	cm_draw_vao(vao);

	cm_end_shader_mode();
//	cm_end_mode_3d();
}

void game_frame_end()
{

}

void game_close()
{
	cm_unload_vao(vao);
	cm_unload_shader(shader);
}
