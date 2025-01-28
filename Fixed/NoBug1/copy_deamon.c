#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <pthread.h>
#include <fcntl.h>
#include <time.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <errno.h>

char sourceFilename[500], destinationFilename[500];
char nonFlatSourceFilename[500], nonFlatDestinationFilename[500];

off_t offset = 0;
pthread_mutex_t offsetMutex = PTHREAD_MUTEX_INITIALIZER;
int copyingDone = 0;

// Function prototypes
void *copyWorker(void *arg);
void copyNonFlatFile();
void runDestinationVM();
void *sendOffset(void *recvConnection);
void connectionHandler(int connection);
void *runServer(void *arg);
void getVMsInfo(int debugMode);
void sendMessage(int connection, const char *message);
void createServer(void (*handler)(int));

void *copyWorker(void *arg) {
    char buf[4096];
    time_t startedAt = time(NULL);

    int sourceFd = open(sourceFilename, O_RDONLY);
    if (sourceFd < 0) {
        perror("Error opening source file");
        pthread_exit(NULL);
    }

    int destinationFd = open(destinationFilename, O_WRONLY | O_CREAT, 0666);
    if (destinationFd < 0) {
        perror("Error opening destination file");
        close(sourceFd);
        pthread_exit(NULL);
    }

    int res;
    do {
        pthread_mutex_lock(&offsetMutex);

        res = pread(sourceFd, buf, sizeof(buf), offset);
        if (res <= 0) {
            pthread_mutex_unlock(&offsetMutex);
            break;
        }

        if (pwrite(destinationFd, buf, res, offset) < 0) {
            perror("Error writing to destination file");
            pthread_mutex_unlock(&offsetMutex);
            close(sourceFd);
            close(destinationFd);
            pthread_exit(NULL);
        }

        offset += res;

        pthread_mutex_unlock(&offsetMutex);
    } while (res == sizeof(buf));

    close(sourceFd);
    close(destinationFd);

    time_t endedAt = time(NULL);
    printf("Copying Done! in %ld seconds\n", (endedAt - startedAt));

    copyingDone = 1;
    pthread_exit(NULL);
}

void copyNonFlatFile() {
    char buf[4096];
    off_t nonFlatOffset = 0;

    time_t startedAt = time(NULL);

    int nonFlatSourceFd = open(nonFlatSourceFilename, O_RDONLY);
    if (nonFlatSourceFd < 0) {
        perror("Error opening non-flat source file");
        return;
    }

    int nonFlatDestinationFd = open(nonFlatDestinationFilename, O_WRONLY | O_CREAT, 0666);
    if (nonFlatDestinationFd < 0) {
        perror("Error opening non-flat destination file");
        close(nonFlatSourceFd);
        return;
    }

    int res;
    do {
        res = pread(nonFlatSourceFd, buf, sizeof(buf), nonFlatOffset);
        if (res <= 0) {
            break;
        }

        if (pwrite(nonFlatDestinationFd, buf, res, nonFlatOffset) < 0) {
            perror("Error writing to non-flat destination file");
            break;
        }

        nonFlatOffset += res;
    } while (res == sizeof(buf));

    close(nonFlatSourceFd);
    close(nonFlatDestinationFd);

    time_t endedAt = time(NULL);
    printf("Non-Flat Copying Done! in %ld seconds\n", (endedAt - startedAt));
}

void runDestinationVM() {
    char storageCommandBuffer[500], destinationChangeUUIDCommand[500];

    time_t startedAt = time(NULL);

    sprintf(destinationChangeUUIDCommand, "VBoxManage internalcommands sethduuid \"%s\"", nonFlatDestinationFilename);
    sprintf(storageCommandBuffer, "VBoxManage storageattach Destination --medium \"%s\" --storagectl \"IDE\" --port 0 --device 0 --type hdd", nonFlatDestinationFilename);

    system(destinationChangeUUIDCommand);
    system(storageCommandBuffer);

    time_t endedAt = time(NULL);
    printf("Running Destination in %ld seconds\n", (endedAt - startedAt));
}

void *sendOffset(void *recvConnection) {
    int connection = (intptr_t)recvConnection;
    char offsetBuffer[50];
    char clientMessage[2000];

    do {
        memset(offsetBuffer, 0, sizeof(offsetBuffer));
        memset(clientMessage, 0, sizeof(clientMessage));

        recv(connection, clientMessage, sizeof(clientMessage), 0);

        if (!copyingDone) {
            pthread_mutex_lock(&offsetMutex);

            sprintf(offsetBuffer, "%zu", offset);
            sendMessage(connection, offsetBuffer);

            recv(connection, clientMessage, sizeof(clientMessage), 0);
            pthread_mutex_unlock(&offsetMutex);
        } else {
            if (strcmp(clientMessage, "SUSPENDING") != 0) {
                strcpy(offsetBuffer, "DONE");
                sendMessage(connection, offsetBuffer);
            }
        }
    } while (strcmp(clientMessage, "CLOSE") != 0);

    close(connection);
    printf("\tConnection Closed\n");

    copyNonFlatFile();
    runDestinationVM();

    pthread_exit(NULL);
}

void connectionHandler(int connection) {
    pthread_t thread;
    pthread_create(&thread, NULL, sendOffset, (void *)(intptr_t)connection);
}

void *runServer(void *arg) {
    createServer(connectionHandler);
    pthread_exit(NULL);
}

void getVMsInfo(int debugMode) {
    if (debugMode) {
        strcpy(sourceFilename, "/path/to/source-flat.vmdk");
        strcpy(nonFlatSourceFilename, "/path/to/source.vmdk");
        strcpy(destinationFilename, "/path/to/destination-flat.vmdk");
        strcpy(nonFlatDestinationFilename, "/path/to/destination.vmdk");
    } else {
        printf("Source File Path: ");
        fgets(sourceFilename, sizeof(sourceFilename), stdin);
        printf("Source File Path (non-flat): ");
        fgets(nonFlatSourceFilename, sizeof(nonFlatSourceFilename), stdin);
        printf("Destination File Path (Will be created automatically): ");
        fgets(destinationFilename, sizeof(destinationFilename), stdin);
        printf("Destination File Path (non-flat) (Will be created automatically): ");
        fgets(nonFlatDestinationFilename, sizeof(nonFlatDestinationFilename), stdin);

        sourceFilename[strcspn(sourceFilename, "\n")] = 0;
        nonFlatSourceFilename[strcspn(nonFlatSourceFilename, "\n")] = 0;
        destinationFilename[strcspn(destinationFilename, "\n")] = 0;
        nonFlatDestinationFilename[strcspn(nonFlatDestinationFilename, "\n")] = 0;
    }
}

int main() {
    pthread_t copyWorkerThread, serverThread;

    getVMsInfo(1);

    pthread_create(&copyWorkerThread, NULL, copyWorker, NULL);
    pthread_create(&serverThread, NULL, runServer, NULL);

    pthread_join(copyWorkerThread, NULL);
    pthread_join(serverThread, NULL);

    printf("Done!\n");
    return 0;
}

