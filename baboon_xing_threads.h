// Michael Girbino -- mjg159
// EECS 338 HW7: Baboon Crossing - POSIX threads implementation

#include <pthread.h>
#include <semaphore.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

#ifndef NUM_THREADS
#define NUM_THREADS 200
#endif

extern const unsigned int RAND_RANGE;

// functions:
void baboon_fork(int atob_or_btoa);
void ToB();
void ToA();

typedef struct _thread_data_t {
	int tid;//thread id
	int amount;//amount to deposit or withdraw
} thread_data_t;
