#include "game.h"
#include "coal_miner.h"

Shader shader;
Camera3D camera;
Vao vao;

float vertices[] =
{
	-0.5f, -0.5f, -0.5f, 0.0f, 0.0f, 0.0f,  // bottom left
	0.5f, -0.5f, -0.5f,  1.0f, 1.0f, 0.0f, // top left
	0.5f, 0.5f, -0.5f,  0.0f, 1.0f, 0.0f, // top right
	-0.5f, 0.5f, -0.5f,  1.0f, 0.0f, 0.0f, // bottom right

	-0.5f, -0.5f, 0.5f, 0.0f, 0.0f, 1.0f,  // bottom left
	0.5f, -0.5f, 0.5f,  1.0f, 1.0f, 1.0f, // top left
	0.5f, 0.5f, 0.5f,  0.0f, 1.0f, 1.0f, // top right
	-0.5f, 0.5f, 0.5f,  1.0f, 0.0f, 1.0f, // bottom right
};

unsigned short indices[] =
{
	0, 1, 2, // Front face
	0, 2, 3,
	1, 5, 6, // Right face
	1, 6, 2,
	5, 4, 7, // Back face
	5, 7, 6,
	4, 0, 3, // Left face
	4, 3, 7,
	3, 2, 6, // Top face
	3, 6, 7,
	4, 5, 1, // Bottom face
	4, 1, 0
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
	camera.projection = CAMERA_PERSPECTIVE;
	camera.fov = 45;

	glm_vec3((vec3){ 0, 0, 0 }, camera.target);
	glm_vec3((vec3){ 0, 0, 3 }, camera.position);
	glm_vec3((vec3){ 0, 1, 0 }, camera.up);

	VaoAttribute attributes[] =
	{
		{ 3, CM_FLOAT, false, 3 * sizeof(float) },
		{ 3, CM_FLOAT, false, 3 * sizeof(float) }
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
	ebo.type = CM_USHORT;
	ebo.indexCount = 36;
	vbo.ebo = ebo;

	vao = cm_load_vao(attributes, 2, vbo);
}

versor rotation = { 0 };

void game_update()
{

}

void game_render()
{
	cm_begin_shader_mode(shader);
	cm_begin_mode_3d(camera, shader);
	mat4 model = { 0 };
	glm_translate(model, (vec3){ 0, 0, 0 });
	glm_quat_rotate(model, rotation, model);
	glm_scale(model, (vec3){1, 1, 1});
	cm_set_shader_uniform_m4x4(shader, "model", model[0]);

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
