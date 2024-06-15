#ifndef CMTIME_H
#define CMTIME_H

#include "cmplatform.h"

void load_time(Window* ptr);
void update_time();
void coal_sleep(double sleepTime);

#ifdef _WIN32
void __stdcall Sleep(unsigned long msTimeout);
void __stdcall timeBeginPeriod(unsigned long precision);
void __stdcall timeEndPeriod(unsigned long precision);
#endif

#endif //CMTIME_H
