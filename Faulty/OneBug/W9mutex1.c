#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>

// http://www.cs.cmu.edu/afs/cs/academic/class/15492-f07/www/pthreads.html


void *functionC();
pthread_mutex_t mutex1 = PTHREAD_MUTEX_INITIALIZER;
int counter = 0;

int main()
{
    int rc1, rc2;
    pthread_t thread1, thread2;

    /* Create independent threads each of which will execute functionC */

    if ((rc1 = pthread_create(&thread1, NULL, &functionC, NULL))) {
        printf("Thread creation failed: %d\n", rc1);
        return -1;
    }

    if ((rc2 = pthread_create(&thread2, NULL, &functionC, NULL))) {
        printf("Thread creation failed: %d\n", rc2);
        return -1;
    }

    /* Wait till threads are complete before main continues */
    pthread_join(thread1, NULL);
    pthread_join(thread2, NULL);

    return 0;  // Exit with 0 indicating success
}

void *functionC()
{
    
    counter++;
    printf("Counter value: %d\n", counter);
    
    return NULL;
}

