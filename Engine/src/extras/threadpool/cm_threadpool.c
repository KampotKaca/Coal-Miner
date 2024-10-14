#include "cm_threadpool.h"
#include "coal_miner.h"

typedef struct
{
	ThreadPool* pool;
	uint32_t threadId;
}ThreadData;

static void* ExecuteJob(void* args);

ThreadPool* cm_create_thread_pool(uint32_t numThreads, uint32_t initialCapacity)
{
	ThreadPool* pool = CM_MALLOC(sizeof(ThreadPool));

	pthread_mutex_init(&pool->lock, NULL);
	pthread_cond_init(&pool->signal, NULL);
	
	pool->jobCount = 0;
	pool->aliveThreadCount = 0;
	pool->isAlive = true;
	pool->workingThreads = 0;
	pool->capacity = initialCapacity;

	if(initialCapacity > 0)
	{
		pool->jobs = CM_MALLOC(initialCapacity * sizeof(ThreadJob));
		memset(pool->jobs, 0, initialCapacity * sizeof(ThreadJob));
	}
	else pool->jobs = NULL;
	
	for (int i = 0; i < numThreads; ++i)
	{
		ThreadData threadData = { .pool = pool, .threadId = i };

		if(pthread_create(&pool->threads[pool->aliveThreadCount], NULL, &ExecuteJob, &threadData) != 0)
			perror("Failed to create the thread\n");
		pool->aliveThreadCount++;
	}
	
	return pool;
}

void cm_submit_job(ThreadPool* pool, ThreadJob job, bool asLast)
{
	pthread_mutex_lock(&pool->lock);

	if(pool->jobCount >= pool->capacity)
	{
		uint32_t oldCapacity = pool->capacity;
		pool->capacity *= 2;
		void* mem = CM_REALLOC(pool->jobs, pool->capacity * sizeof(ThreadJob));
		if(mem == NULL)
		{
			perror("Unable to allocate memory");
			exit(-1);
		}
		else
		{
			memset(&mem[oldCapacity * sizeof(ThreadJob)], 0, oldCapacity * sizeof(ThreadJob));
			pool->jobs = mem;
		}
	}

	if(asLast || pool->jobCount == 0) pool->jobs[pool->jobCount] = job;
	else
	{
		memcpy_s(&pool->jobs[1], sizeof(ThreadJob) * pool->jobCount, pool->jobs, sizeof(ThreadJob) * pool->jobCount);
		pool->jobs[0] = job;
	}
	pool->jobCount++;
	pthread_cond_signal(&pool->signal);
	
	pthread_mutex_unlock(&pool->lock);
}

void cm_destroy_thread_pool(ThreadPool* pool)
{
	pthread_mutex_lock(&pool->lock);
	
	pool->isAlive = false;
	pthread_cond_broadcast(&pool->signal);
	
	pthread_mutex_unlock(&pool->lock);
	
	for (int i = 0; i < pool->aliveThreadCount; ++i)
		pthread_join(pool->threads[i], NULL);
	
	for (int i = 0; i < pool->jobCount; ++i)
		CM_FREE(pool->jobs[i].args);
	
	pthread_mutex_destroy(&pool->lock);
	pthread_cond_destroy(&pool->signal);

	CM_FREE(pool->jobs);
	CM_FREE(pool);
}

static void* ExecuteJob(void* args)
{
	ThreadData* data = (ThreadData*)args;

	ThreadPool* pool = data->pool;
	uint32_t threadId = data->threadId;

	while (pool->isAlive)
	{
		pthread_mutex_lock(&pool->lock);
		while(pool->jobCount == 0)
		{
			pthread_cond_wait(&pool->signal, &pool->lock);
			if(!pool->isAlive)
			{
				pthread_mutex_unlock(&pool->lock);
				return NULL;
			}
		}
		
		pool->workingThreads++;
		
		ThreadJob job = pool->jobs[pool->jobCount - 1];
		pool->jobCount--;
		
		pthread_mutex_unlock(&pool->lock);
		
		if(job.job != NULL)
		{
			job.job(threadId, job.args);
			
			pthread_mutex_lock(&pool->lock);
			
			if(job.callbackJob != NULL) job.callbackJob(threadId, job.args);
			pool->workingThreads--;
			if(job.args != NULL)
			{
				CM_FREE(job.args);
				job.args = NULL;
			}
			
			pthread_mutex_unlock(&pool->lock);
		}
	}
	
	return NULL;
}