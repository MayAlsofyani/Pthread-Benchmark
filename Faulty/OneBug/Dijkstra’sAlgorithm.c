#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>

#define MAX_NODES 10

pthread_mutex_t dist_mutex;
int dist[MAX_NODES];

void* dijkstra(void* arg) {
    int u = *(int*)arg;
    
    // Update distances and relax edges for node u
    printf("Processing node %d\n", u); // Debug output for testing
    
    free(arg); // Free the allocated memory
    return NULL;
}

int main() {
    pthread_mutex_init(&dist_mutex, NULL);

    // Graph and initial distance setup here

    pthread_t threads[MAX_NODES];
    for (int i = 0; i < MAX_NODES; i++) {
        int* node = malloc(sizeof(int));
        if (node == NULL) {
            perror("Failed to allocate memory");
            exit(1);
        }
        *node = i; // Assign the value of i to the dynamically allocated memory
        if (pthread_create(&threads[i], NULL, dijkstra, node) != 0) {
            perror("Failed to create thread");
            exit(1);
        }
    }

    for (int i = 0; i < MAX_NODES; i++) {
        pthread_join(threads[i], NULL);
    }

    pthread_mutex_destroy(&dist_mutex);
    return 0;
}

