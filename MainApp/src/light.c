#include "light.h"

GlobalLight MAIN_GLOBAL_LIGHT = GLOBAL_LIGHT_INIT;

void load_light()
{
}

void update_light()
{
	if(cm_is_key_down(KEY_LEFT_CONTROL))
	{
		vec3 axis;
		glm_vec3_zero(axis);
		
		if(cm_is_key_down(KEY_UP))    axis[2] = 1;
		if(cm_is_key_down(KEY_DOWN))  axis[2] = -1;
		if(cm_is_key_down(KEY_RIGHT)) axis[0] = 1;
		if(cm_is_key_down(KEY_LEFT))  axis[0] = -1;
		
		if(glm_vec3_norm(axis) > 0)
		{
			glm_normalize(axis);
			mat4 rotation;
			
			glm_rotate_make(rotation, glm_rad(20.0f * cm_delta_time_f()), axis);
			glm_mat4_mulv3(rotation, MAIN_GLOBAL_LIGHT.direction, 1.0f, MAIN_GLOBAL_LIGHT.direction);
		}
	}
}

void dispose_light()
{

}

GlobalLight get_global_light() { return MAIN_GLOBAL_LIGHT; }
