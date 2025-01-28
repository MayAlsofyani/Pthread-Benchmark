#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <pthread.h>

#define NUM_THREAD 3  
#define TCOUNT 5
#define COUNT_LIMIT 7
int count = 0;

pthread_mutex_t count_mutex;
pthread_cond_t count_threshold_cv;

// Increment the count
void *inc_count(void *t)
{
    int i;
    long my_id = (long)t;

    for (i = 0; i < TCOUNT; ++i)
    {
        pthread_mutex_lock(&count_mutex);
        count++;
        // Check if the count reaches the threshold and signal
        if (count >= COUNT_LIMIT)
        {
            printf("inc_count(): thread %ld, count = %d Threshold reached. ",
                   my_id, count);
            // Send the signal
            pthread_cond_signal(&count_threshold_cv);
            printf("Just sent signal.\n");
        }

        printf("inc_count(): thread %ld, count = %d, unlocking mutex.\n",
               my_id, count);
        pthread_mutex_unlock(&count_mutex);

        sleep(1); // Simulate some work
    }

    pthread_exit(NULL);
}

// Watcher thread waiting for the count to reach the threshold
void *watch_count(void *t)
{
    long my_id = (long)t;
    printf("Starting watch_count(): thread %ld.\n", my_id);

    // Lock the mutex and wait for the signal
    pthread_mutex_lock(&count_mutex);
    while (count < COUNT_LIMIT)
    {
        printf("watch_count(): thread %ld going into wait...\n", my_id);
        pthread_cond_wait(&count_threshold_cv, &count_mutex);
        printf("watch_count(): thread %ld Condition signal received.\n", my_id);

        printf("watch_count(): thread %ld count now = %d.\n", my_id, count);
    }
    pthread_mutex_unlock(&count_mutex);
    
    pthread_exit(NULL);
}

int main()
{
    long t1 = 1, t2 = 2, t3 = 3;
    pthread_t threads[NUM_THREAD];
    pthread_attr_t attr;

    // Initialize mutex and condition variable
    pthread_mutex_init(&count_mutex, NULL);
    pthread_cond_init(&count_threshold_cv, NULL);

    // Initialize thread attributes
    pthread_attr_init(&attr);
    pthread_attr_setdetachstate(&attr, PTHREAD_CREATE_JOINABLE);

    // Create the watcher thread
    pthread_create(&threads[0], &attr, watch_count, (void *)t1);
    // Create two incrementer threads
    pthread_create(&threads[1], &attr, inc_count, (void *)t2);
    pthread_create(&threads[2], &attr, inc_count, (void *)t3);

    // Join threads
    for (int i = 0; i < NUM_THREAD; ++i)
    {
        pthread_join(threads[i], NULL);
    }

    printf("Main(): Waited on %d threads, final value of count = %d. Done\n",
           NUM_THREAD, count);

    // Clean up
    pthread_attr_destroy(&attr);
    pthread_mutex_destroy(&count_mutex);
    pthread_cond_destroy(&count_threshold_cv);
    pthread_exit(NULL);
}

