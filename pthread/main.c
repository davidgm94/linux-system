#include "common.h"
#include <assert.h>
#include <stdio.h>
#include <stdbool.h>
#include <string.h>
#include <pthread.h>
#include <unistd.h>
#include <sys/time.h>
#include <emmintrin.h>
#include <semaphore.h>
typedef struct timespec timespec;

#define AVOID_COMPILER_MEMORY_REORDERING() asm volatile ("" ::: "memory");

double get_time(void)
{
	timespec ts;
	clock_gettime(CLOCK_MONOTONIC_RAW, &ts);
    return (double)ts.tv_sec + (((double)ts.tv_nsec) / 1000000000);
}
struct work_queue;
typedef struct work_queue work_queue;
typedef void worker(work_queue*, void*);
typedef struct
{
	worker* callback;
	void* data;
} work;

struct work_queue
{
	u32 volatile completed_task_count;
	u32 volatile next_task;
	u32 volatile task_count;
	sem_t semaphore;
	
	work tasks[256];
};
static void test_worker(work_queue* queue, void* data)
{
	printf("Thread %lu: %s\n", pthread_self(), (char*)data);
}

typedef struct
{
	work_queue* queue;
	u32 index;
} thread_data;

void add_entry(work_queue* queue, worker* worker_callback, void* data)
{
	// TODO: switch to atomic_compare_exchange so any thread can add? WARN: potential overhead
	assert(queue->task_count < COUNT_OF(queue->tasks));
	work* task = &queue->tasks[queue->task_count];
	task->callback = worker_callback;
	task->data = data;
	AVOID_COMPILER_MEMORY_REORDERING();
	(queue->task_count)++;
	sem_post(&queue->semaphore);
}

static bool work_on_next_task(work_queue* queue)
{
	u32 original_next_task = queue->next_task;
	bool sleep_after = original_next_task < queue->task_count;

	if (sleep_after)
	{
		bool incremented = __atomic_compare_exchange_n(&queue->next_task, &original_next_task, original_next_task + 1, false, __ATOMIC_SEQ_CST, __ATOMIC_SEQ_CST);
		if (incremented)
		{
			work task = queue->tasks[original_next_task];
			if (task.callback != NULL)
			{
				task.callback(queue, task.data);
			}
			// AVOID_COMPILER_MEMORY_REORDERING();
			__atomic_fetch_add(&queue->completed_task_count, 1, __ATOMIC_SEQ_CST);
		}
	}

	return sleep_after;
}

static void complete_all_work(work_queue* queue)
{
	while (queue->task_count != queue->completed_task_count)
	{
		work_on_next_task(queue);
	}

	queue->completed_task_count = 0;
	queue->next_task = 0;
	queue->task_count = 0;
}

void* task(void* args)
{
	thread_data* thread = (thread_data*)args;
	work_queue* queue = thread->queue;

	for(;;)
	{
		if (work_on_next_task(queue))
		{
			sem_wait(&queue->semaphore);
		}
	}

	return NULL;
}

#define STD_LIB 1
#if STD_LIB
s32 main()
#else
void _start()
#endif
{
	work_queue queue;
	queue.completed_task_count = 0;
	queue.task_count = 0;
	queue.next_task = 0;
	u32 initial_count = 0;
	sem_init(&queue.semaphore, 0, initial_count);

	pthread_t thread_pool[7];
	const u32 thread_count = COUNT_OF(thread_pool);
	thread_data thread_pool_data[thread_count];

	for (u32 i = 0; i < thread_count; i++)
	{
		pthread_t* thread = &thread_pool[i];
		thread_data* pthread_data = &thread_pool_data[i];
		pthread_data->index = i;
		pthread_data->queue = &queue;
		pthread_create(thread, NULL, task, pthread_data);
	}

	for (u64 i = 0; i < 256; i++)
	{
		add_entry(&queue, test_worker, "String 1");
	}

	complete_all_work(&queue);
	printf("Releasing threads...\n");

	_exit(0);
	for (u32 i = 0; i < thread_count; i++)
	{
		pthread_t thread = thread_pool[i];
		pthread_join(thread, NULL);
	}
}
