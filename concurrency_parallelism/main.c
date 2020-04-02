#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/mman.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <unistd.h>
#include <pthread.h>

void* foo(void* number) {
	printf("%d\n", rand());
	return NULL;
}

#define THREAD_COUNT 8
int main(int argc, char* argv[])
{
	pthread_t threadPool[THREAD_COUNT];
	for (int i = 0; i < THREAD_COUNT; i++)
		pthread_create(&threadPool[i], 0, foo, 0);
}
