#include "camera.h"
#include "config.h"

typedef struct Camera3RDPerson
{
	float yaw;
	float pitch;
	float distance;
	vec2 offset;
}Camera3RDPerson;

typedef struct CameraOffsetData
{
	vec3 offset;
	vec3 direction;
}CameraOffsetData;

Camera3D camera = CAMERA_INIT;
Camera3RDPerson cameraController = { 0, 50, 14, 0, 0, };
Transform* cameraTarget;
CameraControllerType cameraType = CC_FREE;
vec2 freeCameraPair = { 0, 90 };

static CameraOffsetData CalculateOffset();
static void FreeCamera();
static void ThirdPersonCamera();

void load_camera()
{
	camera.position[1] = 250;
	camera.farPlane = 1000;
}

void update_camera()
{
	switch (cameraType)
	{
		case CC_FREE: FreeCamera(); break;
		case CC_ThirdPerson: ThirdPersonCamera(); break;
	}
}

void dispose_camera()
{

}

Camera3D get_camera() { return camera; }
void set_camera_target(Transform* target) { cameraTarget = target; }
void remove_camera_target() { cameraTarget = NULL; }
void set_camera_type(CameraControllerType type) { cameraType = type; }

//region Local Functions

static CameraOffsetData CalculateOffset()
{
	CameraOffsetData data = {};
	cm_yp_to_direction(cameraController.yaw, cameraController.pitch, data.direction);
	
	vec3 right;
	glm_cross(data.direction, camera.up, right);
	vec3 off = { 0, cameraController.offset[1], 0 };
	glm_vec3_scale(right, cameraController.offset[0], data.offset);
	vec3 distOff;
	glm_vec3_scale(data.direction, cameraController.distance, distOff);
	glm_vec3_sub(off, distOff, off);
	glm_vec3_add(data.offset, off, data.offset);
	
	return data;
}

static void FreeCamera()
{
	vec2 delta;
	cm_get_mouse_delta(delta);
	glm_vec2_negate(delta);
	glm_vec2_scale(delta, CAM_FREE_ROTATE_SPEED * cm_delta_time_f(), delta);
//	printf("x: %f, y: %f\n", delta[0], delta[1]);
	glm_vec2_add(freeCameraPair, delta, freeCameraPair);
	freeCameraPair[0] = fmodf(freeCameraPair[0], 360);
	freeCameraPair[1] = glm_clamp(freeCameraPair[1], CAM_FREE_ROTATE_MIN_LIMIT, CAM_FREE_ROTATE_MAX_LIMIT);

	cm_yp_to_direction(freeCameraPair[0], freeCameraPair[1], camera.direction);
//	printf("x: %f, y: %f, z: %f", camera.direction[0], camera.direction[1], camera.direction[2]);
	
	vec3 right;
	glm_cross(camera.direction, camera.up, right);
	
	vec3 moveDir = { 0, 0 };
	
	if(cm_is_key_down(KEY_A)) moveDir[0]--;
	if(cm_is_key_down(KEY_D)) moveDir[0]++;
	if(cm_is_key_down(KEY_SPACE)) moveDir[1]++;
	if(cm_is_key_down(KEY_LEFT_SHIFT)) moveDir[1]--;
	if(cm_is_key_down(KEY_W)) moveDir[2]++;
	if(cm_is_key_down(KEY_S)) moveDir[2]--;
	
	glm_normalize(moveDir);
	
	if(!glm_vec3_eqv_eps(moveDir, GLM_VEC2_ZERO))
	{
		vec3 rDir;
		glm_vec3_scale(right, moveDir[0] * CAM_FREE_MOVE_SPEED * cm_delta_time_f(), rDir);
		vec3 fDir;
		glm_vec3_scale(camera.direction, moveDir[2] * CAM_FREE_MOVE_SPEED * cm_delta_time_f(), fDir);
		vec3 upDir;
		glm_vec3_scale(camera.up, moveDir[1] * CAM_FREE_MOVE_SPEED * cm_delta_time_f(), upDir);

		glm_vec3_add(camera.position, fDir, camera.position);
		glm_vec3_add(camera.position, rDir, camera.position);
		glm_vec3_add(camera.position, upDir, camera.position);

	}
}

static void ThirdPersonCamera()
{
}

//endregion