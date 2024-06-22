#include "cmrendering.h"
#include "coal_miner.h"
#include "cmgl.h"

struct CameraUbo
{
	mat4 projection;
	mat4 view;
	mat4 viewProjection;
	vec3 position;
	vec3 direction;
};

Window* CM_RN_WIN_P;
struct CameraUbo CM_CAMERA_UBO;

const unsigned int cmQuadVertices[4] =
{
	0b00, 0b01, 0b11, 0b10
};

const unsigned int cmQuadTriangles[6] =
{
	0, 1, 2,
	0, 2, 3,
};

Vao cmQuad;

static void CreateQuad();

void load_renderer(Window* wPtr)
{
	CM_RN_WIN_P = wPtr;
	
	cm_load_ubo(CAMERA_UBO_BINDING_ID, 3 * sizeof(mat4) + 2 * sizeof(vec3), &CM_CAMERA_UBO);
	CreateQuad();
}

void unload_renderer()
{
	cm_unload_vao(cmQuad);
	unload_ubos();
}

void cm_begin_mode_3d(Camera3D camera)
{
	float aspect = (float)CM_RN_WIN_P->currentFbo.width / (float)CM_RN_WIN_P->currentFbo.height;

	// NOTE: zNear and zFar values are important when computing depth buffer values
	if (camera.projection == CAMERA_PERSPECTIVE)
	{
		glm_perspective(glm_rad(camera.fov), aspect, camera.nearPlane, camera.farPlane, CM_CAMERA_UBO.projection);
	}
	else if (camera.projection == CAMERA_ORTHOGRAPHIC)
	{
		// Setup orthographic projection
		float top = camera.fov * .5f;
		float right = top*aspect;

		glm_ortho(-right, right, -top, top, camera.nearPlane, camera.farPlane, CM_CAMERA_UBO.projection);
	}
	
	glm_normalize(camera.direction);
	
//	vec3 invUp;
//	glm_vec3_negate(camera.up);
	glm_look(camera.position, camera.direction, camera.up, CM_CAMERA_UBO.view);
	glm_scale(CM_CAMERA_UBO.view, (vec3){ -1, 1, 1 });
	glm_mat4_mul(CM_CAMERA_UBO.projection, CM_CAMERA_UBO.view, CM_CAMERA_UBO.viewProjection);

	glm_vec3(camera.position, CM_CAMERA_UBO.position);
	glm_vec3(camera.direction, CM_CAMERA_UBO.direction);

	upload_ubos();
	cm_enable_depth_test();
}

void cm_end_mode_3d()
{
	cm_disable_depth_test();           // Disable DEPTH_TEST for 2D
}

Vao cm_get_unit_quad() { return cmQuad; }

static void CreateQuad()
{
	VaoAttribute attributes[] =
	{
		{ 1, CM_UINT, false, 1 * sizeof(unsigned int) },
	};

	Vbo vbo = { 0 };
	vbo.id = 0;
	vbo.isStatic = true;
	vbo.data = cmQuadVertices;
	vbo.vertexCount = 4;
	vbo.dataSize = sizeof(unsigned int) * 4;
	Ebo ebo = {0};
	ebo.id = 0;
	ebo.isStatic = true;
	ebo.dataSize = sizeof(unsigned int) * 6;
	ebo.data = cmQuadTriangles;
	ebo.type = CM_UINT;
	ebo.indexCount = 6;
	vbo.ebo = ebo;

	cmQuad = cm_load_vao(attributes, 1, vbo);
}