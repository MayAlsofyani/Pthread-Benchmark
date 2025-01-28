#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>

#define BSIZE 4
#define NUMITEMS 30

typedef struct {
  char buf[BSIZE];
  int occupied;
  int nextin, nextout;
  pthread_mutex_t mutex;
  pthread_cond_t more;
  pthread_cond_t less;
} buffer_t;

buffer_t buffer;

void *producer(void *);
void *consumer(void *);

#define NUM_THREADS 2
pthread_t tid[NUM_THREADS]; /* array of thread IDs */

int main(int argc, char *argv[])
{
  int i;

  // Initialize buffer and its associated synchronization primitives
  buffer.occupied = 0;
  buffer.nextin = 0;
  buffer.nextout = 0;

  pthread_mutex_init(&(buffer.mutex), NULL);
  pthread_cond_init(&(buffer.more), NULL);
  pthread_cond_init(&(buffer.less), NULL);

  // Create threads: producer first, consumer second
  pthread_create(&tid[0], NULL, producer, NULL);
  pthread_create(&tid[1], NULL, consumer, NULL);

  // Wait for both threads to finish
  for (i = 0; i < NUM_THREADS; i++) {
    pthread_join(tid[i], NULL);
  }

  printf("\nmain() reporting that all %d threads have terminated\n", NUM_THREADS);

  // Cleanup resources
  pthread_mutex_destroy(&(buffer.mutex));
  pthread_cond_destroy(&(buffer.more));
  pthread_cond_destroy(&(buffer.less));

  return 0;
} /* main */

void *producer(void *parm)
{
  char item[NUMITEMS] = "IT'S A SMALL WORLD, AFTER ALL.";
  int i;

  printf("producer started.\n");
  fflush(stdout);

  for(i = 0; i < NUMITEMS; i++) {
    if (item[i] == '\0') break;  /* Quit if at end of string. */

    pthread_mutex_lock(&(buffer.mutex));

    // Wait if buffer is full
    while (buffer.occupied >= BSIZE) {
      printf("producer waiting.\n");
      fflush(stdout);
      pthread_cond_wait(&(buffer.less), &(buffer.mutex));
    }

    // Insert item into buffer
    buffer.buf[buffer.nextin++] = item[i];
    buffer.nextin %= BSIZE;
    buffer.occupied++;

    printf("producer produced: %c\n", item[i]);
    fflush(stdout);

    // Signal consumer that more items are available
    pthread_cond_signal(&(buffer.more));
    pthread_mutex_unlock(&(buffer.mutex));

    // Simulate work
    usleep(100000);  // Slow down the producer for demonstration
  }

  printf("producer exiting.\n");
  fflush(stdout);
  pthread_exit(0);
}

void *consumer(void *parm)
{
  char item;
  int i;

  printf("consumer started.\n");
  fflush(stdout);

  for(i = 0; i < NUMITEMS; i++) {
    pthread_mutex_lock(&(buffer.mutex));

    // Wait if buffer is empty
    while(buffer.occupied <= 0) {
      printf("consumer waiting.\n");
      fflush(stdout);
      pthread_cond_wait(&(buffer.more), &(buffer.mutex));
    }

    // Consume item from buffer
    item = buffer.buf[buffer.nextout++];
    buffer.nextout %= BSIZE;
    buffer.occupied--;

    printf("consumer consumed: %c\n", item);
    fflush(stdout);

    // Signal producer that there is space available
    pthread_cond_signal(&(buffer.less));
    pthread_mutex_unlock(&(buffer.mutex));

    // Simulate work
    usleep(150000);  // Slow down the consumer for demonstration
  }

  printf("consumer exiting.\n");
  fflush(stdout);
  pthread_exit(0);
}

