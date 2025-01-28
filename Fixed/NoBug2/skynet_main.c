#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <pthread.h>
#include <signal.h>


extern void skynet_logger_error(int, const char *, ...);
extern void skynet_logger_notice(int, const char *, ...);
extern void skynet_malloc(size_t);
extern void skynet_free(void *);
extern void skynet_config_init(const char *);
extern void skynet_config_int(const char *, const char *, int *);
extern void skynet_config_string(const char *, const char *, char *, size_t);
extern void skynet_mq_init();
extern void skynet_service_init(const char *);
extern void skynet_logger_init(unsigned, const char *);
extern void skynet_timer_init();
extern void skynet_socket_init();
extern void skynet_service_create(const char *, unsigned, const char *, int);
extern void skynet_service_releaseall();
extern void skynet_socket_free();
extern void skynet_config_free();
extern void skynet_socket_exit();
extern int skynet_socket_poll();
extern int skynet_message_dispatch();
extern void skynet_updatetime();

struct monitor {
    int count;
    int sleep;
    int quit;
    pthread_t *pids;
    pthread_cond_t cond;
    pthread_mutex_t mutex;
};

static struct monitor *m = 0;

void create_thread(pthread_t *thread, void *(*start_routine)(void *), void *arg) {
    if (pthread_create(thread, 0, start_routine, arg)) {
        skynet_logger_error(0, "Create thread failed");
        exit(1);
    }
}

void wakeup(struct monitor *m, int busy) {
    pthread_mutex_lock(&m->mutex);
    if (m->sleep >= m->count - busy) {
        pthread_cond_signal(&m->cond);
    }
    pthread_mutex_unlock(&m->mutex);
}

void *thread_socket(void *p) {
    struct monitor *m = p;
    while (1) {
        pthread_mutex_lock(&m->mutex);
        if (m->quit) {
            pthread_mutex_unlock(&m->mutex);
            break;
        }
        pthread_mutex_unlock(&m->mutex);

        int r = skynet_socket_poll();
        if (r == 0)
            break;
        if (r < 0) {
            continue;
        }
        wakeup(m, 0);
    }
    return 0;
}

void *thread_timer(void *p) {
    struct monitor *m = p;
    while (1) {
        pthread_mutex_lock(&m->mutex);
        if (m->quit) {
            pthread_mutex_unlock(&m->mutex);
            break;
        }
        pthread_mutex_unlock(&m->mutex);

        skynet_updatetime();
        wakeup(m, m->count - 1);
        usleep(1000);
    }
    return 0;
}

void *thread_worker(void *p) {
    struct monitor *m = p;
    while (1) {
        pthread_mutex_lock(&m->mutex);
        if (m->quit) {
            pthread_mutex_unlock(&m->mutex);
            break;
        }
        pthread_mutex_unlock(&m->mutex);

        if (skynet_message_dispatch()) {
            pthread_mutex_lock(&m->mutex);
            ++m->sleep;

            if (!m->quit)
                pthread_cond_wait(&m->cond, &m->mutex);
            --m->sleep;
            pthread_mutex_unlock(&m->mutex);
        }
    }
    return 0;
}

void skynet_start(unsigned harbor, unsigned thread) {
    unsigned i;

    m = skynet_malloc(sizeof(*m));
    memset(m, 0, sizeof(*m));
    m->count = thread;
    m->sleep = 0;
    m->quit = 0;
    m->pids = skynet_malloc(sizeof(*m->pids) * (thread + 2));

    pthread_mutex_init(&m->mutex, 0);
    pthread_cond_init(&m->cond, 0);

    for (i = 0; i < thread; i++) {
        create_thread(m->pids + i, thread_worker, m);
    }
    create_thread(m->pids + i, thread_timer, m);
    create_thread(m->pids + i + 1, thread_socket, m);

    skynet_logger_notice(0, "skynet start, harbor:%u workers:%u", harbor, thread);

    for (i = 0; i < thread; i++) {
        pthread_join(*(m->pids + i), 0);
    }

    skynet_logger_notice(0, "skynet shutdown, harbor:%u", harbor);

    pthread_mutex_destroy(&m->mutex);
    pthread_cond_destroy(&m->cond);
    skynet_free(m->pids);
    skynet_free(m);
}

void skynet_shutdown(int sig) {
    skynet_logger_notice(0, "recv signal:%d", sig);

    pthread_mutex_lock(&m->mutex);
    m->quit = 1;
    skynet_socket_exit();
    pthread_cond_broadcast(&m->cond);
    pthread_mutex_unlock(&m->mutex);
}

void skynet_coredump(int sig) {
    skynet_shutdown(sig);
    skynet_service_releaseall();

    signal(sig, SIG_DFL);
    raise(sig);
}

void skynet_signal_init() {
    struct sigaction actTerminate;
    actTerminate.sa_handler = skynet_shutdown;
    sigemptyset(&actTerminate.sa_mask);
    actTerminate.sa_flags = 0;
    sigaction(SIGTERM, &actTerminate, 0);

    struct sigaction actCoredump;
    actCoredump.sa_handler = skynet_coredump;
    sigemptyset(&actCoredump.sa_mask);
    actCoredump.sa_flags = 0;
    sigaction(SIGSEGV, &actCoredump, 0);
    sigaction(SIGILL, &actCoredump, 0);
    sigaction(SIGFPE, &actCoredump, 0);
    sigaction(SIGABRT, &actCoredump, 0);

    sigset_t bset, oset;
    sigemptyset(&bset);
    sigaddset(&bset, SIGINT);

    pthread_sigmask(SIG_BLOCK, &bset, &oset);
}

int main(int argc, char *argv[]) {
  

    return 0;
}

