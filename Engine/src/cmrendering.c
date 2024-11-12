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

struct GlobalLightUbo
{
	vec3 lightDirection;
	vec3 lightColor;
	float lightLuminosity;
};

Window* CM_RN_WIN_P;
struct CameraUbo CM_CAMERA_UBO;
struct GlobalLightUbo CM_GLOBAL_LIGHT_UBO;
Frustum CM_MAIN_FRUSTUM;

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
	
	cm_load_ubo(CAMERA_UBO_BINDING_ID, sizeof(struct CameraUbo), &CM_CAMERA_UBO);
	cm_load_ubo(GLOBAL_LIGHT_UBO_BINDING_ID, sizeof(struct GlobalLightUbo), &CM_GLOBAL_LIGHT_UBO);
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
//	glm_scale(CM_CAMERA_UBO.view, (vec3){ -1, 1, 1 });
	glm_mat4_mul(CM_CAMERA_UBO.projection, CM_CAMERA_UBO.view, CM_CAMERA_UBO.viewProjection);

	glm_vec3_copy(camera.position, CM_CAMERA_UBO.position);
	glm_vec3_copy(camera.direction, CM_CAMERA_UBO.direction);
	
	cm_enable_depth_test();

	vec4 planes[6];
	glm_frustum_planes(CM_CAMERA_UBO.viewProjection, planes);
	
	for (int i = 0; i < 6; ++i)
		memcpy(CM_MAIN_FRUSTUM, planes, 6 * sizeof(vec4));
}

void cm_set_global_light(GlobalLight light)
{
	glm_vec3_copy(light.direction, CM_GLOBAL_LIGHT_UBO.lightDirection);
	glm_vec3_copy(light.color, CM_GLOBAL_LIGHT_UBO.lightColor);
	CM_GLOBAL_LIGHT_UBO.lightLuminosity = light.luminosity;
}

void cm_end_mode_3d()
{
	cm_disable_depth_test();           // Disable DEPTH_TEST for 2D
}

Frustum* cm_get_frustum() { return &CM_MAIN_FRUSTUM; }

bool cm_is_in_main_frustum(BoundingVolume* volume)
{
	vec3 min, max;
	glm_vec3_add(volume->center, volume->extents, max);
	glm_vec3_sub(volume->center, volume->extents, min);

	for(int i = 0; i < 6; i++)
	{
		vec3 normal;
		glm_vec3_copy(CM_MAIN_FRUSTUM[i].normal, normal);
		float distance = CM_MAIN_FRUSTUM[i].distance;

		if ((glm_vec3_dot(normal, min) + distance > 0) ||
		    (glm_vec3_dot(normal, max) + distance > 0) ||
		    (glm_vec3_dot(normal, (vec3){min[0], max[1], min[2]}) + distance > 0) ||
		    (glm_vec3_dot(normal, (vec3){max[0], min[1], min[2]}) + distance > 0) ||
		    (glm_vec3_dot(normal, (vec3){min[0], min[1], max[2]}) + distance > 0) ||
		    (glm_vec3_dot(normal, (vec3){max[0], max[1], min[2]}) + distance > 0) ||
		    (glm_vec3_dot(normal, (vec3){min[0], max[1], max[2]}) + distance > 0) ||
		    (glm_vec3_dot(normal, (vec3){max[0], min[1], max[2]}) + distance > 0) ||
		    (glm_vec3_dot(normal, (vec3){max[0], max[1], max[2]}) + distance > 0))
			continue;

		// If we get here, it isn't in the frustum
		return false;
	}

	return true;
}

bool cm_is_on_or_forward_plane(Plane* plane, BoundingVolume* volume)
{
	vec3 absNormal;
	glm_vec3_abs(plane->normal, absNormal);
	const float r = volume->extents[0] * absNormal[0] +
	                volume->extents[1] * absNormal[1] +
					volume->extents[2] * absNormal[2];

	return -r <= glm_dot(plane->normal, volume->center) - plane->distance;
}

bool cm_is_on_or_backward_plane(Plane* plane, BoundingVolume* volume)
{
	vec3 absNormal;
	glm_vec3_abs(plane->normal, absNormal);
	const float r = volume->extents[0] * absNormal[0] +
	                volume->extents[1] * absNormal[1] +
	                volume->extents[2] * absNormal[2];

	return -r >= glm_dot(plane->normal, volume->center) - plane->distance;
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
	vbo.data = cmQuadVertices;
	vbo.vertexCount = 4;
	vbo.dataSize = sizeof(unsigned int) * 4;
	Ebo ebo = {0};
	ebo.id = 0;
	ebo.dataSize = sizeof(unsigned int) * 6;
	ebo.data = cmQuadTriangles;
	ebo.type = CM_UINT;
	ebo.indexCount = 6;
	vbo.ebo = ebo;

	cmQuad = cm_load_vao(attributes, 1, vbo);
}