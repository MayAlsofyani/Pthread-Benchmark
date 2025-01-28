#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <string.h>
#include <pthread.h>




typedef struct node
{
    int data;
    struct node* next;
}Node;


Node *head = NULL;


pthread_mutex_t mutex;

pthread_cond_t cond;


int produced_num = 0;

void* th_producer(void *args)
{
    while(1)
    {
        Node *node = (Node *)malloc(sizeof(Node));
        node->data = rand() % 100;
  
        pthread_mutex_lock(&mutex);
        node->next = head;
        head = node;
        printf("%lu生产了一个烧饼:%d\n",pthread_self(),head->data);
        pthread_mutex_unlock(&mutex);


        if(++produced_num==3)
        {
            pthread_cond_signal(&cond);
        }
        sleep(rand() % 3);
    }
    return NULL;
}

void* th_consumer(void *args)
{
    while(1)
    {
        pthread_mutex_lock(&mutex);

        if(head == NULL)
        {
       
            pthread_cond_wait(&cond,&mutex);
            //continue;
           
        }
        Node * node = head;
        head = head->next;
        printf("%lu消费了一个烧饼:%d\n",pthread_self(),node->data);
        free(node);
        produced_num--;
        pthread_mutex_unlock(&mutex);
    }
    return NULL;
}
int main()
{

    pthread_t thid_producer,thid_consumer;

    //init
    pthread_mutex_init(&mutex,NULL);
    pthread_cond_init(&cond,NULL);


    pthread_create(&thid_producer,NULL,th_producer,NULL);

    pthread_create(&thid_consumer,NULL,th_consumer,NULL);


    pthread_join(thid_producer,NULL);
    pthread_join(thid_consumer,NULL);

    //destory
    pthread_mutex_destroy(&mutex);
    pthread_cond_destroy(&cond);

    return 0;
}
