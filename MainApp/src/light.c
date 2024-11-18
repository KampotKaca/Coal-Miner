#include "light.h"

#define DAY_NIGHT_DURATION 20.0f

GlobalLight MAIN_GLOBAL_LIGHT = GLOBAL_LIGHT_INIT;

static void RotateLight(vec3 axis, float angle);

void load_light()
{
	RotateLight((vec3){ 0, 0, 1 }, -60.0f);
}

void update_light()
{
#if DEBUG
	if(cm_is_key_down(KEY_LEFT_CONTROL))
	{
		vec3 axis;
		glm_vec3_zero(axis);
		
		if(cm_is_key_down(KEY_UP))    axis[2] = 1;
		if(cm_is_key_down(KEY_DOWN))  axis[2] = -1;
		if(cm_is_key_down(KEY_RIGHT)) axis[0] = 1;
		if(cm_is_key_down(KEY_LEFT))  axis[0] = -1;
		
		if(glm_vec3_norm(axis) > 0) RotateLight(axis, 20.0f * cm_delta_time_f());
	}
#else
	RotateLight((vec3){ 0, 0, 1 }, (360.0f / DAY_NIGHT_DURATION) * cm_delta_time_f());
#endif
}

void dispose_light()
{

}

GlobalLight get_global_light() { return MAIN_GLOBAL_LIGHT; }

static void RotateLight(vec3 axis, float angle)
{
	glm_normalize(axis);
	mat4 rotation;
	
	glm_rotate_make(rotation, glm_rad(angle), axis);
	glm_mat4_mulv3(rotation, MAIN_GLOBAL_LIGHT.direction, 1.0f, MAIN_GLOBAL_LIGHT.direction);
}
