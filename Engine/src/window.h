#ifndef WINDOW_H
#define WINDOW_H

#include "coal_miner.h"

void create_window(unsigned int width, unsigned int height, const char* title);
void set_window_flags(ConfigFlags hint);
bool window_should_close();
void begin_draw();
void end_draw();

void terminate_window();

#endif //WINDOW_H
