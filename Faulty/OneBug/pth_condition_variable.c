/* Arquivo:  
 *    pth_condition_variable.c
 *
 * PropÃ³sito:
 *    Demonstrar Condition Variable em C
 *
 *
 * Compile:  gcc -g -Wall -o pth_condition_variable pth_condition_variable.c -lpthread -lrt
 * Usage:    ./pth_condition_variable 
 *
 */

#include <pthread.h>
#include <stdio.h>
#include <unistd.h>
 
pthread_cond_t cond1 = PTHREAD_COND_INITIALIZER;
 
pthread_mutex_t lock = PTHREAD_MUTEX_INITIALIZER;
 
int done = 1;
 
void* execute()
{
    
    if (done == 1) {
        done = 2;
        printf("Esperando por cond1\n");
        pthread_cond_wait(&cond1, &lock);
    }
    else {
        printf("Sinalizando cond1\n");
         // Unlock before signaling
        pthread_cond_signal(&cond1);
        return NULL; // Return immediately to avoid double unlocking
    }

    
    printf("Fim da thread\n");

    return NULL;
}
 
int main()
{
    pthread_t tid1, tid2;
 
    pthread_create(&tid1, NULL, execute, NULL);
 
    sleep(5);
 
    pthread_create(&tid2, NULL, execute, NULL);
 
    pthread_join(tid2, NULL);
 
    return 0;
}
