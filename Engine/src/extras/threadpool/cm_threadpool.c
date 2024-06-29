#include "cm_threadpool.h"
#include "coal_miner.h"

static void* ExecuteJob(void* args);

ThreadPool* cm_create_thread_pool(unsigned int numThreads)
{
	ThreadPool* pool = CM_MALLOC(sizeof(ThreadPool));
	
	pthread_mutex_init(&pool->lock, NULL);
	pthread_cond_init(&pool->signal, NULL);
	
	pool->jobCount = 0;
	pool->aliveThreadCount = 0;
	pool->isAlive = true;
	pool->workingThreads = 0;
	
	for (int i = 0; i < numThreads; ++i)
	{
		if(pthread_create(&pool->threads[pool->aliveThreadCount], NULL, &ExecuteJob, pool) != 0)
			perror("Failed to create the thread\n");
		pool->aliveThreadCount++;
	}
	
	return pool;
}

void cm_submit_job(ThreadPool* pool, ThreadJob job)
{
	pthread_mutex_lock(&pool->lock);
	
	pool->jobs[pool->jobCount] = job;
	pool->jobCount++;
	pthread_cond_signal(&pool->signal);
	
	pthread_mutex_unlock(&pool->lock);
}

void cm_destroy_thread_pool(ThreadPool* pool)
{
	pool->isAlive = false;
	pthread_cond_signal(&pool->signal);
}

static void* ExecuteJob(void* args)
{
	ThreadPool* pool = (ThreadPool*)args;
	while (pool->isAlive)
	{
		pthread_mutex_lock(&pool->lock);
		while(pool->jobCount == 0)
		{
			pthread_cond_wait(&pool->signal, &pool->lock);
			if(!pool->isAlive) break;
		}
		
		if(!pool->isAlive)
		{
			pthread_mutex_unlock(&pool->lock);
			break;
		}
		
		pool->workingThreads++;
		
		ThreadJob job = pool->jobs[pool->jobCount - 1];
		pool->jobCount--;
		
		pthread_mutex_unlock(&pool->lock);
		
		if(job.job != NULL)
		{
			job.job(job.args);
			
			if(job.callbackJob != NULL)
			{
				pthread_mutex_lock(&pool->lock);
				job.callbackJob(job.args);
				pthread_mutex_unlock(&pool->lock);
			}
			
			CM_FREE(job.args);
		}
		
		pthread_mutex_lock(&pool->lock);
		pool->workingThreads--;
		pthread_mutex_unlock(&pool->lock);
	}
	
	pthread_mutex_lock(&pool->lock);
	pool->aliveThreadCount--;
	bool destroyPool = false;
	if(pool->aliveThreadCount == 0)
	{
		destroyPool = true;
		pool->jobCount = 0;
	}
	
	pthread_mutex_unlock(&pool->lock);
	
	if(destroyPool)
	{
		pthread_mutex_destroy(&pool->lock);
		pthread_cond_destroy(&pool->signal);
		
		CM_FREE(pool);
		pool = NULL;
	}
	
	return NULL;
}