#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <pthread.h>
#include <signal.h>
#include <unistd.h>

//A structure that contains the data
// to be passed to a new thread
struct DataForThread {
	char name[36];
	int limit;
	int grouping;
};

//This is the function we want to run
// in new threads using the pthreads library
void*  workerThreadFunc(void* data);

//Pointer to the mutex used to
// synchronize threads
pthread_mutex_t * mutex;

int main() {
	//the number of threads to start
	int numThreads = 5;

	//the id's of the threads we will start
	pthread_t threadIds[numThreads];

	//initialize the mutex
	mutex = (pthread_mutex_t *)malloc(sizeof(pthread_mutex_t));
	pthread_mutex_init(mutex,NULL);

	//create all the threads
	for(int i=1;i<=numThreads;i++) {
		//create a data structure containing
		// the data the new thread will need
		struct DataForThread * dft = (struct DataForThread *) malloc(sizeof(struct DataForThread));
		sprintf(dft->name,"Thread%d",i);
		dft->limit = i * 10;
		dft->grouping = i;

		//launch the thread with 'workerThreadFunc' as its main
		pthread_create(&threadIds[i - 1],NULL,workerThreadFunc,dft);
	}

	//wait for the threads to complete
	for(int i=1;i<=numThreads;i++) {
		printf("Waiting for thread %d\n",i);
		pthread_join(threadIds[i - 1],0);
		printf("Thread %d exited\n",i);
	}
	free(mutex);
	printf("Program complete...\n");
}

//many copies of this method will be run, each in a different thread
void* workerThreadFunc(void* data) {
	struct DataForThread* info = (struct DataForThread*)data;

	int count = 1;
	while(count <= info->limit) {
		//acquire the lock
		pthread_mutex_lock(mutex);
		//print the correct number of messages
		for(int i=0;i<info->grouping;i++) {
			printf("%s message %d of %d\n",info->name,count++,info->limit);
			sleep(1);
		}
		//release the lock
		pthread_mutex_unlock(mutex);
	}

	free(info);
	return 0;
}