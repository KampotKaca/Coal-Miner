#include "game.h"
#include "coal_miner.h"

Shader shader;
Camera3D camera;
Vao vao;

float vertices[] =
{
	-0.5f, -0.5f, 0.0f, 0.0f, 0.0f,  // bottom left
	-0.5f, 0.5f, 0.0f, 1.0f, 1.0f, // top left
	0.5f,  0.5f, 0.0f, 0.0f, 1.0f, // top right
	0.5f, -0.5f, 0.0f, 1.0f, 0.0f, // bottom right
};

unsigned int indices[] =
{
	0, 1, 2,
	0, 2, 3,
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
		{ 3, CM_FLOAT, false, 3 * sizeof(float) },
		{ 2, CM_FLOAT, false, 2 * sizeof(float) }
	};

	Vbo vbo = { 0 };
	vbo.id = 0;
	vbo.isStatic = false;
	vbo.data = vertices;
	vbo.dataSize = sizeof(vertices);
	Ebo ebo = {0};
	ebo.id = 0;
	ebo.isStatic = false;
	ebo.dataSize = sizeof(indices);
	ebo.data = indices;
	ebo.type = CM_UINT;
	ebo.indexCount = 6;
	vbo.ebo = ebo;

	vao = cm_load_vao(attributes, 2, vbo);
}

void game_update()
{

}

void game_render()
{
	cm_begin_shader_mode(shader);
	cm_begin_mode_3d(camera, shader);

	cm_draw_vao(vao);

	cm_end_mode_3d();
	cm_end_shader_mode();
}

void game_frame_end()
{

}

void game_close()
{
	cm_unload_vao(vao);
	cm_unload_shader(shader);
}
