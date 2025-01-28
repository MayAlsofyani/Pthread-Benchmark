#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <semaphore.h>
#include <unistd.h>

#define THREADS_NUM 3


typedef struct List {
    int value;
    struct List *next;
} List;

struct List *lista = NULL;

pthread_mutex_t mutex;
sem_t sem;


void display(struct List *element) {
    printf("[");
    while (element) {
        printf("%d", element->value);
        if (element->next) {
            printf(", ");
        }
        element = element->next;
    }
    printf("]\n");
}


void push_f(int value) {
    struct List *newElement;
    struct List *cursor = lista;
    newElement = (struct List *)malloc(sizeof(struct List));
    newElement->value = value;
    newElement->next = NULL;

    if (lista == NULL) {
        lista = newElement;
    } else {
     
        while (cursor->next) {
            cursor = cursor->next;
        }
        cursor->next = newElement;
    }
}


int pop_f() {
    if (lista == NULL) {
        return -1;
    }

    struct List *cursor = lista;
    struct List *prev = NULL;

    while (cursor->next) {
        prev = cursor;
        cursor = cursor->next;
    }

    int return_value = cursor->value;


    if (prev == NULL) {
        free(cursor);
        lista = NULL;
    } else {
        free(cursor);
        prev->next = NULL;
    }

    return return_value;
}


void *popThread(void *arg) {
    while (1) {
        sem_wait(&sem);
  

        int val = pop_f();
        if (val != -1) {
            printf("Thread removed: %d\n", val);
        } else {
            printf("Thread tried to remove from an empty list.\n");
        }
        display(lista);


        sleep(3);
    }
    return NULL;
}


void *pushThread(void *arg) {
    while (1) {
        int val = rand() % 100;


        push_f(val);
        printf("Thread inserted: %d\n", val);
        display(lista);



        sem_post(&sem);

        sleep(1);
    }
    return NULL;
}

int main() {
    pthread_t tid[THREADS_NUM];
    pthread_mutex_init(&mutex, NULL);
    sem_init(&sem, 0, 0); // Initialize semaphore with 0

    srand(time(NULL));


    pthread_create(&tid[0], NULL, pushThread, NULL);
    pthread_create(&tid[1], NULL, popThread, NULL);
    pthread_create(&tid[2], NULL, popThread, NULL);


    for (int i = 0; i < THREADS_NUM; i++) {
        pthread_join(tid[i], NULL);
    }


    sem_destroy(&sem);
    pthread_mutex_destroy(&mutex);


    while (lista) {
        pop_f();
    }

    return 0;
}

