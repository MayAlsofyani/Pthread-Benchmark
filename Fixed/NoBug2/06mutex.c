#include <stdio.h>
#include <pthread.h>
#include <unistd.h>

pthread_mutex_t m;

void *func(void *p) {
    int *source = (int *)p;

    for (int i = 0; i < 10; i++) {
        pthread_mutex_lock(&m);
        int t = *(source + i);
        printf("t = %d\n", t);
        pthread_mutex_unlock(&m);

        int j;
        for (j = i; j > 0 && t < source[j - 1]; j--) {
            source[j] = source[j - 1];
        }
        source[j] = t;

        usleep(100);
    }

    return NULL;
}

int main() {
    int a[10];
    pthread_t id;


    pthread_mutex_init(&m, NULL);
    pthread_mutex_lock(&m);

    
    int success = pthread_create(&id, NULL, func, a);
    if (success != 0) {
        perror("pthread_create");
        return -1;
    }

    for (int i = 0; i < 10; i++) {
        printf("请输入第%d个数据: ", i + 1);
        scanf("%d", &a[i]);

        pthread_mutex_unlock(&m);
        usleep(100);
        pthread_mutex_lock(&m);
    }

    pthread_mutex_unlock(&m);

   
    pthread_join(id, NULL);


    printf("Sorted array: ");
    for (int i = 0; i < 10; i++) {
        printf("%d ", a[i]);
    }
    printf("\n");


    pthread_mutex_destroy(&m);
    return 0;
}

