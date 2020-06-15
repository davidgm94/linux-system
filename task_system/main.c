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

#define AVOID_COMPILER_MEMORY_REORDERING() asm volatile ("" ::: "memory")

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

#define TASK_COUNT 2
#define HW_THREADS 12
#define THREADS_TO_CREATE 11
struct work_queue
{
	u32 volatile completed_task_goal;
	u32 volatile completed_task_count;
	u32 volatile next_task_to_write;
	u32 volatile next_task_to_read;
	sem_t semaphore;
	work tasks[TASK_COUNT];
};

static void test_worker(work_queue* queue, void* data)
{
	printf("Thread %lu: %s\n", pthread_self(), (char*)data);
}
static void test2(work_queue* queue, void* data)
{
	u64 sum = 0;
	u64* data_ptr = (u64*)data;
	u64 n = *data_ptr;
	for (u64 i = 0; i < 100000000; i++)
	{
		sum += n * i;
	}
	*data_ptr = sum;
}

typedef struct
{
	work_queue* queue;
	u32 index;
} thread_data;

static void add_entry(work_queue* queue, worker* worker_callback, void* data)
{
	// TODO: switch to atomic_com.are_exchange so any thread can add? WARN: potential overhead
	u32 new_next_task_to_write = (queue->next_task_to_write + 1) % COUNT_OF(queue->tasks);
	while (new_next_task_to_write == queue->next_task_to_read);
	work* task = &queue->tasks[queue->next_task_to_write];
	task->callback = worker_callback;
	task->data = data;
	++queue->completed_task_goal;

	AVOID_COMPILER_MEMORY_REORDERING();

	queue->next_task_to_write = new_next_task_to_write;
	sem_post(&queue->semaphore);
}

static bool work_on_next_task(work_queue* queue)
{
	bool sleep_after = false;
	u32 original_next_task_to_read = queue->next_task_to_read;
	u32 new_next_task_to_read = (original_next_task_to_read + 1) % COUNT_OF(queue->tasks);

	if (original_next_task_to_read != queue->next_task_to_write)
	{
		u32 index = __sync_val_compare_and_swap(&queue->next_task_to_read, original_next_task_to_read, new_next_task_to_read);
		if (index == original_next_task_to_read)
		{
			work task = queue->tasks[original_next_task_to_read];
			task.callback(queue, task.data);
			__sync_fetch_and_add(&queue->completed_task_count, 1);
		}
	}
	else
	{
		sleep_after = true;
	}

	return sleep_after;
}

static void complete_all_work(work_queue* queue)
{
	while (queue->completed_task_goal != queue->completed_task_count)
	{
		work_on_next_task(queue);
	}

	queue->completed_task_count = 0;
	queue->completed_task_goal = 0;
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
	queue.completed_task_goal = 0;
	queue.next_task_to_read = 0;
	queue.next_task_to_write = 0;

	u32 initial_count = 0;
	sem_init(&queue.semaphore, 0, initial_count);

	pthread_t thread_pool[THREADS_TO_CREATE];
	const u32 thread_count = COUNT_OF(thread_pool);
	thread_data thread_pool_data[thread_count];

	for (u32 i = 0; i < thread_count; i++)
	{
		pthread_attr_t attr;
		pthread_attr_init(&attr);
		pthread_attr_setdetachstate(&attr, PTHREAD_CREATE_DETACHED);
		pthread_t* thread = &thread_pool[i];
		thread_data* pthread_data = &thread_pool_data[i];
		pthread_data->index = i;
		pthread_data->queue = &queue;
		int result = pthread_create(thread, NULL, task, pthread_data);
	}
	u64 arr[1000* 1000];
	for (u64 i = 0; i < 1000 * 1000; i++)
	{
		arr[i] = i;
		add_entry(&queue, test2, &arr[i]);
	}

	// for (u64 i = 0; i < 1000*1000; i++)
	// {
	// 	printf("Sum: %lu\n", arr[i]);
	// }

	complete_all_work(&queue);
	printf("Releasing threads...\n");

	_exit(0);
	for (u32 i = 0; i < thread_count; i++)
	{
		pthread_t thread = thread_pool[i];
		pthread_join(thread, NULL);
	}
}
