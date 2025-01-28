#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <pthread.h>

enum {
    QUEUE_BUF_SIZE = 100000,
};

typedef struct vector_s {
    int size;
    int tail;
    void **buf;
} vector_t;

typedef struct queue_cycl_s {
    int front;
    int tail;
    int max_size;
    void **cyclic_buf;
} queue_cycl_t;

vector_t *vector_init(int size) {
    vector_t *tmp = (vector_t *)calloc(1, sizeof(vector_t));
    if (tmp == NULL) {
        fprintf(stderr, "vector_init error\n");
        exit(1);
    }

    tmp->buf = (void **)calloc(size, sizeof(void *));
    if (tmp->buf == NULL) {
        fprintf(stderr, "vector_init error\n");
        free(tmp);
        exit(1);
    }

    tmp->size = size;
    tmp->tail = 0;

    return tmp;
}

int vector_push_back(vector_t *v, void *data) {
    if (v->tail == v->size)
        return 0;

    v->buf[v->tail++] = data;

    return 1;
}

void *vector_pop_back(vector_t *v) {
    return (v->tail == 0) ? NULL : v->buf[--(v->tail)];
}

void vector_delete(vector_t *v) {
    free(v->buf);
    free(v);
}

queue_cycl_t *queue_init() {
    queue_cycl_t *tmp = (queue_cycl_t *)calloc(1, sizeof(queue_cycl_t));
    if (tmp == NULL) {
        fprintf(stderr, "queue_init error\n");
        exit(1);
    }

    tmp->max_size = QUEUE_BUF_SIZE + 1;
    tmp->cyclic_buf = (void **)calloc(tmp->max_size, sizeof(void *));
    if (tmp->cyclic_buf == NULL) {
        fprintf(stderr, "queue_init error\n");
        free(tmp);
        exit(1);
    }

    return tmp;
}

int queue_size(queue_cycl_t *q) {
    return (q->tail >= q->front) ? (q->tail - q->front) : (q->max_size - q->front + q->tail);
}

int queue_is_full(queue_cycl_t *q) {
    return ((q->tail + 1) % q->max_size == q->front);
}

int queue_is_empty(queue_cycl_t *q) {
    return (q->front == q->tail);
}

int queue_enqueue(queue_cycl_t *q, void *data) {
    if (queue_is_full(q))
        return 0;

    q->cyclic_buf[q->tail] = data;
    q->tail = (q->tail + 1) % q->max_size;

    return 1;
}

void *queue_dequeue(queue_cycl_t *q) {
    if (queue_is_empty(q))
        return NULL;
    void *tmp = q->cyclic_buf[q->front];
    q->cyclic_buf[q->front] = NULL;
    q->front = (q->front + 1) % q->max_size;

    return tmp;
}

vector_t *queue_dequeueall(queue_cycl_t *q) {
    int s = queue_size(q);
    vector_t *tmp = vector_init(s);
    void *data = NULL;

    while ((data = queue_dequeue(q)) != NULL) {
        if (!vector_push_back(tmp, data)) {
            queue_enqueue(q, data);
            fprintf(stderr, "queue_dequeueall error\n");
            exit(1);
        }
    }

    return tmp;
}

void queue_delete(queue_cycl_t *q) {
    free(q->cyclic_buf);
    free(q);
}

typedef struct actor_s {
    pthread_t thread;
    pthread_mutex_t m;
    pthread_cond_t cond;
    queue_cycl_t *q;
} actor_t;

void actor_send_to(actor_t *a, void *msg) {
    pthread_mutex_lock(&a->m);
    queue_enqueue(a->q, msg);
    pthread_cond_signal(&a->cond);
    pthread_mutex_unlock(&a->m);
}

void *actor_runner(void *arg) {
    actor_t *iam = (actor_t *)arg;
    int buf[50] = {0};

    while (1) {
        pthread_mutex_lock(&iam->m);
        while (queue_is_empty(iam->q)) {
            pthread_cond_wait(&iam->cond, &iam->m);
        }
        vector_t *v = queue_dequeueall(iam->q);
        pthread_mutex_unlock(&iam->m);

        int *data = NULL, exit_flag = 0;
        while ((data = vector_pop_back(v)) != NULL) {
            if (*data == -1) {
                exit_flag = 1;
            } else {
                buf[*data]++;
            }
            free(data);
        }
        vector_delete(v);
        if (exit_flag) break;
    }

    for (int i = 0; i < 50; i++) {
        if (buf[i] != n_senders) {
            fprintf(stderr, "ERROR!!!!!!!!\n");
        }
    }

    return NULL;
}

actor_t *actor_init() {
    actor_t *tmp = (actor_t *)calloc(1, sizeof(actor_t));
    if (tmp == NULL) {
        fprintf(stderr, "actor_init error\n");
        exit(1);
    }

    pthread_mutex_init(&tmp->m, NULL);
    pthread_cond_init(&tmp->cond, NULL);
    tmp->q = queue_init();
    pthread_create(&tmp->thread, NULL, actor_runner, tmp);

    return tmp;
}

void actor_finalize(actor_t *a) {
    pthread_join(a->thread, NULL);
    queue_delete(a->q);
    pthread_mutex_destroy(&a->m);
    pthread_cond_destroy(&a->cond);
    free(a);
}

void *sender(void *arg) {
    actor_t *receiver = (actor_t *)arg;
    int *msg = NULL;

    for (int i = 0; i < 50; i++) {
        msg = (int *)calloc(1, sizeof(int));
        if (msg == NULL) {
            fprintf(stderr, "Memory allocation failed\n");
            exit(1);
        }
        *msg = i;
        actor_send_to(receiver, msg);
    }

    return NULL;
}

int main(int argc, char **argv) {
    if (argc != 2) {
        fprintf(stderr, "Usage: %s <number_of_senders>\n", argv[0]);
        exit(1);
    }

    n_senders = atoi(argv[1]);
    pthread_t *senders_id = (pthread_t *)calloc(n_senders, sizeof(pthread_t));
    if (senders_id == NULL) {
        fprintf(stderr, "Memory allocation failed\n");
        exit(1);
    }

    actor_t *actor = actor_init();
    for (int i = 0; i < n_senders; i++) {
        pthread_create(&senders_id[i], NULL, sender, actor);
    }

    for (int i = 0; i < n_senders; i++) {
        pthread_join(senders_id[i], NULL);
    }

    int *msg_ext = (int *)calloc(1, sizeof(int));
    *msg_ext = -1;
    actor_send_to(actor, msg_ext);
    actor_finalize(actor);

    free(senders_id);
    return 0;
}

