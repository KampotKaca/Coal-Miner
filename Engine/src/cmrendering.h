#ifndef CMRENDERING_H
#define CMRENDERING_H

#include "cmwindow.h"

#define CAMERA_UBO_BINDING_ID 8
#define GLOBAL_LIGHT_UBO_BINDING_ID 9

void load_renderer(Window* wPtr);
void unload_renderer();

#endif //CMRENDERING_H
