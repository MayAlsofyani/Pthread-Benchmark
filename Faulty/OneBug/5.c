#include <pthread.h>
#include <iostream>
#include <cstdlib>
using namespace std;

#define THREADNUM 4
#define VECLEN 1000000

struct DOT {
    int *a;
    int *b;
    long long int sum;
    long int veclen;
};

DOT data;
pthread_t callThd[THREADNUM];
pthread_mutex_t mutexsum;

void *dotprod(void *arg)
{
    long offset = reinterpret_cast<long>(arg);
    int start, end, len;
    long long int threadsum = 0;
    int *x, *y;
    
    len = data.veclen;
    start = offset * len;
    end = start + len;
    x = data.a;
    y = data.b;

    // Common code for each thread, operates based on the thread number (= offset)
    for (int i = start; i < end; i++) {
        threadsum += static_cast<long long int>(x[i]) * y[i];
    }

    // Occupy mutex lock because we are changing the value of shared sum
    
    data.sum += threadsum;
    cout << "Thread " << offset << " did " << start << " to " << end
         << ":  ThreadSum = " << threadsum << ", global sum = " << data.sum << "\n";
    

    pthread_exit(nullptr);
}

int main(int argc, char *argv[])
{
    int i;
    int *a, *b;
    void *status;
    pthread_attr_t attr;

    // Intilizing all the values
    a = (int*) malloc(THREADNUM * VECLEN * sizeof(int));
    b = (int*) malloc(THREADNUM * VECLEN * sizeof(int));

    if (a == nullptr || b == nullptr) {
        cerr << "Error allocating memory for vectors.\n";
        exit(1);
    }

    for (i = 0; i < VECLEN * THREADNUM; i++) {
        a[i] = rand() % 100;  // Change this to generate values in an appropriate range
        b[i] = rand() % 100;  // Change this to generate values in an appropriate range
    }

    data.veclen = VECLEN;
    data.a = a;
    data.b = b;
    data.sum = 0;

    pthread_mutex_init(&mutexsum, nullptr);
    pthread_attr_init(&attr);
    pthread_attr_setdetachstate(&attr, PTHREAD_CREATE_JOINABLE);

    for (i = 0; i < THREADNUM; i++) {
        pthread_create(&callThd[i], &attr, dotprod, reinterpret_cast<void*>(i));
    }

    pthread_attr_destroy(&attr);

    // Wait for all threads to complete
    for (i = 0; i < THREADNUM; i++) {
        pthread_join(callThd[i], &status);
    }

    cout << "Sum = " << data.sum << " \n";

    free(a);
    free(b);
    pthread_mutex_destroy(&mutexsum);
    pthread_exit(nullptr);
}

