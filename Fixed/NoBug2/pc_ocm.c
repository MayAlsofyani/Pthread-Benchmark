#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <pthread.h>
#include <errno.h>
#include <string.h>
#include <fcntl.h>
#include <sys/mman.h>

void *producer(void *args);
void *consumer(void *args);

typedef struct {
    int a_mem[0xffff];
    int b_mem[0xffff];

    pthread_mutex_t *a_mutex;
    pthread_mutex_t *b_mutex;

    int a_is_processing;
    int b_is_processing;
    int a_is_empty;
    int b_is_empty;

    pthread_cond_t *a_is_ready, *a_isnt_processing;
    pthread_cond_t *b_is_ready, *b_isnt_processing;
} queue;

queue *queueInit(void);
void queueDelete(queue *q);

int main() {
    queue *fifo;
    pthread_t pro, con;

    fifo = queueInit();
    if (fifo == 0) {
        fprintf(stderr, "main: Queue Init failed.\n");
        exit(1);
    }

    pthread_create(&pro, NULL, producer, fifo);
    pthread_create(&con, NULL, consumer, fifo);
    pthread_join(pro, NULL);
    pthread_join(con, NULL);
    queueDelete(fifo);

    return 0;
}

void *producer(void *q) {
    queue *fifo;
    int i;

    fifo = (queue *)q;

    int fd = 0;
    void *map_base = 0, *virt_addr = 0;

    const uint32_t ALT_H2F_BASE = 0xC0000000;
    const uint32_t ALT_H2F_OCM_OFFSET = 0x00000000;

    off_t target = ALT_H2F_BASE;
    unsigned long read_result;
    int idx = 0;

    if ((fd = open("/dev/mem", O_RDWR | O_SYNC)) == -1) {
        fprintf(stderr, "Error at line %d, file %s (%d) [%s]\n", __LINE__, __FILE__, errno, strerror(errno));
        exit(1);
    }
    printf("/dev/mem opened.\n"); fflush(stdout);

    map_base = mmap(0, 0x44000, PROT_READ | PROT_WRITE, MAP_SHARED, fd, target & ~(0x44000 - 1));
    printf("Memory mapped at address %p.\n", map_base); fflush(stdout);
    virt_addr = map_base + (target & (0x44000 - 1));

    for (i = 0; i < 20; i++) {
        pthread_mutex_lock(fifo->a_mutex);
        while (fifo->a_is_processing) {
            printf("producer: memory A is processing.\n");
            pthread_cond_wait(fifo->a_isnt_processing, fifo->a_mutex);
        }

        fifo->a_is_empty = 0;
        printf("producer: copying a\n");
        for (idx = 0; idx < 0xffff; idx++) {
            void* access_addr = virt_addr + ALT_H2F_OCM_OFFSET + idx * 4;
            read_result = *((uint32_t*) access_addr);
            fifo->a_mem[idx] = read_result;
        }
        printf("producer: copying a (end)\n");
        pthread_mutex_unlock(fifo->a_mutex);
        pthread_cond_signal(fifo->a_is_ready);

        pthread_mutex_lock(fifo->b_mutex);
        while (fifo->b_is_processing) {
            printf("producer: memory B is processing.\n");
            pthread_cond_wait(fifo->b_isnt_processing, fifo->b_mutex);
        }

        fifo->b_is_empty = 0;
        printf("producer: copying b\n");
        for (idx = 0; idx < 0xffff; idx++) {
            void* access_addr = virt_addr + ALT_H2F_OCM_OFFSET + idx * 4;
            read_result = *((uint32_t*) access_addr);
            fifo->b_mem[idx] = read_result;
        }
        printf("producer: copying b (end)\n");
        pthread_mutex_unlock(fifo->b_mutex);
        pthread_cond_signal(fifo->b_is_ready);
    }

    if (munmap(map_base, 0x44000) == -1) {
        fprintf(stderr, "Error at line %d, file %s (%d) [%s]\n", __LINE__, __FILE__, errno, strerror(errno));
        exit(1);
    }
    close(fd);

    return NULL;
}

void *consumer(void *q) {
    queue *fifo;
    int i;

    fifo = (queue *)q;

    for (i = 0; i < 20; i++) {
        pthread_mutex_lock(fifo->a_mutex);
        while (fifo->a_is_empty) {
            printf("consumer: memory A is empty, wait producer.\n");
            pthread_cond_wait(fifo->a_is_ready, fifo->a_mutex);
        }

        fifo->a_is_processing = 1;
        printf("image process A\n");
        usleep(10);

        fifo->a_is_processing = 0;
        fifo->a_is_empty = 1;
        pthread_mutex_unlock(fifo->a_mutex);
        pthread_cond_signal(fifo->a_isnt_processing);

        pthread_mutex_lock(fifo->b_mutex);
        while (fifo->b_is_empty) {
            printf("consumer: memory B is empty, wait producer.\n");
            pthread_cond_wait(fifo->b_is_ready, fifo->b_mutex);
        }

        fifo->b_is_processing = 1;
        printf("image process B\n");
        usleep(10);

        fifo->b_is_processing = 0;
        fifo->b_is_empty = 1;
        pthread_mutex_unlock(fifo->b_mutex);
        pthread_cond_signal(fifo->b_isnt_processing);
    }

    return NULL;
}

queue *queueInit(void) {
    queue *q;

    q = (queue *)malloc(sizeof(queue));
    if (q == NULL) return NULL;

    q->a_is_processing = 0;
    q->b_is_processing = 0;
    q->a_is_empty = 1;
    q->b_is_empty = 1;

    q->a_mutex = (pthread_mutex_t *)malloc(sizeof(pthread_mutex_t));
    pthread_mutex_init(q->a_mutex, NULL);
    q->b_mutex = (pthread_mutex_t *)malloc(sizeof(pthread_mutex_t));
    pthread_mutex_init(q->b_mutex, NULL);

    q->a_is_ready = (pthread_cond_t *)malloc(sizeof(pthread_cond_t));
    pthread_cond_init(q->a_is_ready, NULL);
    q->b_is_ready = (pthread_cond_t *)malloc(sizeof(pthread_cond_t));
    pthread_cond_init(q->b_is_ready, NULL);
    q->a_isnt_processing = (pthread_cond_t *)malloc(sizeof(pthread_cond_t));
    pthread_cond_init(q->a_isnt_processing, NULL);
    q->b_isnt_processing = (pthread_cond_t *)malloc(sizeof(pthread_cond_t));
    pthread_cond_init(q->b_isnt_processing, NULL);

    return q;
}

void queueDelete(queue *q) {
    pthread_mutex_destroy(q->a_mutex);
    free(q->a_mutex);
    pthread_mutex_destroy(q->b_mutex);
    free(q->b_mutex);

    pthread_cond_destroy(q->a_is_ready);
    free(q->a_is_ready);
    pthread_cond_destroy(q->b_is_ready);
    free(q->b_is_ready);
    pthread_cond_destroy(q->a_isnt_processing);
    free(q->a_isnt_processing);
    pthread_cond_destroy(q->b_isnt_processing);
    free(q->b_isnt_processing);

    free(q);
}

