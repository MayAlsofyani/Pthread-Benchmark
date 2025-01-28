#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <pthread.h>
#include <string.h>

#define CONSUMER_COUNT 2
#define PRODUCER_COUNT 2

typedef struct node_ {
    struct node_ *next;
    int num;
} node_t;

node_t *head = NULL;
pthread_t thread[CONSUMER_COUNT + PRODUCER_COUNT];
pthread_cond_t cond;
pthread_mutex_t mutex;

void* consume(void* arg) {
    int id = *(int*)arg;
    free(arg);  // Free the dynamically allocated id after use
    while (1) {
        pthread_mutex_lock(&mutex);

        while (head == NULL) {
            printf("%d consume wait\n", id);
            pthread_cond_wait(&cond, &mutex);
        }

        // Consume the node
        node_t *p = head;
        head = head->next;
        printf("Consumer %d consumed %d\n", id, p->num);

        pthread_mutex_unlock(&mutex);

        // Free the consumed node
        free(p);
    }
    return NULL;
}

void* produce(void* arg) {
    int id = *(int*)arg;
    free(arg);  // Free the dynamically allocated id after use
    int i = 0;

    while (1) {
        pthread_mutex_lock(&mutex);

        node_t *p = (node_t*)malloc(sizeof(node_t));
        memset(p, 0x00, sizeof(node_t));
        p->num = i++;
        p->next = head;
        head = p;

        printf("Producer %d produced %d\n", id, p->num);

        // Signal one waiting consumer
        pthread_cond_signal(&cond);
        pthread_mutex_unlock(&mutex);

        sleep(1);  // Simulate work
    }
    return NULL;
}

int main() {
    int i = 0;
    pthread_cond_init(&cond, NULL);
    pthread_mutex_init(&mutex, NULL);

    // Create consumer threads
    for (; i < CONSUMER_COUNT; i++) {
        int *p = (int*)malloc(sizeof(int));  // Dynamically allocate id
        *p = i;
        pthread_create(&thread[i], NULL, consume, (void*)p);
    }

    // Create producer threads
    for (i = 0; i < PRODUCER_COUNT; i++) {
        int *p = (int*)malloc(sizeof(int));  // Dynamically allocate id
        *p = i;
        pthread_create(&thread[i + CONSUMER_COUNT], NULL, produce, (void*)p);
    }

    // Wait for all threads to complete (which never happens in this case)
    for (i = 0; i < CONSUMER_COUNT + PRODUCER_COUNT; i++) {
        pthread_join(thread[i], NULL);
    }

    pthread_mutex_destroy(&mutex);
    pthread_cond_destroy(&cond);

    return 0;
}

