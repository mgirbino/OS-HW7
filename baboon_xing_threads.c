// Michael Girbino -- mjg159
// EECS 338 HW7: Baboon Crossing - POSIX threads implementation

#include "baboon_xing_threads.h"

sem_t AtoB, BtoA, mutex;

int CrossingCount;
int CrossedCount;
int AtoBWaitCount;
int BtoAWaitCount;
direction CrossingDirection;

int main(int argc, char *argv[]) {
  pthread_t threads[NUM_THREADS];
  thread_data_t thread_data[NUM_THREADS];
  int error_check;
  direction baboon_sequence[NUM_THREADS];

  //initialize semaphores
	if (sem_init(&AtoB, 0, (unsigned int)0) < 0
		|| sem_init(&BtoA, 0, (unsigned int)0) < 0
		|| sem_init(&mutex, 0, (unsigned int)1) < 0) {
		perror("sem_init");
		exit(EXIT_FAILURE);
	}

  // Random distribution of baboons:
  for (int i = 0; i < NUM_THREADS; i++) {
    void *thread_func;//the function to call

    baboon_sequence[i] = (rand()%2) ? Xing_AtoB : Xing_BtoA;
    thread_data[i].tid = i;

    if (baboon_sequence[i] == Xing_AtoB) {//if random amount < 0
      thread_func = ToB;
    }
    else {//else amount > 0
      thread_func = ToA;
    }
    if ((error_check = pthread_create(&threads[i], NULL, thread_func, &thread_data[i]))) {
      fprintf(stderr, "error: pthread_create, %d\n", error_check);
      return EXIT_FAILURE;
    }
  }

  printf("About to join threads\n");
  fflush(stdout);

  for (int i = 0; i < NUM_THREADS; i++) {
    if ((error_check = pthread_join(threads[i], NULL))) {
      fprintf(stderr, "error: pthread_join, %d\n", error_check);
    }
  }

  printf("Everybody was able to cross\n");
  printf("Baboon Creation Sequence:\n");

  for (int i = 0; i < NUM_THREADS; i++) {
    if (baboon_sequence[i] == Xing_AtoB) {
      printf("A to B");
    }
    else {
      printf("B to A");
    }
    printf(",");
  }
  printf("\n");

  return EXIT_SUCCESS;
}

void *ToB(void *arg) {
  thread_data_t *baboon_data = (thread_data_t *)arg;
  int tid = baboon_data->tid;
  printf("Baboon A to B %d Created\n", tid);
  fflush(stdout);

  semwait(&mutex);

  // Continue in same direction, or start going in this direction
  // Crossing direction = (AtoB or none) and Crossing count < 5, and total < 10:
  if ((CrossingDirection == Xing_AtoB || CrossingDirection == None) &&
    CrossingCount < 5 && (CrossedCount + CrossingCount) < 10) {
    printf("A to B about to cross %d\n", tid);
    fflush(stdout);
    CrossingDirection = Xing_AtoB;
    CrossingCount++;
    semsignal(&mutex);
  }
  // wait to cross:
  else {
    printf("A to B waiting to cross %d\n", tid);
    fflush(stdout);
    AtoBWaitCount++;
    semsignal(&mutex);
    semwait(&AtoB);

    printf("A to B about to cross %d\n", tid);
    fflush(stdout);
    AtoBWaitCount--;
    CrossingCount++;
    CrossingDirection = Xing_AtoB;
    semsignal(&mutex);
  }

  printf("A to B crossing %d\n", tid);
  fflush(stdout);
  semwait(&mutex);
  CrossedCount++;
  CrossingCount--;
  printf("A to B successfully crossed %d\n", tid);
  fflush(stdout);

  // keep going in this direction
  // nonzero AtoBWaitCount and (total <10 or (total >= 10 and BtoAWaitCount = 0):
  if (AtoBWaitCount != 0 &&
    ( ((CrossedCount + CrossingCount) < 10) ||
    ( (CrossedCount + CrossingCount) >= 10 &&
    BtoAWaitCount == 0) ) ) {
    semsignal(&AtoB);
  }
  // switch directions
  // none crossing and nonzero BtoAWaitCount and (AtoBWaitCount = 0 or total >= 10):
  else if (CrossingCount == 0 && BtoAWaitCount !=0 &&
    (AtoBWaitCount == 0 ||
      (CrossedCount + CrossingCount) >= 10)) {
    CrossingDirection = Xing_BtoA;
    CrossedCount = 0;
    semsignal(&BtoA);
  }
  // change direction to none
  // none waiting on either side and none crossing:
  else if (CrossingCount == 0 &&
    AtoBWaitCount == 0 && BtoAWaitCount == 0) {
    CrossingDirection = None;
    CrossedCount == 0;
    semsignal(&mutex);
  }
  // continue as none:
  else {
    semsignal(&mutex);
  }

  pthread_exit(NULL);
}

void *ToA(void *arg) {
  thread_data_t *baboon_data = (thread_data_t *)arg;
  int tid = baboon_data->tid;
  printf("Baboon B to A %d Created\n", tid);
  fflush(stdout);

	semwait(&mutex);

  // Continue in same direction, or start going in this direction
  // Crossing direction = (BtoA or none) and Crossing count < 5, and total < 10:
  if ((CrossingDirection == Xing_BtoA || CrossingDirection == None) &&
    CrossingCount < 5 && (CrossedCount + CrossingCount) < 10) {
    printf("B to A about to cross %d\n", tid);
    fflush(stdout);
    CrossingDirection = Xing_BtoA;
    CrossingCount++;
    semsignal(&mutex);
  }
  // wait to cross:
  else {
    printf("B to A waiting to cross %d\n", tid);
    fflush(stdout);
    BtoAWaitCount++;
    semsignal(&mutex);
    semwait(&BtoA);

    printf("B to A about to cross %d\n", tid);
    fflush(stdout);
    BtoAWaitCount--;
    CrossingCount++;
    CrossingDirection = Xing_BtoA;
    semsignal(&mutex);
  }

  printf("B to A crossing %d\n", tid);
  fflush(stdout);
  semwait(&mutex);
  CrossedCount++;
  CrossingCount--;
  printf("B to A successfully crossed %d\n", tid);
  fflush(stdout);

  // keep going in this direction
  // nonzero BtoAWaitCount and (total <10 or (total >= 10 and BtoAWaitCount = 0):
  if (BtoAWaitCount != 0 &&
    (((CrossedCount + CrossingCount) < 10) ||
    ((CrossedCount + CrossingCount) >= 10 &&
    BtoAWaitCount == 0))) {
    semsignal(&BtoA);
  }
  // switch directions
  // none crossing and nonzero BtoAWaitCount and (BtoAWaitCount = 0 or total >= 10):
  else if (CrossingCount == 0 && BtoAWaitCount !=0 &&
    (BtoAWaitCount == 0 ||
      (CrossedCount + CrossingCount) >= 10)) {
    CrossingDirection = Xing_BtoA;
    CrossedCount = 0;
    semsignal(&BtoA);
  }
  // change direction to none
  // none waiting on either side and none crossing:
  else if (CrossingCount == 0 &&
    BtoAWaitCount == 0 && BtoAWaitCount == 0) {
    CrossingDirection = None;
    CrossedCount == 0;
    semsignal(&mutex);
  }
  // continue as none:
  else {
    semsignal(&mutex);
  }

  pthread_exit(NULL);
}

void semwait(sem_t *sem) {
	if (sem_wait(sem) < 0) {
		perror("sem_wait");
		exit(EXIT_FAILURE);
	}
}

void semsignal(sem_t *sem) {
	if (sem_post(sem) < 0) {
		perror("sem_post");
		exit(EXIT_FAILURE);
	}
}
