#include "rendering.h"
#include "coal_miner.h"
#include "coal_gl.h"

struct CameraUbo
{
	M4x4 projection;
	M4x4 view;
	M4x4 viewProjection;
	V3 position;
	V3 direction;
};

Window* windowPtr;
struct CameraUbo cameraUbo;

void load_renderer(Window* wPtr)
{
	windowPtr = wPtr;
	cm_load_ubo(32, 3 * sizeof(M4x4) + 2 * sizeof(V3), &cameraUbo);
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
		// Setup perspective projection
		double top = camera.nearPlane * tan(camera.fov * 0.5 * DEG2RAD);
		double right = top*aspect;

		cameraUbo.projection = m4x4_frustum(-right, right, -top, top, camera.nearPlane, camera.farPlane);
	}
	else if (camera.projection == CAMERA_ORTHOGRAPHIC)
	{
		// Setup orthographic projection
		double top = camera.fov / 2.0;
		double right = top*aspect;

		cameraUbo.projection = m4x4_orthographic(-right, right, -top, top, camera.nearPlane, camera.farPlane);
	}

	cameraUbo.view = m4x4_look_at(camera.position, camera.target, camera.up);
	cameraUbo.viewProjection = m4x4_mult(cameraUbo.projection, cameraUbo.view);
	cameraUbo.position = camera.position;
	cameraUbo.direction = v3_norm(v3_sub(camera.target, camera.position));
	enable_depth_test();

	upload_ubos();
}

void cm_end_mode_3d()
{
	disable_depth_test();           // Disable DEPTH_TEST for 2D
}