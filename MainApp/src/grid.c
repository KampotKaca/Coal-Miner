#include "grid.h"
#include "coal_miner.h"
#include "config.h"
#include "terrainGeneration/terrainConfig.h"

typedef struct GridVertex
{
	unsigned int vertex;
	unsigned int colorId;
}GridVertex;

typedef struct GridData
{
	Shader gridShader;
	Vao gridVao;
	int u_axis_offsetId, u_axis_sizeId, u_colorId;
	
	GridVertex vertices[((GRID_SIZE + 1) + (GRID_SIZE + 1)) * 2];
}GridData;

static void CreateGridShader();
static void CreateGridVao();

GridData gridData;

void load_grid()
{
	int id = 0;
	for (unsigned int x = 0; x <= GRID_SIZE; ++x)
	{
		unsigned int vertex = (x << 16);
		
		GridVertex vStart =
		{
			.vertex = vertex,
			.colorId = 0
		};
		gridData.vertices[id] = vStart;
		
		vertex = (x << 16) | GRID_SIZE;
		
		GridVertex vEnd =
		{
			.vertex = vertex,
			.colorId = 1
		};
		gridData.vertices[id + 1] = vEnd;
		
		id += 2;
	}
	
	for (unsigned int z = 0; z <= GRID_SIZE; ++z)
	{
		unsigned int vertex = z;
		GridVertex vStart =
		{
			.vertex = vertex,
			.colorId = 0
		};
		gridData.vertices[id] = vStart;
		
		vertex = (GRID_SIZE << 16) | z;
		GridVertex vEnd =
		{
			.vertex = vertex,
			.colorId = 2
		};
		
		gridData.vertices[id + 1] = vEnd;
		
		id += 2;
	}

	CreateGridShader();
	CreateGridVao();
}

void draw_grid()
{
	cm_begin_shader_mode(gridData.gridShader);

	cm_set_uniform_f(gridData.u_axis_offsetId, -GRID_SIZE * 0.5f);
	cm_set_uniform_f(gridData.u_axis_sizeId, TERRAIN_CHUNK_SIZE);
	cm_set_uniform_vec4(gridData.u_colorId, GRID_COLOR);
	
	cm_draw_vao(gridData.gridVao, CM_LINES);

	cm_end_shader_mode();
}

void dispose_grid()
{
	cm_unload_vao(gridData.gridVao);
	cm_unload_shader(gridData.gridShader);
}

static void CreateGridShader()
{
	Path vsPath = TO_RES_PATH(vsPath, "shaders/grid.vert");
	Path fsPath = TO_RES_PATH(fsPath, "shaders/grid.frag");
	
	gridData.gridShader = cm_load_shader(vsPath, fsPath);
	gridData.u_axis_offsetId = cm_get_uniform_location(gridData.gridShader, "u_axis_offset");
	gridData.u_axis_sizeId = cm_get_uniform_location(gridData.gridShader, "u_axis_size");
	gridData.u_colorId = cm_get_uniform_location(gridData.gridShader, "u_color");
}

static void CreateGridVao()
{
	VaoAttribute attributes[] =
	{
		{ 1, CM_UINT, false, 1 * sizeof(unsigned int) },
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
	
	gridData.gridVao = cm_load_vao(attributes, 2, vbo);
}