#include <stdio.h>
#include <pthread.h>
#include <stdlib.h>

int matrice[10][4] = { {8, 25, 3, 41},
                       {11, 18, 3, 4},
                       {4, 15, 78, 12},
                       {1, 12, 0, 12},
                       {7, 9, 13, 15},
                       {4, 21, 37, 89},
                       {1, 54, 7, 3},
                       {15, 78, 7, 1},
                       {12, 15, 13, 47},
                       {91, 13, 72, 90} };

int vecteur[4] = {1, 2, 3, 4};

int resultat[10] = {0, 0, 0, 0, 0, 0, 0, 0, 0, 0}; // col resultat
pthread_mutex_t mutex;

void* computeVector(void* arg) {
    int column = *((int*)arg);  // Correctly pass the argument by dereferencing it
    free(arg);  // Free dynamically allocated memory for the thread argument
    
    for (int i = 0; i < 10; i++) {
        
        resultat[i] = resultat[i] + matrice[i][column] * vecteur[column];
        
    }
    
    return NULL;
}

int main() {
    if (pthread_mutex_init(&mutex, NULL) != 0) {
        perror("ERROR INITIALIZING MUTEX");
        return 1;
    }

    pthread_t thread_ids[4];
    
    for (int i = 0; i < 4; i++) {
        int* arg = malloc(sizeof(*arg));  // Allocate memory for each thread argument
        if (arg == NULL) {
            perror("ERROR ALLOCATING MEMORY");
            return 1;
        }
        *arg = i;
        
        if (pthread_create(&thread_ids[i], NULL, computeVector, arg) != 0) {
            perror("ERROR CREATING THREAD");
            return 1;
        }
    }

    for (int i = 0; i < 4; i++) {
        if (pthread_join(thread_ids[i], NULL) != 0) {
            perror("ERROR JOINING THREAD");
            return 1;
        }
    }

    // Print the result
    for (int i = 0; i < 10; i++) {
        printf("\n%d", resultat[i]);
    }

    pthread_mutex_destroy(&mutex);
    return 0;
}

