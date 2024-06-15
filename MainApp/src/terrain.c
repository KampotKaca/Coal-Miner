#include "terrain.h"

Shader shader;
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
	0, 2, 1, // Front face
	0, 3, 2,
	1, 6, 5, // Right face
	1, 2, 6,
	5, 7, 4, // Back face
	5, 6, 7,
	4, 3, 0, // Left face
	4, 7, 3,
	3, 6, 2, // Top face
	3, 7, 6,
	4, 1, 5, // Bottom face
	4, 0, 1,
};

Transform trs = TRANSFORM_INIT;

static void CreateShader();
static void CreateVao();

//region Callback Functions
void load_terrain()
{
	CreateShader();
	CreateVao();
}

void update_terrain()
{
	versor rot;
	glm_quat(rot, 10 * cm_delta_time_f(), .2f, .8f, .3f);
	glm_quat_mul(trs.rotation, rot, trs.rotation);
//	trs.position[2] += 2 * cm_delta_time_f();
}

void draw_terrain()
{
	cm_begin_shader_mode(shader);
	
	mat4 model;
	cm_get_transformation(&trs, model);
	
	cm_set_shader_uniform_m4x4(shader, "model", model[0]);
	
	cm_draw_vao(vao);
	
	cm_end_shader_mode();
}

void dispose_terrain()
{
	cm_unload_vao(vao);
	cm_unload_shader(shader);
}
//endregion

//region Local Functions

static void CreateShader()
{
	char vsPath[MAX_PATH_SIZE] = RES_PATH;
	strcat_s(vsPath, MAX_PATH_SIZE, "shaders/default.vert");
	
	char fsPath[MAX_PATH_SIZE] = RES_PATH;
	strcat_s(fsPath, MAX_PATH_SIZE, "shaders/default.frag");
	
	shader = cm_load_shader(vsPath, fsPath);
}

static void CreateVao()
{
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

//endregion