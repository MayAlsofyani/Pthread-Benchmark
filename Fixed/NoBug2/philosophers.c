#include <stdlib.h>
#include <stdio.h>
#include <time.h>
#include <pthread.h>
#include <unistd.h>

pthread_mutex_t* f;
int* hungerOfPhylosophs;
int N;

void takeLeftFork(int);
void takeRightFork(int);
void putLeftFork(int);
void putRightFork(int);
void* meal(void*);

int main()
{
    int Hunger, *Nums;
    pthread_t* P;

    while (1) {
        printf("Type number of philosophers and hunger of them:\n");
        scanf("%d%d", &N, &Hunger);
        if (N == 0) break;
    
    
        f = (pthread_mutex_t*)malloc(N * sizeof(pthread_mutex_t));
        hungerOfPhylosophs = (int*)malloc(N * sizeof(int));
        P = (pthread_t*)malloc(N * sizeof(pthread_t));
        Nums = (int*)malloc(N * sizeof(int));

 
        for (int i = 0; i < N; i++) {
            pthread_mutex_init(f + i, NULL);
            hungerOfPhylosophs[i] = Hunger;
            Nums[i] = i;
        }

  
        for (int i = 0; i < N; i++) {
            pthread_create(P + i, NULL, meal, Nums + i);
        }


        for (int i = 0; i < N; i++) {
            pthread_join(P[i], NULL);
        }


        for (int i = 0; i < N; i++) {
            pthread_mutex_destroy(f + i);
        }
        free(f);
        free(hungerOfPhylosophs);
        free(P);
        free(Nums);

        printf("All philosophers finished\n");
    }
    return 0;
}

// Functions to pick up and put down the forks
void takeLeftFork(int name) {
    if (name == 0)
        pthread_mutex_lock(f + N - 1); // Philosopher 0 picks the last fork as the left fork
    else
        pthread_mutex_lock(f + name - 1);
}

void takeRightFork(int name) {
    pthread_mutex_lock(f + name);
}

void putLeftFork(int name) {
    if (name == 0)
        pthread_mutex_unlock(f + N - 1);
    else
        pthread_mutex_unlock(f + name - 1);
}

void putRightFork(int name) {
    pthread_mutex_unlock(f + name);
}


void* meal(void* pName) {
    int name = *(int*)pName;
    while (hungerOfPhylosophs[name] > 0) {

        if (name == 0) {
            takeRightFork(name);
            takeLeftFork(name);
        } else {
            takeLeftFork(name);
            takeRightFork(name);
        }


        hungerOfPhylosophs[name]--;
        printf("Philosopher %d is eating for 3 seconds, hunger left: %d\n", name, hungerOfPhylosophs[name]);
        sleep(3);


        putLeftFork(name);
        putRightFork(name);


        printf("Philosopher %d is thinking for 3 seconds\n", name);
        sleep(3);
    }
    printf("Philosopher %d has finished eating and is now only thinking\n", name);
    return NULL;
}

