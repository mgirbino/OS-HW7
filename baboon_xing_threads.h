// Michael Girbino -- mjg159
// EECS 338 HW7: Baboon Crossing - POSIX threads implementation

#define _GNU_SOURCE

#include <pthread.h>
#include <semaphore.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/types.h>
#include <inttypes.h>
#include <sys/errno.h>
#include <sys/time.h>
#include <time.h>
#include <sys/wait.h>

#ifndef NUM_THREADS
#define NUM_THREADS 20
#endif

typedef enum {None, Xing_AtoB, Xing_BtoA} direction;

typedef struct _thread_data_t {
	int tid;//thread id
} thread_data_t;

// functions:
void *ToB(void *arg);
void *ToA(void *arg);

void semwait(sem_t *sem);//error-checked semaphore wait
void semsignal(sem_t *sem);//error-checked semaphore signal
