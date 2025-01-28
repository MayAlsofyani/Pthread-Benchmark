#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <pthread.h>
#include <time.h>
#include <stdbool.h>

static pthread_mutex_t lock = PTHREAD_MUTEX_INITIALIZER;
static unsigned int seed0;
static unsigned int seed1;
static int have_init = 0;

static int read_dev_random (void *buffer, size_t buffer_size)
{
  int fd;
  ssize_t status = 0;

  char *buffer_position;
  size_t yet_to_read;

  fd = open ("/dev/urandom", O_RDONLY);
  if (fd < 0)
  {
    perror ("open");
    return (-1);
  }

  buffer_position = (char *) buffer;
  yet_to_read = buffer_size;

  while (yet_to_read > 0)
  {
    status = read (fd, (void *) buffer_position, yet_to_read);
    if (status < 0)
    {
      if (errno == EINTR)
 continue;

      fprintf (stderr, "read_dev_random: read failed.\n");
      break;
    }

    buffer_position += status;
    yet_to_read -= (size_t) status;
  }

  close (fd);

  if (status < 0)
    return (-1);
  return (0);
}

static void do_init (void)
{
  if (have_init)
    return;

  read_dev_random (&seed0, sizeof (seed0));
  read_dev_random (&seed1, sizeof (seed1));
  have_init = 1;
}

int sn_random_init (void)
{
  have_init = 0;
  do_init ();

  return (0);
}

int sn_random (void)
{
  int r0;
  int r1;



  do_init ();

  r0 = rand_r (&seed0);
  r1 = rand_r (&seed1);



  return (r0 ^ r1);
}

int sn_true_random (void)
{
  int ret = 0;
  int status;

  status = read_dev_random (&ret, sizeof (ret));
  if (status != 0)
    return (sn_random ());

  return (ret);
}

int sn_bounded_random (int min, int max)
{
  int range;
  int rand;

  if (min == max)
    return (min);
  else if (min > max)
  {
    range = min;
    min = max;
    max = range;
  }

  range = 1 + max - min;
  rand = min + (int) (((double) range)
      * (((double) sn_random ()) / (((double) 32767) + 1.0)));

  assert (rand >= min);
  assert (rand <= max);

  return (rand);
}

double sn_double_random (void)
{
  return (((double) sn_random ()) / (((double) 32767) + 1.0));
}
# 7 "os21.c" 2
# 1 "pycparser/utils/fake_libc_include/sys/types.h" 1
# 8 "os21.c" 2
# 1 "pycparser/utils/fake_libc_include/sys/syscall.h" 1
# 9 "os21.c" 2


pthread_mutex_t lock;
pthread_mutex_t lock2;
pthread_cond_t empty;
pthread_cond_t full;


struct item{
        int val;
        int sleeptime;
};

struct item * buffer;

void * consumer(void *dummy)
{

        int * curitem = (int *)(dummy);
        pid_t x = syscall(__NR_gettid);

        while(1)
        {
  
                fflush(stdout);
                while(*curitem < 0){

                        pthread_cond_wait(&empty, &lock);
                }
                sleep(buffer[*curitem].sleeptime);
                printf("(CONSUMER) My ID is %d and I ate the number %d in %d seconds\n\n",x,buffer[*curitem].val, buffer[*curitem].sleeptime);
                buffer[*curitem].val = 0;
                buffer[*curitem].sleeptime = 0;
                *curitem = *curitem - 1;


                if(*curitem == 30){
                        pthread_cond_signal(&full);
                }

        }
        return 0;
}
void * producer(void *dummy)
{
        pthread_mutex_lock(&lock2);
        int * curitem = (int *)(dummy);
        pid_t x = syscall(__NR_gettid);
        pthread_mutex_unlock(&lock2);
        int i = 0;
        while(1)
        {
                sleep(generate_rand(3,7));
                fflush(stdout);
                pthread_mutex_lock(&lock);

                while(*curitem > 30){
                        pthread_cond_wait(&full, &lock);
                }

                *curitem = *curitem + 1;
                i = (int)generate_rand(0,10);
                printf("(PRODUCER) My ID is %d and I created the value %d\n\n",x,i);
                buffer[*curitem].val = i;
                i = (int)generate_rand(2,9);
                buffer[*curitem].sleeptime = i;
                if(*curitem == 0){
                    pthread_cond_signal(&empty);
                }
                pthread_mutex_unlock(&lock);
        }

        return 0;
}

int main(int argc, char **argv)
{
        int * curitem = malloc(sizeof(int));
        int curthread = 0;
        buffer = malloc(sizeof(struct item)*32);
        *curitem = -1;
        pthread_mutex_init(&lock, 0);
        pthread_mutex_init(&lock2, 0);
        pthread_cond_init(&full, 0);
        pthread_cond_init(&empty, 0);
        int threadcap = atoi(argv[1]);
        pthread_t conthreads[threadcap];
        pthread_t prod;
        pthread_create(&prod, 0, producer, (void*)curitem);

        for(; curthread < threadcap; curthread++)
                pthread_create(&conthreads[curthread], 0, consumer, (void*)curitem);

        pthread_join(prod, 0);

        for(curthread = 0; curthread < threadcap; curthread++)
                pthread_join(conthreads[curthread], 0);

        return 0;
}
