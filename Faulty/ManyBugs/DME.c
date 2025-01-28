#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <unistd.h>
#include <string.h>

int shared = 5;
pthread_mutex_t lock;

void sendreply(int replyfrompno, int replytopno, int ts) {
    FILE *ptr;
    char filename[10];
    sprintf(filename, "file%d.txt", replytopno);
    ptr = fopen(filename, "a");
    if (ptr == NULL) {
        perror("Error opening file");
        exit(1);
    }
    fprintf(ptr, "%d %d\n", replyfrompno, ts);
    fclose(ptr);
}

void *process1(void *val) {
    int replyfrom[4] = {0};
    int clockvalue = 0;
    int replyto[4] = {0};

    FILE *ptr1, *ptr2, *ptr3;

    printf("--------------process1---------------\n");
    ptr1 = fopen("file1.txt", "r");
    if (ptr1 == NULL) {
        perror("Error opening file1.txt");

        exit(1);
    }

    int pno = 0, ts = 0, timefromotherprocess = 0;
    while (fscanf(ptr1, "%d %d", &pno, &ts) != EOF) {
        if (ts > 0 && (pno == 2 || pno == 3)) {
            if (ts > timefromotherprocess)
                timefromotherprocess = ts;
        }
    }

    clockvalue = (clockvalue < timefromotherprocess) ? timefromotherprocess + 1 : clockvalue + 1;

    printf("file1 %d %d\n", pno, ts);
    fclose(ptr1);
    pthread_mutex_unlock(&lock);


    printf("--------------process1---------------\n");
    ptr2 = fopen("file2.txt", "a");
    ptr3 = fopen("file3.txt", "a");
    if (ptr2 == NULL || ptr3 == NULL) {
        perror("Error opening file2.txt or file3.txt");

        exit(1);
    }
    fprintf(ptr2, "%d %d\n", 1, clockvalue);
    fprintf(ptr3, "%d %d\n", 1, clockvalue);
    fclose(ptr2);
    fclose(ptr3);
    pthread_mutex_unlock(&lock);

    pthread_mutex_lock(&lock);
    printf("--------------process1---------------\n");
    ptr1 = fopen("file1.txt", "r");
    if (ptr1 == NULL) {
        perror("Error opening file1.txt");
        pthread_mutex_unlock(&lock);
        exit(1);
    }

    while (fscanf(ptr1, "%d %d", &pno, &ts) != EOF) {
        if (ts > 0 && (pno != 1)) {
            sendreply(1, pno, -ts);
            replyto[pno] = 1;
        }

        printf("1file1 %d %d\n", pno, ts);
    }
    fclose(ptr1);
    pthread_mutex_unlock(&lock);

    while (1) {
        pthread_mutex_lock(&lock);
        printf("--------------process1---------------\n");
        ptr1 = fopen("file1.txt", "r");
        if (ptr1 == NULL) {
            perror("Error opening file1.txt");
            pthread_mutex_unlock(&lock);
            exit(1);
        }

        while (fscanf(ptr1, "%d %d", &pno, &ts) != EOF) {
            if (ts < 0) {
                replyfrom[pno] = 1;
            }
            printf("1file1 %d %d\n", pno, ts);
        }
        if (replyfrom[2] == 1 && replyfrom[3] == 1) {
            fclose(ptr1);
            pthread_mutex_unlock(&lock);
            break;
        }
        fclose(ptr1);
        pthread_mutex_unlock(&lock);
        sleep(2);
    }

    pthread_mutex_lock(&lock);
    printf("--------------process1---------------\n");
    shared = shared + 10;
    printf("p1 did %d\n", shared);
    pthread_mutex_unlock(&lock);

    return NULL;
}

void *process2(void *val) {
    // Implementation similar to process1
    // Make sure to adjust file handling and synchronization similarly
    return NULL;
}

void *process3(void *val) {
    // Implementation similar to process1
    // Make sure to adjust file handling and synchronization similarly
    return NULL;
}

int main() {
    pthread_t pt[3];
    pthread_mutex_init(&lock, NULL);

    pthread_create(&pt[0], NULL, process1, NULL);
    pthread_create(&pt[1], NULL, process2, NULL);
    pthread_create(&pt[2], NULL, process3, NULL);

    pthread_join(pt[0], NULL);
    pthread_join(pt[1], NULL);
    pthread_join(pt[2], NULL);

    pthread_mutex_destroy(&lock);
    return 0;
}

