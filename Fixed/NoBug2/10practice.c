#include <stdio.h>
#include <pthread.h>
#include <errno.h>

pthread_mutex_t lock_flag;
int flag = 1;

void *handlerA(void *arg)
{
	int i =0;
	while(1)
	{	
		pthread_mutex_lock(&lock_flag);
		if(flag == 1)
		{
		flag = 2;
			printf("handlerA:  A\n");
		i++;
		}
		pthread_mutex_unlock(&lock_flag);
			sleep(1);
		if(i >= 10)
			break;
	}
}
void *handlerB(void *arg)
{
	int i = 0;
	while(1)
	{	
		pthread_mutex_lock(&lock_flag);
		if(flag == 2)
		{
		flag = 3;
			printf("handlerB:  B\n");
		i++;
		}
		pthread_mutex_unlock(&lock_flag);
		sleep(1);
		if(i >= 10)
		break;	
	}

}
void *handlerC(void *arg)
{
	int i =0;
	while(1)
	{
		pthread_mutex_lock(&lock_flag);
		if(flag == 3)
		{
		flag = 1;
			printf("handlerC:  C\n");
		i++;	
		}
		pthread_mutex_unlock(&lock_flag);
		sleep(1);
		if(i >= 10)
		break;	
	}
	
}

int main()
{
	pthread_t pidA;
	pthread_t pidB;
	pthread_t pidC;

	int ret;
	pthread_mutex_init(&lock_flag,NULL);

	ret = pthread_create(&pidA,NULL,handlerA,&flag);
	if(ret < 0)
	{
		perror("pthread create");
		return -1;
	}

	ret = pthread_create(&pidB,NULL,handlerB,&flag);
	if(ret < 0)
	{
		perror("pthread create");
		return -1;
	}

	ret = pthread_create(&pidC,NULL,handlerC,&flag);
	if(ret < 0)
	{
		perror("pthread create");
		return -1;
	}

	pthread_join(pidA,NULL);
	pthread_join(pidB,NULL);
	pthread_join(pidC,NULL);
	return 0;
}
