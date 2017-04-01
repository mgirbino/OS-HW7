// Michael Girbino -- mjg159
// EECS 338 HW7: Baboon Crossing - POSIX threads implementation

#include "baboon_crossing.h"

int main(int argc, char *argv[])
{
  union semun semaphore_values;
  unsigned short semaphore_init_values[NUMBER_OF_SEMAPHORES];
  semaphore_init_values[SEMAPHORE_MUTEX] = 1;
  semaphore_init_values[SEMAPHORE_AtoB] = 0;
  semaphore_init_values[SEMAPHORE_BtoA] = 0;
  semaphore_values.array = semaphore_init_values;

  // creating semaphores:
  int semid = get_semid((key_t)SEMAPHORE_KEY);
  if (semctl(semid, SEMAPHORE_MUTEX, SETALL, semaphore_values) == -1)
  {
    perror("semctl failed");
    exit(EXIT_FAILURE);
  }

  // creating shared memory:
  int shmid = get_shmid((key_t)SEMAPHORE_KEY);
  struct shared_variable_struct * shared_variables = shmat(shmid, 0, 0);

  shared_variables->CrossingCount     = 0;
  shared_variables->CrossedCount      = 0;
  shared_variables->AtoBWaitCount     = 0;
  shared_variables->BtoAWaitCount     = 0;
  shared_variables->CrossingDirection = None;

  // random number of baboons (bounded):
  const int number_of_baboons =
    (rand() % (MAX_NUMBER_OF_BABOONS + 1 - MIN_NUMBER_OF_BABOONS)) +
    MIN_NUMBER_OF_BABOONS;

  printf("%d Baboons created\n", number_of_baboons);

  // Random distribution of baboons:
  for (int i = 0; i < number_of_baboons; i++)
  {
    if (rand() % 2)
    {
      printf("Baboon A to B\n");
      baboon_fork(BABOONAtoB);
    }
    else
    {
      printf("Baboon B to A\n");
      baboon_fork(BABOONBtoA);
    }
    stall(BABOON_CREATE_STALL_TIME);
  }

  // waiting for processes to exit:
  for (int i = 0; i < number_of_baboons; i++)
  {
    wait(NULL);
  }
  printf("Crossing Finished\n");

  // detach shared memory:
  if (shmdt(shared_variables) == -1)
  {
    perror("shmdt failed");
    exit(EXIT_FAILURE);
  }

  // these are also necessary:
  if (shmctl(shmid, IPC_RMID, NULL) < 0)
  {
    perror("shmctrl failed");
    exit(EXIT_FAILURE);
  }
}

void baboon_fork(int atob_or_btoa)
{
  pid_t child_pid;
  child_pid = fork();
  if (child_pid == -1)
  {
    perror("Fork Failed");
    exit(EXIT_FAILURE);
  }
  // Child:
  else if (!child_pid)
  {
    if (atob_or_btoa == BABOONAtoB)
    {
      ToB();
      exit(EXIT_SUCCESS);
    }
    else if (atob_or_btoa == BABOONBtoA)
    {
      ToA();
      exit(EXIT_SUCCESS);
    }
    else
    {
      perror("Invalid Baboon");
      exit(EXIT_FAILURE);
    }
  }
  // Parent:
  else
  {
    return;
  }
}

void ToB(void)
{
  int semid = get_semid((key_t)SEMAPHORE_KEY);
  int shmid = get_shmid((key_t)SEMAPHORE_KEY);
  struct shared_variable_struct * shared_variables = shmat(shmid, 0, 0);
  semaphore_wait(semid, SEMAPHORE_MUTEX);

  // Continue in same direction, or start going in this direction
  // Crossing direction = (AtoB or none) and Crossing count < 5, and total < 10:
  if ((shared_variables->CrossingDirection == AtoB || shared_variables->CrossingDirection == None) &&
    shared_variables->CrossingCount < 5 && (shared_variables->CrossedCount + shared_variables->CrossingCount) < 10)
  {
    printf("A to B about to cross %d\n", getpid());
    shared_variables->CrossingDirection = AtoB;
    shared_variables->CrossingCount++;
    semaphore_signal(semid, SEMAPHORE_MUTEX);
  }
  // wait to cross:
  else
  {
    printf("A to B waiting to cross %d\n", getpid());
    shared_variables->AtoBWaitCount++;
    semaphore_signal(semid, SEMAPHORE_MUTEX);
    semaphore_wait(semid, SEMAPHORE_AtoB);

    printf("A to B about to cross %d\n", getpid());
    shared_variables->AtoBWaitCount--;
    shared_variables->CrossingCount++;
    shared_variables->CrossingDirection = AtoB;
    semaphore_signal(semid, SEMAPHORE_MUTEX);
  }

  printf("A to B crossing %d\n", getpid());
  stall(CROSS_ROPE_STALL_TIME);
  printf("A to B successfully crossed %d\n", getpid());
  semaphore_wait(semid, SEMAPHORE_MUTEX);
  shared_variables->CrossedCount++;
  shared_variables->CrossingCount--;

  // keep going in this direction
  // nonzero AtoBWaitCount and (total <10 or (total >= 10 and BtoAWaitCount = 0):
  if (shared_variables->AtoBWaitCount != 0 &&
    (((shared_variables->CrossedCount + shared_variables->CrossingCount) < 10) ||
    ((shared_variables->CrossedCount + shared_variables->CrossingCount) >= 10 &&
    shared_variables->BtoAWaitCount == 0)))
  {
    semaphore_signal(semid,SEMAPHORE_AtoB);
  }
  // switch directions
  // none crossing and nonzero BtoAWaitCount and (AtoBWaitCount = 0 or total >= 10):
  else if (shared_variables->CrossingCount == 0 && shared_variables->BtoAWaitCount !=0 &&
    (shared_variables->AtoBWaitCount == 0 ||
      (shared_variables->CrossedCount + shared_variables->CrossingCount) >= 10))
  {
    shared_variables->CrossingDirection = BtoA;
    shared_variables->CrossedCount = 0;
    semaphore_signal(semid, SEMAPHORE_BtoA);
  }
  // change direction to none
  // none waiting on either side and none crossing:
  else if (shared_variables->CrossingCount == 0 &&
    shared_variables->AtoBWaitCount == 0 && shared_variables->BtoAWaitCount == 0)
  {
    shared_variables->CrossingDirection = None;
    shared_variables->CrossedCount == 0;
    semaphore_signal(semid, SEMAPHORE_MUTEX);
  }
  // continue as none:
  else
  {
    semaphore_signal(semid, SEMAPHORE_MUTEX);
  }

  if (shmdt(shared_variables) == -1)
  {
		perror("shmdt failed");
		exit(EXIT_FAILURE);
	}

	exit(EXIT_SUCCESS);
}

void ToA(void)
{
  int semid = get_semid((key_t)SEMAPHORE_KEY);
  int shmid = get_shmid((key_t)SEMAPHORE_KEY);
  struct shared_variable_struct * shared_variables = shmat(shmid, 0, 0);
  semaphore_wait(semid, SEMAPHORE_MUTEX);

  // Continue in same direction, or start going in this direction
  // Crossing direction = (BtoA or none) and Crossing count < 5, and total < 10:
  if ((shared_variables->CrossingDirection == BtoA || shared_variables->CrossingDirection == None) &&
    shared_variables->CrossingCount < 5 && (shared_variables->CrossedCount + shared_variables->CrossingCount) < 10)
  {
    printf("B to A about to cross %d\n", getpid());
    shared_variables->CrossingDirection = BtoA;
    shared_variables->CrossingCount++;
    semaphore_signal(semid, SEMAPHORE_MUTEX);
  }
  // wait to cross:
  else
  {
    printf("B to A waiting to cross %d\n", getpid());
    shared_variables->BtoAWaitCount++;
    semaphore_signal(semid, SEMAPHORE_MUTEX);
    semaphore_wait(semid, SEMAPHORE_BtoA);

    printf("B to A about to cross %d\n", getpid());
    shared_variables->BtoAWaitCount--;
    shared_variables->CrossingCount++;
    shared_variables->CrossingDirection = BtoA;
    semaphore_signal(semid, SEMAPHORE_MUTEX);
  }

  stall(CROSS_ROPE_STALL_TIME);
  printf("B to A successfully crossed %d\n", getpid());
  semaphore_wait(semid, SEMAPHORE_MUTEX);
  shared_variables->CrossedCount++;
  shared_variables->CrossingCount--;

  // keep going in this direction
  // nonzero BtoAWaitCount and (total <10 or (total >= 10 and BtoAWaitCount = 0):
  if (shared_variables->BtoAWaitCount != 0 &&
    (((shared_variables->CrossedCount + shared_variables->CrossingCount) < 10) ||
    ((shared_variables->CrossedCount + shared_variables->CrossingCount) >= 10 &&
    shared_variables->BtoAWaitCount == 0)))
  {
    semaphore_signal(semid,SEMAPHORE_BtoA);
  }
  // switch directions
  // none crossing and nonzero BtoAWaitCount and (BtoAWaitCount = 0 or total >= 10):
  else if (shared_variables->CrossingCount == 0 && shared_variables->BtoAWaitCount !=0 &&
    (shared_variables->BtoAWaitCount == 0 ||
      (shared_variables->CrossedCount + shared_variables->CrossingCount) >= 10))
  {
    shared_variables->CrossingDirection = BtoA;
    shared_variables->CrossedCount = 0;
    semaphore_signal(semid, SEMAPHORE_BtoA);
  }
  // change direction to none
  // none waiting on either side and none crossing:
  else if (shared_variables->CrossingCount == 0 &&
    shared_variables->BtoAWaitCount == 0 && shared_variables->BtoAWaitCount == 0)
  {
    shared_variables->CrossingDirection = None;
    shared_variables->CrossedCount == 0;
    semaphore_signal(semid, SEMAPHORE_MUTEX);
  }
  // continue as none:
  else
  {
    semaphore_signal(semid, SEMAPHORE_MUTEX);
  }

  if (shmdt(shared_variables) == -1)
  {
		perror("shmdt failed");
		exit(EXIT_FAILURE);
	}

	exit(EXIT_SUCCESS);
}

void semaphore_wait(int semid, int semnumber)
{
  struct sembuf wait_buffer;
  wait_buffer.sem_num = semnumber;
  wait_buffer.sem_op = -1;
  wait_buffer.sem_flg = 0;
  if (semop(semid, &wait_buffer, 1) == -1)
  {
    perror("semaphore_wait failed");
    exit(EXIT_FAILURE);
  }
}

void semaphore_signal(int semid, int semnumber)
{
  struct sembuf signal_buffer;
  signal_buffer.sem_num = semnumber;
  signal_buffer.sem_op = 1;
  signal_buffer.sem_flg = 0;

  if (semop(semid, &signal_buffer, 1) == -1)
  {
    perror("semaphore_signal failed");
    exit(EXIT_FAILURE);
  }
}

void stall(int iterations)
{
  int i;
  while(i < iterations)
    i++;
}

int get_semid(key_t semkey)
{
  int value = semget(semkey, NUMBER_OF_SEMAPHORES, 0777 | IPC_CREAT);
  if (value == -1)
  {
    perror("semget failed");
    exit(EXIT_FAILURE);
  }
  return value;
}

int get_shmid(key_t shmkey)
{
  int value = shmget(shmkey, sizeof(struct shared_variable_struct), 0777 | IPC_CREAT);
  if (value == -1)
  {
    perror("shmkey failed");
    exit(EXIT_FAILURE);
  }
  return value;
}
