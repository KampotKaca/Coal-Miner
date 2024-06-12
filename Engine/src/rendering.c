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
	disable_backface_culling();
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

//	printf("x: %f, y: %f, z: %f", camera.up.x, camera.up.y, camera.up.z);
	cameraUbo.view = m4x4_look_at(camera.position, camera.target, camera.up);
	cameraUbo.view = (M4x4){
		cameraUbo.view.m0, cameraUbo.view.m4, cameraUbo.view.m8, cameraUbo.view.m12,
		cameraUbo.view.m1, cameraUbo.view.m5, cameraUbo.view.m9, cameraUbo.view.m13,
		cameraUbo.view.m2, cameraUbo.view.m6, cameraUbo.view.m10, cameraUbo.view.m14,
		cameraUbo.view.m3, cameraUbo.view.m7, cameraUbo.view.m11, cameraUbo.view.m15,
	};

	cameraUbo.viewProjection = m4x4_mult(cameraUbo.projection, cameraUbo.view);
	cameraUbo.position = camera.position;
	cameraUbo.direction = v3_norm(v3_sub(camera.target, camera.position));
//	cm_set_shader_uniform_m4x4(shader, "cameraViewProjection", cameraUbo.viewProjection);

//	enable_depth_test();

//	upload_ubos();
}

void cm_end_mode_3d()
{
	disable_depth_test();           // Disable DEPTH_TEST for 2D
}