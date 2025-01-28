#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <sys/time.h>
#include <sys/resource.h>

double get_time(){
        struct timeval t;
        struct timezone tzp;
        gettimeofday(&t, &tzp);
        return t.tv_sec + t.tv_usec*1e-6;
}


void *increment_counter(void *ptr);

int counter = 0;
pthread_mutex_t mutex1 = PTHREAD_MUTEX_INITIALIZER;

int main(){
	int x;
	double startTime = get_time();
	for(x = 0; x < 1000; x++){
		counter = 0;
		pthread_t thread1, thread2, thread3, thread4;
		int amount1 = 100;
		int amount2 = 100;
		int amount3 = 100;
		int amount4 = 100;
		int iret1, iret2, iret3, iret4;
		iret1 = pthread_create( &thread1, NULL, increment_counter,(void *) &amount1);
		iret2 = pthread_create( &thread2, NULL, increment_counter,(void *) &amount2);
		iret3 = pthread_create( &thread3, NULL, increment_counter,(void *) &amount3);
		iret4 = pthread_create( &thread4, NULL, increment_counter,(void *) &amount4);
		
		pthread_join(thread1, NULL);
		pthread_join(thread2, NULL);
		pthread_join(thread3, NULL);
		pthread_join(thread4, NULL);
		
		if(counter != 400){	
			//printf("%d\n", counter);
		}
		
	}
	double endTime = get_time();
	double total_time = endTime - startTime;
	printf("%f\n", total_time);
}

void *increment_counter(void *ptr){
	int *amount;
	amount= (int *) ptr;
	int i;
	for(i = 0; i < *amount; i++){
		
		pthread_mutex_lock( &mutex1);
		counter++;
		pthread_mutex_unlock( &mutex1);
	}
}
