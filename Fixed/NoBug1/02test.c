#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <unistd.h>

#define errExit(str) \
    do { perror(str); exit(-1); } while(0)

pthread_mutex_t mutex = PTHREAD_MUTEX_INITIALIZER;

static size_t i_gloabl_n = 0;

void *thr_demo(void *args)
{
    printf("this is thr_demo thread, pid: %d, tid: %lu\n", getpid(), pthread_self());
    pthread_mutex_lock(&mutex);
    
    int i;
    int max = *(int *)args;
    size_t tid = pthread_self();
    setbuf(stdout, NULL);
    for (i = 0; i < max; ++i) {
        printf("(tid: %lu):%lu\n", tid, ++i_gloabl_n);
    }

    pthread_mutex_unlock(&mutex);  // Unlock the mutex after critical section
    return (void *)10;
}

void *thr_demo2(void *args)
{
    printf("this is thr_demo2 thread, pid: %d, tid: %lu\n", getpid(), pthread_self());
    pthread_mutex_lock(&mutex);

    int i;
    int max = *(int *)args;
    size_t tid = pthread_self();
    setbuf(stdout, NULL);
    for (i = 0; i < max; ++i) {
        printf("(tid: %lu):%lu\n", tid, ++i_gloabl_n);
    }

    pthread_mutex_unlock(&mutex);  // Unlock the mutex after critical section
    return (void *)10;
}

int main()
{
    pthread_t pt1, pt2;

    int arg = 20;
    int ret;

    ret = pthread_create(&pt1, NULL, thr_demo, &arg);
    if (ret != 0) {
        errExit("pthread_create");
    }

    ret = pthread_create(&pt2, NULL, thr_demo2, &arg);
    if (ret != 0) {
        errExit("pthread_create");
    }

    printf("this is main thread, pid = %d, tid = %lu\n", getpid(), pthread_self());

    // Join the first thread and capture return value
    void *ret_val;
    ret = pthread_join(pt1, &ret_val);
    if (ret != 0) {
        errExit("pthread_join");
    }
    printf("Thread 1 returned: %ld\n", (long)ret_val);

    // Join the second thread and capture return value
    ret = pthread_join(pt2, &ret_val);
    if (ret != 0) {
        errExit("pthread_join");
    }
    printf("Thread 2 returned: %ld\n", (long)ret_val);

    // Clean up the mutex
    pthread_mutex_destroy(&mutex);

    return 0;
}

