#include "rendering.h"
#include "coal_miner.h"
#include "coal_gl.h"

Window* windowPtr;
M4x4 projection;
M4x4 view;
M4x4 view_projection_matrix;

void setup_renderer(Window* wPtr)
{
	windowPtr = wPtr;
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
		
		projection = m4x4_frustum(-right, right, -top, top, camera.nearPlane, camera.farPlane);
	}
	else if (camera.projection == CAMERA_ORTHOGRAPHIC)
	{
		// Setup orthographic projection
		double top = camera.fov / 2.0;
		double right = top*aspect;
		
		projection = m4x4_orthographic(-right, right, -top, top, camera.nearPlane, camera.farPlane);
	}
	
	view = m4x4_look_at(camera.position, camera.target, camera.up);
	view_projection_matrix = m4x4_mult(projection, view);
	enable_depth_test();
}

void cm_end_mode_3d()
{
	disable_depth_test();           // Disable DEPTH_TEST for 2D
}