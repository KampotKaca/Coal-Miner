#ifndef CM_THREADPOOL_H
#define CM_THREADPOOL_H

#include <pthread.h>
#include <stdbool.h>
#include "coal_config.h"
#include <stdint.h>

typedef struct
{
	void* args;
	void (*job)(uint32_t threadId, void* args);
	void (*callbackJob)(uint32_t threadId, void* args);
}ThreadJob;

typedef struct
{
	ThreadJob* jobs;
	pthread_t threads[MAX_THREADS_IN_THREAD_POOL];
	volatile unsigned int capacity;
	volatile unsigned int jobCount;
	volatile unsigned int aliveThreadCount;
	volatile bool isAlive;
	
	pthread_mutex_t lock;
	pthread_cond_t signal;
	volatile unsigned int workingThreads;
}ThreadPool;

#endif //CM_THREADPOOL_H
