#include "grid.h"
#include "coal_miner.h"
#include "config.h"

typedef struct GridData
{
	unsigned int vertices[((GRID_SIZE + 1) + (GRID_SIZE + 1)) * 2];
}GridData;

static void CreateShader();
static void CreateVao();

Shader gridShader;
Vao gridVao;
GridData gridData;

void load_grid()
{
	int id = 0;
	for (unsigned int x = 0; x <= GRID_SIZE; ++x)
	{
		unsigned int vertex = (x << 16);
		gridData.vertices[id] = vertex;
		
		vertex = (x << 16) | GRID_SIZE;
		gridData.vertices[id + 1] = vertex;
		
		id += 2;
	}
	
	for (unsigned int z = 0; z <= GRID_SIZE; ++z)
	{
		unsigned int vertex = z;
		gridData.vertices[id] = vertex;
		
		vertex = (GRID_SIZE << 16) | z;
		gridData.vertices[id + 1] = vertex;
		
		id += 2;
	}
	
	CreateShader();
	CreateVao();
}

void draw_grid()
{
	cm_begin_shader_mode(gridShader);

	cm_set_shader_uniform_f(gridShader, "u_axis_offset", -GRID_SIZE * 0.5f);
	cm_set_shader_uniform_f(gridShader, "u_axis_size", GRID_AXIS_SIZE);
	cm_set_shader_uniform_vec4(gridShader, "u_color", GRID_COLOR);
	cm_draw_vao(gridVao, CM_LINES);

	cm_end_shader_mode();
}

void dispose_grid()
{
	cm_unload_vao(gridVao);
	cm_unload_shader(gridShader);
}

static void CreateShader()
{
	char vsPath[MAX_PATH_SIZE] = RES_PATH;
	strcat_s(vsPath, MAX_PATH_SIZE, "shaders/grid.vert");
	
	char fsPath[MAX_PATH_SIZE] = RES_PATH;
	strcat_s(fsPath, MAX_PATH_SIZE, "shaders/grid.frag");
	
	gridShader = cm_load_shader(vsPath, fsPath);
}

static void CreateVao()
{
	VaoAttribute attributes[] =
	{
		{ 1, CM_UINT, false, 1 * sizeof(unsigned int) },
	};
	
	Vbo vbo = { 0 };
	vbo.id = 0;
	vbo.isStatic = true;
	vbo.data = gridData.vertices;
	vbo.vertexCount = ((GRID_SIZE + 1) + (GRID_SIZE + 1)) * 2;
	vbo.dataSize = sizeof(gridData.vertices);
	Ebo ebo = {0};
	vbo.ebo = ebo;
	
	gridVao = cm_load_vao(attributes, 1, vbo);
}