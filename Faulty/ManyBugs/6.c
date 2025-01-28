#include <pthread.h>
#include <iostream>
#include <unistd.h>
using namespace std;

#define NUM_THREADS  3
#define TCOUNT 10
#define COUNT_LIMIT 12

int count1 = 0;
pthread_mutex_t count_mutex;
pthread_cond_t count_threshold_cv;

void *inc_count(void *t) {
    long my_id = (long)t;

    for (int i = 0; i < TCOUNT; i++) {

        count1++;

        if (count1 >= COUNT_LIMIT) {
            cout << "inc_count(): thread " << my_id << ", count = " << count1 << "  Threshold reached. ";
            pthread_cond_broadcast(&count_threshold_cv);
            cout << "Just sent signal.\n";
        }
        cout << "inc_count(): thread " << my_id << ", count = " << count1 << ", unlocking mutex\n";

        usleep(100);
    }
    pthread_exit(NULL);
}

void *watch_count(void *t) {
    long my_id = (long)t;

    cout << "Starting watch_count(): thread " << my_id << "\n";
    while (count1 < COUNT_LIMIT) {
        cout << "watch_count(): thread " << my_id << " Count= " << count1 << ". Going into wait...\n";
        pthread_cond_wait(&count_threshold_cv, &count_mutex);
        cout << "watch_count(): thread " << my_id << " Condition signal received. Count= " << count1 << "\n";
    }


    cout << "watch_count(): thread " << my_id << " Updating the value of count...\n";
    count1 += 125;
    cout << "watch_count(): thread " << my_id << " count now = " << count1 << ".\n";
    
    cout << "watch_count(): thread " << my_id << " Unlocking mutex.\n";

    pthread_exit(NULL);
}

int main(int argc, char *argv[]) {
    long t1 = 1, t2 = 2, t3 = 3;
    pthread_t threads[3];
    pthread_attr_t attr;

    pthread_mutex_init(&count_mutex, NULL);
    pthread_cond_init(&count_threshold_cv, NULL);

    pthread_attr_init(&attr);
    pthread_attr_setdetachstate(&attr, PTHREAD_CREATE_JOINABLE);
    pthread_create(&threads[0], &attr, watch_count, (void *)t1);
    pthread_create(&threads[1], &attr, inc_count, (void *)t2);
    pthread_create(&threads[2], &attr, inc_count, (void *)t3);

    for (int i = 0; i < NUM_THREADS; i++) {
        pthread_join(threads[i], NULL);
    }
    cout << "Main(): Waited and joined with " << NUM_THREADS << " threads. Final value of count = " << count1 << ". Done.\n";

    pthread_attr_destroy(&attr);
    pthread_mutex_destroy(&count_mutex);
    pthread_cond_destroy(&count_threshold_cv);
    pthread_exit(NULL);
}

