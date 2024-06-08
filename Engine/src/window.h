#ifndef WINDOW_H
#define WINDOW_H

#include "platform.h"
#include "coal_miner_internal.h"

void create_window(unsigned int width, unsigned int height,
				   const char* title, const char* iconLocation);
void set_window_flags(ConfigFlags hint);
bool window_should_close();
void begin_draw();
void end_draw();

void close_window();

#endif //WINDOW_H
