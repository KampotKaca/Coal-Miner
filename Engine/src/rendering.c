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
//	disable_backface_culling();
//	cm_load_ubo(32, 3 * sizeof(M4x4) + 2 * sizeof(V3), &cameraUbo);
}

void unload_renderer()
{
	unload_ubos();
}

void cm_begin_mode_3d(Camera3D camera, Shader shader)
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

//	printf("x: %f, y: %f, z: %f", camera.up.x, camera.up.y, camera.up.z);
//	cameraUbo.view = m4x4_look_at(camera.position, camera.target, camera.up);

	glm_mat4_mul(cameraUbo.projection, cameraUbo.view, cameraUbo.viewProjection);

//	cameraUbo.position = camera.position;
//	cameraUbo.direction = v3_norm(v3_sub(camera.target, camera.position));

	cm_set_shader_uniform_m4x4(shader, "cameraViewProjection", cameraUbo.projection[0]);

//	enable_depth_test();

//	upload_ubos();
}

void cm_end_mode_3d()
{
	disable_depth_test();           // Disable DEPTH_TEST for 2D
}