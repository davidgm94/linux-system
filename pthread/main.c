#include <stdio.h>
#include <pthread.h>
#include "common.h"
#define THREAD_COUNT 8

typedef struct
{
	unsigned index;
} thread_data;

void* fn(void* args)
{
	thread_data* data = (thread_data*) args;
	unsigned index = data->index;
	return (void*)printf("Hello, this is thread no %u\n", index);
}

int main(int argc, char* argv[])
{
	i32 s = 0;
	pthread_t thread_pool[THREAD_COUNT];
	thread_data thread_pool_data[THREAD_COUNT];
	for (int i = 0; i < THREAD_COUNT; i++)
	{
		pthread_t* thread = &thread_pool[i];
		thread_pool_data[i].index = i;
		void* args = &thread_pool_data[i];
		pthread_create(thread, NULL, fn, args);
	}

	for (int i = 0; i < THREAD_COUNT; i++)
	{
		pthread_t thread = thread_pool[i];
		pthread_join(thread, NULL);
	}

	return 0;
}
