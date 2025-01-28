#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>

int arr[100];
int target;
pthread_mutex_t mutex;
int found = 0; // Shared variable to indicate if target is found

void* binary_search(void* arg) {
    int* range = (int*) arg;
    int low = range[0], high = range[1];

    if (low > high || found) return NULL;

    int mid = (low + high) / 2;

    
    if (arr[mid] == target) {
        found = 1;
        printf("Element found at index %d\n", mid);
    }
    

    if (!found) {
        pthread_t left_thread, right_thread;
        int left_range[] = {low, mid - 1};
        int right_range[] = {mid + 1, high};

        pthread_create(&left_thread, NULL, binary_search, left_range);
        pthread_create(&right_thread, NULL, binary_search, right_range);

        pthread_join(left_thread, NULL);
        pthread_join(right_thread, NULL);
    }
    return NULL;
}

int main() {
    int n = 100; // Size of array
    target = 50; // Example target to search for
    for (int i = 0; i < n; i++) arr[i] = i; // Fill array with sorted values

    pthread_mutex_init(&mutex, NULL);

    pthread_t thread;
    int range[] = {0, n - 1};
    pthread_create(&thread, NULL, binary_search, range);
    pthread_join(thread, NULL);

    pthread_mutex_destroy(&mutex);

    if (!found) printf("Element not found\n");
    return 0;
}
