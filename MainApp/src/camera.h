#ifndef CAMERA_H
#define CAMERA_H

#include "coal_miner.h"

typedef enum CameraControllerType
{
	CC_FREE,
	CC_ThirdPerson
}CameraControllerType;

void load_camera();
void update_camera();
void dispose_camera();
Camera3D get_camera();
void set_camera_target(Transform* target);
void set_camera_type(CameraControllerType type);

#endif //CAMERA_H
