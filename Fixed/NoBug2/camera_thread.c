#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include <pthread.h>
#include <signal.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/ioctl.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <linux/videodev2.h>

int sockfd;
int cameraFd;
unsigned char *devconfp;
pthread_mutex_t mutex = PTHREAD_MUTEX_INITIALIZER;
pthread_cond_t cond = PTHREAD_COND_INITIALIZER;

void sig_handler(int signo) {
    if (signo == SIGINT) {
        printf("SIGINT received, exiting!\n");
        close(sockfd);
        if (cameraFd >= 0) {
            close(cameraFd);
        }
        exit(0);
    }
}

unsigned char *temp;
ssize_t length;

void get_frame(int cameraFd) {
    struct v4l2_buffer buf;
    buf.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
    buf.memory = V4L2_MEMORY_MMAP;

    if (ioctl(cameraFd, VIDIOC_DQBUF, &buf) == -1) {
        perror("VIDIOC_DQBUF");
        exit(1);
    }

    length = buf.bytesused;
    temp = (unsigned char *)malloc(length);
    if (!temp) {
        perror("malloc");
        exit(1);
    }
    memcpy(temp, (unsigned char *)buf.m.userptr, length);

    if (ioctl(cameraFd, VIDIOC_QBUF, &buf) == -1) {
        perror("VIDIOC_QBUF");
        free(temp);
        exit(1);
    }
}

int stop = 0;

void *frame_capture_thread(void *arg) {
    enum v4l2_buf_type type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
    while (!stop) {
        get_frame(cameraFd);
        pthread_mutex_lock(&mutex);
        devconfp = (unsigned char*)malloc(length);
        if (!devconfp) {
            perror("malloc");
            pthread_mutex_unlock(&mutex);
            exit(1);
        }
        memcpy(devconfp, temp, length);
        pthread_mutex_unlock(&mutex);
        pthread_cond_broadcast(&cond);
        usleep(1500);
        free(temp);
    }

    if (ioctl(cameraFd, VIDIOC_STREAMOFF, &type) == -1) {
        perror("VIDIOC_STREAMOFF");
        exit(1);
    }
    return NULL;
}

void send_msg(int fd) {
    ssize_t size;
    char readbuffer[1024] = {'\0'};
    char writebuffer[1024] = {'\0'};
    size = read(fd, readbuffer, sizeof(readbuffer) - 1);
    if (size < 0) {
        perror("read");
        exit(1);
    }

    char respbuffer[] = "HTTP/1.0 200 OK\r\nConnection: close\r\n"
                        "Server: Net-camera-1-0\r\nCache-Control: no-store, no-cache, must-revalidate\r\n"
                        "Pragma: no-cache\r\nContent-Type: multipart/x-mixed-replace; boundary=www.briup.com\r\n\r\n";
    if (write(fd, respbuffer, strlen(respbuffer)) != strlen(respbuffer)) {
        perror("write");
        exit(1);
    }

    if (strstr(readbuffer, "snapshot")) {
        pthread_mutex_lock(&mutex);
        if (pthread_cond_wait(&cond, &mutex) != 0) {
            perror("pthread_cond_wait");
            pthread_mutex_unlock(&mutex);
            exit(1);
        }

        sprintf(writebuffer, "--www.briup.com\nContent-Type: image/jpeg\nContent-Length: %zu\n\n", length);
        write(fd, writebuffer, strlen(writebuffer));
        write(fd, devconfp, length);

        free(devconfp);
        pthread_mutex_unlock(&mutex);
    } else {
        while (1) {
            pthread_mutex_lock(&mutex);
            if (pthread_cond_wait(&cond, &mutex) != 0) {
                perror("pthread_cond_wait");
                pthread_mutex_unlock(&mutex);
                exit(1);
            }

            sprintf(writebuffer, "--www.briup.com\nContent-Type: image/jpeg\nContent-Length: %zu\n\n", length);
            write(fd, writebuffer, strlen(writebuffer));
            write(fd, devconfp, length);

            pthread_mutex_unlock(&mutex);
            sleep(1);
        }
    }
}

void *service_thread(void *arg) {
    int fd = (intptr_t)arg;
    send_msg(fd);
    close(fd);
    return NULL;
}

int main(int argc, char *argv[]) {
    if (argc != 3) {
        fprintf(stderr, "Usage: %s <device> <port>\n", argv[0]);
        exit(1);
    }

    if (signal(SIGINT, sig_handler) == SIG_ERR) {
        perror("signal");
        exit(1);
    }

    if ((cameraFd = open(argv[1], O_RDWR)) < 0) {
        perror("open");
        exit(1);
    }

    sockfd = socket(AF_INET, SOCK_STREAM, 0);
    if (sockfd < 0) {
        perror("socket");
        exit(1);
    }

    struct sockaddr_in saddr = {0};
    saddr.sin_family = AF_INET;
    saddr.sin_port = htons(atoi(argv[2]));
    saddr.sin_addr.s_addr = INADDR_ANY;

    if (bind(sockfd, (struct sockaddr *)&saddr, sizeof(saddr)) < 0) {
        perror("bind");
        exit(1);
    }

    if (listen(sockfd, 10) < 0) {
        perror("listen");
        exit(1);
    }

    pthread_t capture_thread;
    if (pthread_create(&capture_thread, NULL, frame_capture_thread, NULL) != 0) {
        perror("pthread_create");
        exit(1);
    }

    while (1) {
        int client_fd = accept(sockfd, NULL, NULL);
        if (client_fd < 0) {
            perror("accept");
            continue;
        }

        pthread_t service;
        if (pthread_create(&service, NULL, service_thread, (void *)(intptr_t)client_fd) != 0) {
            perror("pthread_create");
        }
    }

    return 0;
}

