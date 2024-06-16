#include "cmtime.h"
#ifdef _WIN32

#else
#include <unistd.h>
#include <time.h>
#endif
#include <glfw/glfw3.h>

typedef struct Time
{
	double deltaTime;
	double timeSinceStart;
	double timeScale;
	unsigned int targetFrameRate;
	unsigned long frameCount;
	double lastFrameTime;
	double frameRecordTime;
	unsigned int frameRate;

	void* platformPtr;
}Time;

Time TIME = { 0, 0, 1, 60, 0, 0, 0, 0 };

void load_time(Window* ptr)
{
	TIME.platformPtr = ptr;
	TIME.timeSinceStart = glfwGetTime();
	TIME.frameRecordTime = TIME.timeSinceStart;
}

void update_time()
{
	double newTime = glfwGetTime();
	TIME.lastFrameTime = newTime - TIME.timeSinceStart;

	double sleepTime = (1.0 / (double)TIME.targetFrameRate) - TIME.lastFrameTime;
	if(sleepTime >= 0.001) coal_sleep(sleepTime);

	newTime = glfwGetTime();

	TIME.deltaTime = newTime - TIME.timeSinceStart;
	TIME.timeSinceStart = newTime;
	TIME.frameCount++;

	if(TIME.frameCount % FRAME_RATE_RECORD_RATE == 0)
	{
		TIME.frameRate = (unsigned int)(1.0 / ((TIME.timeSinceStart - TIME.frameRecordTime) / (double)FRAME_RATE_RECORD_RATE));
		TIME.frameRecordTime = TIME.timeSinceStart;
	}
}

void coal_sleep(double sleepTime)
{
#ifdef _WIN32

	timeBeginPeriod(1);
	Sleep((unsigned long)(sleepTime * 1000));
	timeEndPeriod(1);

#else
	struct timespec req, rem;
    req.tv_sec = (time_t)seconds;
    req.tv_nsec = (long)((seconds - req.tv_sec) * 1e9);
    nanosleep(&req, &rem);
#endif
}

float cm_delta_time_f() { return (float)TIME.deltaTime; }
double cm_delta_time() { return TIME.deltaTime; }
double cm_time_since_start() { return TIME.timeSinceStart; }
unsigned int cm_get_target_frame_rate() { return TIME.targetFrameRate; }
void cm_set_target_frame_rate(unsigned int t) { TIME.targetFrameRate = t; }
unsigned int cm_frame_rate() { return TIME.frameRate; }
double cm_frame_time() { return TIME.lastFrameTime; }