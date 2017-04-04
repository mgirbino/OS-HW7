// Michael Girbino -- mjg159
// EECS 338 HW7: Baboon Crossing - POSIX threads implementation

#define _GNU_SOURCE

#include <pthread.h>
#include <semaphore.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/errno.h>
#include <sys/time.h>
#include <time.h>
#include <sys/wait.h>

#ifndef NUM_THREADS
#define NUM_THREADS 20
#endif

// amount of iterations to do nothing:
#define BABOON_CREATE_STALL_TIME  10000
#define CROSS_ROPE_STALL_TIME  100000

typedef enum {None, Xing_AtoB, Xing_BtoA} direction;

// functions:
void *ToB(void *arg);
void *ToA(void *arg);

void stall(int iterations);

void semwait(sem_t *sem);//error-checked semaphore wait
void semsignal(sem_t *sem);//error-checked semaphore signal
