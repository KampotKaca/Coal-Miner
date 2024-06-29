#ifndef CMTIME_H
#define CMTIME_H

#include "cmplatform.h"

void load_time(Window* ptr);
void update_time();

#ifdef _WIN32
void __stdcall timeBeginPeriod(unsigned long precision);
void __stdcall timeEndPeriod(unsigned long precision);
#endif

#endif //CMTIME_H
