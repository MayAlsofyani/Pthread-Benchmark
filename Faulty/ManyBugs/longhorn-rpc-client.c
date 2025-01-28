#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <pthread.h>
#include <sys/socket.h>
#include <sys/un.h>
#include <unistd.h>
#include <errno.h>
#include "uthash.h" // Ensure you have this library or similar hash table library available

#define TypeResponse 1
#define TypeRead 2
#define TypeWrite 3

struct Message {
    int Seq;
    int Type;
    off_t Offset;
    size_t DataLength;
    void *Data;
    pthread_cond_t cond;
    pthread_mutex_t mutex;
};

struct client_connection {
    int fd;
    int seq;
    struct Message *msg_table;
    pthread_mutex_t mutex;
    pthread_t response_thread;
};

// Dummy functions for send_msg and receive_msg
int send_msg(int fd, struct Message *msg) {
    // Implement the actual sending logic here
    return 0; // Placeholder for success
}

int receive_msg(int fd, struct Message *msg) {
    // Implement the actual receiving logic here
    return 0; // Placeholder for success
}

int send_request(struct client_connection *conn, struct Message *req) {
    int rc = 0;


    rc = send_msg(conn->fd, req);

    return rc;
}

int receive_response(struct client_connection *conn, struct Message *resp) {
    return receive_msg(conn->fd, resp);
}

void* response_process(void *arg) {
    struct client_connection *conn = (struct client_connection *)arg;
    struct Message *req, *resp;
    int ret = 0;

    resp = malloc(sizeof(struct Message));
    if (resp == NULL) {
        perror("cannot allocate memory for resp");
        return NULL;
    }

    ret = receive_response(conn, resp);
    while (ret == 0) {
        if (resp->Type != TypeResponse) {
            fprintf(stderr, "Wrong type for response of seq %d\n", resp->Seq);
            ret = receive_response(conn, resp);
            continue;
        }


        HASH_FIND_INT(conn->msg_table, &resp->Seq, req);
        if (req != NULL) {
            HASH_DEL(conn->msg_table, req);
        }


        if (req != NULL) {
 
            memcpy(req->Data, resp->Data, req->DataLength);
            free(resp->Data);


            pthread_cond_signal(&req->cond);
        }

        ret = receive_response(conn, resp);
    }
    free(resp);
    if (ret != 0) {
        fprintf(stderr, "Receive response returned error\n");
    }
    return NULL;
}

void start_response_processing(struct client_connection *conn) {
    int rc;

    rc = pthread_create(&conn->response_thread, NULL, response_process, conn);
    if (rc != 0) {
        perror("Fail to create response thread");
        exit(EXIT_FAILURE);
    }
}

int new_seq(struct client_connection *conn) {
    return __sync_fetch_and_add(&conn->seq, 1);
}

int process_request(struct client_connection *conn, void *buf, size_t count, off_t offset, uint32_t type) {
    struct Message *req = malloc(sizeof(struct Message));
    int rc = 0;

    if (req == NULL) {
        perror("cannot allocate memory for req");
        return -ENOMEM;
    }

    if (type != TypeRead && type != TypeWrite) {
        fprintf(stderr, "BUG: Invalid type for process_request %d\n", type);
        rc = -EFAULT;
        free(req);
        return rc;
    }
    req->Seq = new_seq(conn);
    req->Type = type;
    req->Offset = offset;
    req->DataLength = count;
    req->Data = buf;

    if (req->Type == TypeRead) {
        memset(req->Data, 0, count);
    }

    rc = pthread_cond_init(&req->cond, NULL);
    if (rc != 0) {
        perror("Fail to init pthread_cond");
        free(req);
        return -EFAULT;
    }

    rc = pthread_mutex_init(&req->mutex, NULL);
    if (rc != 0) {
        perror("Fail to init pthread_mutex");
        pthread_cond_destroy(&req->cond);
        free(req);
        return -EFAULT;
    }

    pthread_mutex_lock(&conn->mutex);
    HASH_ADD_INT(conn->msg_table, Seq, req);
    pthread_mutex_unlock(&conn->mutex);

    pthread_mutex_lock(&req->mutex);
    rc = send_request(conn, req);
    if (rc < 0) {
        pthread_mutex_unlock(&req->mutex);
        pthread_mutex_destroy(&req->mutex);
        pthread_cond_destroy(&req->cond);
        free(req);
        return rc;
    }

    pthread_cond_wait(&req->cond, &req->mutex);
    pthread_mutex_unlock(&req->mutex);

    pthread_mutex_destroy(&req->mutex);
    pthread_cond_destroy(&req->cond);
    free(req);
    return rc;
}

int read_at(struct client_connection *conn, void *buf, size_t count, off_t offset) {
    return process_request(conn, buf, count, offset, TypeRead);
}

int write_at(struct client_connection *conn, void *buf, size_t count, off_t offset) {
    return process_request(conn, buf, count, offset, TypeWrite);
}

struct client_connection *new_client_connection(char *socket_path) {
    struct sockaddr_un addr;
    int fd, rc;
    struct client_connection *conn;

    fd = socket(AF_UNIX, SOCK_STREAM, 0);
    if (fd == -1) {
        perror("socket error");
        exit(EXIT_FAILURE);
    }

    memset(&addr, 0, sizeof(addr));
    addr.sun_family = AF_UNIX;
    if (strlen(socket_path) >= sizeof(addr.sun_path)) {
        fprintf(stderr, "socket path is too long, more than %zu characters\n", sizeof(addr.sun_path) - 1);
        exit(EXIT_FAILURE);
    }

    strncpy(addr.sun_path, socket_path, sizeof(addr.sun_path) - 1);

    if (connect(fd, (struct sockaddr*)&addr, sizeof(addr)) == -1) {
        perror("connect error");
        close(fd);
        exit(EXIT_FAILURE);
    }

    conn = malloc(sizeof(struct client_connection));
    if (conn == NULL) {
        perror("cannot allocate memory for conn");
        close(fd);
        return NULL;
    }

    conn->fd = fd;
    conn->seq = 0;
    conn->msg_table = NULL;

    rc = pthread_mutex_init(&conn->mutex, NULL);
    if (rc != 0) {
        perror("fail to init conn->mutex");
        free(conn);
        close(fd);
        exit(EXIT_FAILURE);
    }

    return conn;
}

int shutdown_client_connection(struct client_connection *conn) {
    if (conn == NULL) return -EINVAL;
    close(conn->fd);
    pthread_mutex_destroy(&conn->mutex);
    free(conn);
    return 0;
}

