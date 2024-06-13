#include "rendering.h"
#include "coal_miner.h"
#include "coal_gl.h"

struct CameraUbo
{
	mat4 projection;
	mat4 view;
	mat4 viewProjection;
	vec3 position;
	vec3 direction;
};

Window* windowPtr;
struct CameraUbo cameraUbo;

void load_renderer(Window* wPtr)
{
	windowPtr = wPtr;
	cm_load_ubo(CAMERA_UBO_BINDING_ID, 3 * sizeof(mat4) + 2 * sizeof(vec3), &cameraUbo);
}

void unload_renderer()
{
	unload_ubos();
}

void cm_begin_mode_3d(Camera3D camera)
{
	float aspect = (float)windowPtr->currentFbo.width / (float)windowPtr->currentFbo.height;

	// NOTE: zNear and zFar values are important when computing depth buffer values
	if (camera.projection == CAMERA_PERSPECTIVE)
	{
		glm_perspective(glm_rad(camera.fov), aspect, camera.nearPlane, camera.farPlane, cameraUbo.projection);
	}
	else if (camera.projection == CAMERA_ORTHOGRAPHIC)
	{
		// Setup orthographic projection
		float top = camera.fov * .5f;
		float right = top*aspect;

		glm_ortho(-right, right, -top, top, camera.nearPlane, camera.farPlane, cameraUbo.projection);
	}

	glm_lookat(camera.position, camera.target, camera.up, cameraUbo.view);
	glm_mat4_mul(cameraUbo.projection, cameraUbo.view, cameraUbo.viewProjection);

	glm_vec3(camera.position, cameraUbo.position);
	vec3 direction;
	glm_vec3_sub(camera.target, camera.position, direction);
	glm_normalize(direction);
	glm_vec3(direction, cameraUbo.direction);

	upload_ubos();
	enable_depth_test();
}

void cm_end_mode_3d()
{
	disable_depth_test();           // Disable DEPTH_TEST for 2D
}