#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>

#define NUM_THREADS  4
#define TOTAL_COUNT 10
#define COUNT_LIMIT 12

int count = 0;
pthread_mutex_t count_mutex;
pthread_cond_t count_threshold_cv;

void *inc_count(void *t)
{
    int i;
    long my_id = (long)t;

    for (i = 0; i < TOTAL_COUNT; i++) {

        count++;

        if (count == COUNT_LIMIT) {
            printf("inc_count(): thread %ld, count = %d  Threshold reached. ",
                my_id, count);
            pthread_cond_signal(&count_threshold_cv);
            printf("Just sent signal.\n");
        }
        printf("inc_count(): thread %ld, count = %d, unlocking mutex\n",
            my_id, count);

        /* Do some work so threads can alternate on mutex lock */
        sleep(1);
    }
    pthread_exit(NULL);
}

void *watch_count(void *t)
{
    long my_id = (long)t;

    printf("watch_count(): thread %ld\n", my_id);


    while (count < COUNT_LIMIT) {
        printf("watch_count(): thread %ld Count= %d. Going into wait...\n", my_id, count);
        pthread_cond_wait(&count_threshold_cv, &count_mutex);
        printf("watch_count(): thread %ld Condition signal received. Count= %d\n", my_id, count);
    }
    printf("watch_count(): thread %ld Updating the value of count...\n", my_id);
    count += 125;
    printf("watch_count(): thread %ld count now = %d.\n", my_id, count);
    printf("watch_count(): thread %ld Unlocking mutex.\n", my_id);
    pthread_exit(NULL);
}

int main(int argc, char *argv[])
{
    int i;
    long t1=1, t2=2, t3=3, t4=4;
    pthread_t threads[NUM_THREADS];
    pthread_attr_t attr;

    /* Initialize mutex and condition variable objects */
    pthread_mutex_init(&count_mutex, NULL);
    pthread_cond_init(&count_threshold_cv, NULL);

    /* For portability, explicitly create threads in a joinable state */
    pthread_attr_init(&attr);
    pthread_attr_setdetachstate(&attr, PTHREAD_CREATE_JOINABLE);
    pthread_create(&threads[0], &attr, watch_count, (void *)t1);
    pthread_create(&threads[1], &attr, inc_count, (void *)t2);
    pthread_create(&threads[2], &attr, inc_count, (void *)t3);
    pthread_create(&threads[3], &attr, inc_count, (void *)t4);

    /* Wait for all threads to complete */
    for (i = 0; i < NUM_THREADS; i++) {
        pthread_join(threads[i], NULL);
    }
    printf ("main(): waited and joined with %d threads. Final value of count = %d. Done.\n",
        NUM_THREADS, count);

    /* Clean up and exit */
    pthread_attr_destroy(&attr);
    pthread_mutex_destroy(&count_mutex);
    pthread_cond_destroy(&count_threshold_cv);
    pthread_exit(NULL);
}

