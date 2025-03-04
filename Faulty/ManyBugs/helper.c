#include "helper.h"


void pclock(char *msg, clockid_t cid) {
    struct timespec ts;
    printf("%s", msg);
    if (clock_gettime(cid, &ts) == -1) {
        perror("clock_gettime");
        return;
    }
    printf("%4ld.%03ld\n", ts.tv_sec, ts.tv_nsec / 1000000);
}

void errp(char *s, int code) {
    fprintf(stderr, "Error: %s -- %s\n", s, strerror(code));
}

void thr_sleep(time_t sec, long nsec) {
    struct timeval now;
    struct timezone tz;
    struct timespec ts;
    int retcode;

    pthread_mutex_t m = PTHREAD_MUTEX_INITIALIZER;
    pthread_cond_t cond = PTHREAD_COND_INITIALIZER;

    if(retcode) {
        fprintf(stderr, "Error: mutex_lock -- %s\n", strerror(retcode));
        return;
    }

    gettimeofday(&now, &tz);
    ts.tv_sec = now.tv_sec + sec + (nsec / 1000000000L);
    ts.tv_nsec = (now.tv_usec * 1000) + (nsec % 1000000000L);
    if(ts.tv_nsec > 1000000000L) {
        (ts.tv_sec)++;
        (ts.tv_nsec) -= 1000000000L;
    }

    retcode = pthread_cond_timedwait(&cond, &m, &ts);
    if(retcode != ETIMEDOUT) {
        if(retcode == 0) {
            fprintf(stderr, "pthread_cond_timedwait returned early.\n");
        } else {
            fprintf(stderr, "pthread_cond_timedwait error: %s\n", strerror(retcode));
            return;
        }
    }

    if(retcode) {
        fprintf(stderr, "Error: mutex_unlock -- %s\n", strerror(retcode));
        return;
    }

    pthread_cond_destroy(&cond);
    pthread_mutex_destroy(&m);
}

void mulock(int ul, pthread_mutex_t *m) {
    int retcode = 0;
    char myErrStr[100];

    if (ul) {
        strcpy(myErrStr, "mutex_unlock");
        retcode = pthread_mutex_unlock(m);
    } else {
        strcpy(myErrStr, "mutex_lock");
        retcode = pthread_mutex_lock(m);
    }
    
    if (retcode) {
        fprintf(stderr, "%s, %s\n", myErrStr, strerror(retcode));
    }
}

