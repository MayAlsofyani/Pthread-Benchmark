#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <pthread.h>
#include <fcntl.h>
#include <sys/epoll.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <errno.h>
#include <time.h>

#define MAX_CHANNEL 5
#define LISTEN_BACKLOG 50

pthread_t thread[MAX_CHANNEL];
pthread_mutex_t mut = PTHREAD_MUTEX_INITIALIZER;

int clients[100];
char msg[100];
int max_clients = 100;

int setnonblocking(int sock) {
    int opts = fcntl(sock, F_GETFL);
    if (opts < 0) {
        perror("fcntl(F_GETFL)");
        return -1;
    }
    opts = opts | O_NONBLOCK;
    if (fcntl(sock, F_SETFL, opts) < 0) {
        perror("fcntl(F_SETFL)");
        return -1;
    }
    return 1;
}

void* thread_connect() {
    int listenfd, connfd;

    listenfd = create_conn(10023);
    if (listenfd <= 0) {
        perror("create_conn");
        exit(EXIT_FAILURE);
    }

    memset(clients, 0, sizeof(clients));

    int epollfd = epoll_create(101);
    if (epollfd == -1) {
        perror("epoll_create");
        exit(EXIT_FAILURE);
    }

    struct epoll_event ev, events[150];
    ev.events = EPOLLIN;
    ev.data.fd = listenfd;
    if (epoll_ctl(epollfd, EPOLL_CTL_ADD, listenfd, &ev) == -1) {
        perror("epoll_ctl");
        exit(EXIT_FAILURE);
    }

    for (;;) {
        int nfds = epoll_wait(epollfd, events, 150, -1);
        if (nfds == -1) {
            perror("epoll_wait");
            exit(EXIT_FAILURE);
        }

        for (int i = 0; i < nfds; ++i) {
            int currfd = events[i].data.fd;
            if (currfd == listenfd) {
                int conn_sock = accept(listenfd, NULL, NULL);
                if (conn_sock == -1) {
                    perror("accept");
                    continue;
                }

                int saved = 0;
                pthread_mutex_lock(&mut);
                for (int n = 0; n < max_clients; ++n) {
                    if (clients[n] == 0) {
                        clients[n] = conn_sock;
                        saved = 1;
                        break;
                    }
                }
                pthread_mutex_unlock(&mut);

                if (saved == 0) {
                    close(conn_sock);
                    continue;
                }

                setnonblocking(conn_sock);
                ev.events = EPOLLIN | EPOLLET;
                ev.data.fd = conn_sock;
                if (epoll_ctl(epollfd, EPOLL_CTL_ADD, conn_sock, &ev) == -1) {
                    perror("epoll_ctl");
                    exit(EXIT_FAILURE);
                }
            } else {
                char str[50];
                int n = read(currfd, str, sizeof(str));
                if (n <= 0) {
                    pthread_mutex_lock(&mut);
                    for (int j = 0; j < max_clients; ++j) {
                        if (clients[j] == currfd) {
                            clients[j] = 0;
                            break;
                        }
                    }
                    pthread_mutex_unlock(&mut);
                    close(currfd);
                    epoll_ctl(epollfd, EPOLL_CTL_DEL, currfd, NULL);
                } else {
                    pthread_mutex_lock(&mut);
                    strncpy(msg, str, n);
                    pthread_mutex_unlock(&mut);
                }
            }
        }
    }
}

void* thread_proc() {
    while (1) {
        pthread_mutex_lock(&mut);
        int len = strlen(msg);
        if (len > 0) {
            for (int i = 0; i < max_clients; ++i) {
                if (clients[i] > 0) {
                    write(clients[i], msg, len);
                }
            }
            memset(msg, 0, sizeof(msg));
        }
        pthread_mutex_unlock(&mut);
        usleep(200);
    }
}

void* thread_route() {
    while (1) {
        pthread_mutex_lock(&mut);
        if (strlen(msg) > 0) {
            printf("msg: %s\n", msg);
        }
        pthread_mutex_unlock(&mut);
        usleep(200000);
    }
}

int thread_create() {
    if (pthread_create(&thread[0], NULL, thread_connect, NULL) != 0) {
        perror("pthread_create");
        return -1;
    }

    if (pthread_create(&thread[1], NULL, thread_proc, NULL) != 0) {
        perror("pthread_create");
        return -1;
    }

    if (pthread_create(&thread[3], NULL, thread_route, NULL) != 0) {
        perror("pthread_create");
        return -1;
    }

    return 0;
}

void thread_wait() {
    for (int i = 0; i < 3; ++i) {
        if (thread[i] != 0) {
            pthread_join(thread[i], NULL);
            printf("Thread %d ended\n", i);
        }
    }
}

int create_conn(int port) {
    struct sockaddr_in servaddr;
    int listenfd;

    listenfd = socket(AF_INET, SOCK_STREAM, 0);
    if (listenfd < 0) {
        perror("socket");
        return -1;
    }

    memset(&servaddr, 0, sizeof(servaddr));
    servaddr.sin_family = AF_INET;
    servaddr.sin_addr.s_addr = htonl(INADDR_ANY);
    servaddr.sin_port = htons(port);

    if (bind(listenfd, (struct sockaddr *)&servaddr, sizeof(servaddr)) < 0) {
        perror("bind");
        return -1;
    }
    if (listen(listenfd, LISTEN_BACKLOG) == -1) {
        perror("listen");
        return -1;
    }

    return listenfd;
}

int main(int argc, char** argv) {
    pthread_mutex_init(&mut, NULL);
    if (thread_create() != 0) {
        printf("Error creating threads\n");
        exit(EXIT_FAILURE);
    }
    thread_wait();

    printf("Server main thread ended\n");

    return 0;
}

