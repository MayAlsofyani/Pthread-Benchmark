#include <pthread.h>
#include <unistd.h>
#include <stdio.h>
#include<math.h>
int tickets = 20;
pthread_mutex_t mutex;

void *mythread1(void)
{
    while (1)
    {

        if (tickets > 0)
        {
            usleep(1000);
            printf("ticketse1 sells ticket:%d\n", tickets--);
        }
        else
        {

            break;
        }
        sleep(1);
    }
    return (void *)0;
}
void *mythread2(void)
{
    while (1)
    {

        if (tickets > 0)
        {
            usleep(1000);
            printf("ticketse2 sells ticket:%d\n", tickets--);
 
        }
        else
        {
  
            break;
        }
        sleep(1);
    }
    return (void *)0;
}

int main(int argc, const char *argv[])
{
    //int i = 0;
    int ret = 0;
    pthread_t id1, id2;

    ret = pthread_create(&id1, NULL, (void *)mythread1, NULL);
    if (ret)
    {
        printf("Create pthread error!\n");
        return 1;
    }

    ret = pthread_create(&id2, NULL, (void *)mythread2, NULL);
    if (ret)
    {
        printf("Create pthread error!\n");
        return 1;
    }

    pthread_join(id1, NULL);
    pthread_join(id2, NULL);

    return 0;
}
