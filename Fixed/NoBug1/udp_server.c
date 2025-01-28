#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <unistd.h>

#define FILE_SIZE 671088640  // 800 MB
#define BUFFER_SIZE 8

int thread_count = 0;
int server_file_des, bytes_read;
unsigned long long int block_size = 0, file_pos = 0, total_bytes = 0;
pthread_mutex_t mutex1 = PTHREAD_MUTEX_INITIALIZER;

void *receive_data(void *thread_id) {
    unsigned long long int start_pos, end_pos;
    unsigned long long int local_bytes = 0;
    char buffer[BUFFER_SIZE];

    pthread_mutex_lock(&mutex1);
    start_pos = file_pos;
    file_pos += block_size;
    pthread_mutex_unlock(&mutex1);

    end_pos = start_pos + block_size;

    struct sockaddr_in client_addr;
    socklen_t addr_len = sizeof(client_addr);

    while (start_pos < end_pos) {
        bytes_read = recvfrom(server_file_des, buffer, BUFFER_SIZE, 0, (struct sockaddr*)&client_addr, &addr_len);
        
        if (bytes_read > 0) {
            local_bytes += bytes_read;
            start_pos += bytes_read;
            printf("Thread %ld transferred %llu bytes (total transferred: %llu bytes)\n", (long)thread_id, local_bytes, start_pos);
        } else {
            perror("recvfrom");
            break;
        }
    }

    pthread_mutex_lock(&mutex1);
    total_bytes += local_bytes;
    pthread_mutex_unlock(&mutex1);

    pthread_exit(NULL);
}

int main() {
    int ch;
    printf("Perform Network Benchmarking on\n1. 1 Thread\n2. 2 Threads\n3. 4 Threads\n4. 8 Threads\n");
    scanf("%d", &ch);

    if (ch == 1 || ch == 2 || ch == 3 || ch == 4) {
        thread_count = (ch == 4) ? 8 : ch == 3 ? 4 : ch;
        printf("Number of Threads: %d\n", thread_count);
    } else {
        printf("Invalid Choice\nProgram terminated\n");
        return 0;
    }

    struct sockaddr_in address;
    int addrlen = sizeof(address);

    // Create UDP socket
    if ((server_file_des = socket(AF_INET, SOCK_DGRAM, 0)) == 0) {
        perror("Socket failed");
        exit(EXIT_FAILURE);
    }

    // Setup server address
    address.sin_family = AF_INET;
    address.sin_addr.s_addr = INADDR_ANY;
    address.sin_port = htons(5000);

    // Bind the socket
    if (bind(server_file_des, (struct sockaddr *)&address, sizeof(address)) < 0) {
        perror("Bind failed");
        exit(EXIT_FAILURE);
    }

    // Divide the workload among threads
    block_size = FILE_SIZE / thread_count;

    pthread_t threads[thread_count];
    for (long j = 0; j < thread_count; j++) {
        pthread_create(&threads[j], NULL, receive_data, (void*)j);  // Thread Creation
    }

    for (int k = 0; k < thread_count; k++) {
        pthread_join(threads[k], NULL);  // Wait for all threads to complete
    }

    printf("Total bytes transferred: %llu bytes\n", total_bytes);

    close(server_file_des);
    return 0;
}

