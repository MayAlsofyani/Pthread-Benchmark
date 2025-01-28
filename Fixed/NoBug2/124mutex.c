#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <pthread.h>

void *thread_function(void *arg);
pthread_mutex_t work_mutex;
pthread_cond_t work_cond;

#define WORK_SIZE 1024
char work_area[WORK_SIZE];
int time_to_exit = 0;

int main()
{
    int res;
    pthread_t a_thread;
    void *thread_result;

    res = pthread_mutex_init(&work_mutex, NULL);
    if (res != 0) {
        perror("Mutex initialization failed");
        exit(EXIT_FAILURE);
    }

    res = pthread_cond_init(&work_cond, NULL);
    if (res != 0) {
        perror("Condition variable initialization failed");
        exit(EXIT_FAILURE);
    }

    res = pthread_create(&a_thread, NULL, thread_function, NULL);
    if (res != 0) {
        perror("Thread creation failed");
        exit(EXIT_FAILURE);
    }

    pthread_mutex_lock(&work_mutex);
    printf("Input some text, enter 'end' to finish:\n");
    while (!time_to_exit) {
        fgets(work_area, WORK_SIZE, stdin);

     
        pthread_cond_signal(&work_cond);
        pthread_mutex_unlock(&work_mutex);

        pthread_mutex_lock(&work_mutex);
        while (work_area[0] != '\0') {
            pthread_cond_wait(&work_cond, &work_mutex);
        }
    }

    pthread_mutex_unlock(&work_mutex);
    printf("Waiting for thread to finish...\n");

    res = pthread_join(a_thread, &thread_result);
    if (res != 0) {
        perror("pthread_join failed");
        exit(EXIT_FAILURE);
    }

    printf("Thread joined\n");

    pthread_mutex_destroy(&work_mutex);
    pthread_cond_destroy(&work_cond);

    exit(EXIT_SUCCESS);
}

void *thread_function(void *arg)
{
    sleep(1);
    pthread_mutex_lock(&work_mutex);
    while (strncmp("end", work_area, 3) != 0) {
        printf("You input %d characters\n", (int)(strlen(work_area) - 1));
        work_area[0] = '\0';

     
        pthread_cond_signal(&work_cond);
        pthread_cond_wait(&work_cond, &work_mutex);
    }

    time_to_exit = 1;
    pthread_cond_signal(&work_cond);
    pthread_mutex_unlock(&work_mutex);
    pthread_exit(0);
}

