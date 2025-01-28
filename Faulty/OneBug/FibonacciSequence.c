#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>

pthread_mutex_t mutex;
int fib_cache[1000]; // Shared cache for memoization

void* fibonacci(void* arg) {
    int n = *(int*) arg;
    if (n <= 1) return (void*) (size_t) n;

    pthread_t thread1, thread2;
    int n1 = n - 1, n2 = n - 2;
    void* res1;
    void* res2;

    pthread_create(&thread1, NULL, fibonacci, &n1);
    pthread_create(&thread2, NULL, fibonacci, &n2);

    pthread_join(thread1, &res1);
    pthread_join(thread2, &res2);

    int result = (size_t) res1 + (size_t) res2;

    
    fib_cache[n] = result;
    

    return (void*) (size_t) result;
}

int main() {
    int n = 10; // Example Fibonacci number
    pthread_mutex_init(&mutex, NULL);

    pthread_t thread;
    pthread_create(&thread, NULL, fibonacci, &n);

    void* result;
    pthread_join(thread, &result);

    printf("Fibonacci of %d is %zu\n", n, (size_t) result);
    pthread_mutex_destroy(&mutex);

    return 0;
}
