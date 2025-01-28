#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <semaphore.h>
#include <unistd.h>

#define THREADS_NUM 3

// Node structure for the list
typedef struct List {
    int value;
    struct List *next;
} List;

struct List *lista = NULL; // Initialize list as NULL

pthread_mutex_t mutex;
sem_t sem;

// Function to display the list
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

// Function to insert an element at the end of the list
void push_f(int value) {
    struct List *newElement;
    struct List *cursor = lista;
    newElement = (struct List *)malloc(sizeof(struct List));
    newElement->value = value;
    newElement->next = NULL;

    if (lista == NULL) {
        lista = newElement;  // If list is empty, new element becomes the head
    } else {
        // Traverse to the end of the list
        while (cursor->next) {
            cursor = cursor->next;
        }
        cursor->next = newElement; // Insert at the end
    }
}

// Function to remove the last element from the list
int pop_f() {
    if (lista == NULL) {
        return -1; // List is empty
    }

    struct List *cursor = lista;
    struct List *prev = NULL;

    // Traverse to the last element
    while (cursor->next) {
        prev = cursor;
        cursor = cursor->next;
    }

    int return_value = cursor->value;

    // Remove the last element
    if (prev == NULL) { // List had only one element
        free(cursor);
        lista = NULL;
    } else {
        free(cursor);
        prev->next = NULL;
    }

    return return_value;
}

// Wrapper function to remove elements from the list for a thread
void *popThread(void *arg) {
    while (1) {
        sem_wait(&sem);
        pthread_mutex_lock(&mutex);

        int val = pop_f();
        if (val != -1) {
            printf("Thread removed: %d\n", val);
        } else {
            printf("Thread tried to remove from an empty list.\n");
        }
        display(lista);

        pthread_mutex_unlock(&mutex);
        sleep(3); // Simulate some work
    }
    return NULL;
}

// Wrapper function to add elements to the list for a thread
void *pushThread(void *arg) {
    while (1) {
        int val = rand() % 100;

        pthread_mutex_lock(&mutex);

        push_f(val);
        printf("Thread inserted: %d\n", val);
        display(lista);

        pthread_mutex_unlock(&mutex);

        sem_post(&sem); // Signal that an element was added

        sleep(1); // Simulate some work
    }
    return NULL;
}

int main() {
    pthread_t tid[THREADS_NUM];
    pthread_mutex_init(&mutex, NULL);
    sem_init(&sem, 0, 0); // Initialize semaphore with 0

    srand(time(NULL));

    // Create threads: 1 push thread and 2 pop threads
    pthread_create(&tid[0], NULL, pushThread, NULL);
    pthread_create(&tid[1], NULL, popThread, NULL);
    pthread_create(&tid[2], NULL, popThread, NULL);

    // Wait for threads to finish (they run indefinitely in this case)
    for (int i = 0; i < THREADS_NUM; i++) {
        pthread_join(tid[i], NULL);
    }

    // Clean up
    sem_destroy(&sem);
    pthread_mutex_destroy(&mutex);

    // Free the list
    while (lista) {
        pop_f();
    }

    return 0;
}

