#ifndef MULT_THREAD
#define MULT_THREAD

#include <stdbool.h>

#define _GNU_SOURCE
#include <pthread.h>

#define THREAD_POOL_SIZE 10

bool thread_is_available();
void join_done_threads();
int  request_thread(pthread_t** assigned_thread);
int num_available_threads();

#endif