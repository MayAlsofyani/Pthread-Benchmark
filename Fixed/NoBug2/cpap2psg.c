#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <pthread.h>
#include <fcntl.h>
#include <string.h>
#include <time.h>
#include <sys/time.h>
#include <errno.h>

#define MAX_CHANNEL 6

static pthread_t threadTickGenerator;
static pthread_t threadFIFO2DA[MAX_CHANNEL];
static pthread_mutex_t mutexTick[MAX_CHANNEL];
static pthread_cond_t condTick[MAX_CHANNEL];

int debug = 0;

int readAChar(int fifoFD) {
    char aChar;
    int err = read(fifoFD, &aChar, 1);
    if (err < 0) {
        perror("read");
        return -1;
    } else if (err == 0) {
        sleep(1);
        return -2;
    }
    return (int)aChar;
}

void waitTick(int Channel) {
    pthread_mutex_lock(&mutexTick[Channel]);
    pthread_cond_wait(&condTick[Channel], &mutexTick[Channel]);
    pthread_mutex_unlock(&mutexTick[Channel]);
}

void *functionTickGenerator(void *param) {
    int Channel;
    for (Channel = 0; Channel < MAX_CHANNEL; Channel++) {
        pthread_mutex_init(&mutexTick[Channel], NULL);
        pthread_cond_init(&condTick[Channel], NULL);
    }

    while (1) {
        struct timespec sleepTime = {0, 40 * 1000000};
        
        for (Channel = 0; Channel < MAX_CHANNEL; Channel++) {
            pthread_mutex_lock(&mutexTick[Channel]);
            pthread_cond_signal(&condTick[Channel]);
            pthread_mutex_unlock(&mutexTick[Channel]);
        }

        nanosleep(&sleepTime, NULL);
    }

    return NULL;
}

void *functionFIFO2DA(void *param) {
    int channel = (intptr_t)param;
    char stringFIFO[32];
    int fifoFD;

    pthread_detach(pthread_self());
    snprintf(stringFIFO, sizeof(stringFIFO), "/tmp/FIFO_CHANNEL%d", channel);
    fifoFD = open(stringFIFO, O_RDONLY);
    if (fifoFD < 0) {
        perror("open");
        if (mkfifo(stringFIFO, 0777) != 0) {
            perror("mkfifo");
            goto threadError;
        }
        fifoFD = open(stringFIFO, O_RDONLY);
        if (fifoFD < 0) {
            perror("open after mkfifo");
            goto threadError;
        }
    }

    while (1) {
        char readBuffer[512 * 1024];
        char *walkInBuffer = readBuffer;
        int readSize;
        int stringIndex;
        int readInt;
        char *toNullPos;

        readSize = read(fifoFD, readBuffer, sizeof(readBuffer));
        if (readSize <= 0) {
            struct timespec sleepTime = {0, 40 * 1000000};
            nanosleep(&sleepTime, NULL);
            continue;
        }

        if (debug) printf("ch%d:", channel);
        do {
            toNullPos = strchr(walkInBuffer, ',');
            if (toNullPos) *toNullPos = '\0';

            stringIndex = sscanf(walkInBuffer, "0x%x", &readInt);
            if (stringIndex == 0) {
                stringIndex = sscanf(walkInBuffer, "%d", &readInt);
                if (stringIndex == 0) {
                    fprintf(stderr, "Invalid number: %s\n", walkInBuffer);
                    if (toNullPos) {
                        walkInBuffer = toNullPos + 1;
                        continue;
                    } else {
                        break;
                    }
                }
            }

            writeDAC(channel, readInt);
            waitTick(channel);

            if (toNullPos)
                walkInBuffer = toNullPos + 1;
            else
                break;
        } while (strlen(walkInBuffer) > 0);

        if (debug) printf("\n");
    }

threadError:
    printf("Channel %d exit\n", channel);
    if (fifoFD >= 0) close(fifoFD);
    return NULL;
}

static const int maxValue[MAX_CHANNEL] = {
    120,
    240,
    60,
    30,
    30,
    30
};

int cpap2psg(int rs232_descriptor, char *cmdBuffer, int cmdSize, int checkedXor) {
    int expectedLength = 5;

    if (sendCPAPCmd(rs232_descriptor, cmdBuffer, cmdSize, checkedXor))
        return -1;

    unsigned char responseBuffer[1024];
    int responseSize;
    responseSize = recvCPAPResponse(rs232_descriptor, responseBuffer, sizeof(responseBuffer), expectedLength);

    if (responseSize < 0) {
        return responseSize;
    }

    int adjustedValue = (65535.0 / maxValue[0]) * responseBuffer[2];
    if (debug) printf("cpap:%d -> da:%d\n", responseBuffer[2], adjustedValue);

    writeDAC(0, adjustedValue);
    return 0;
}

int main(int argc, char **argv) {
    if (access("/etc/debug", F_OK) == 0)
        debug = 1;

    int rs232_descriptor;
    char cmdBuffer[2] = {0x93, 0xCB};
    int cmdSize = sizeof(cmdBuffer);
    int checkedXor;

    initDAC();

    int deviceDesc = openCPAPDevice();
    checkedXor = getCheckedXor(cmdBuffer, cmdSize);

    pthread_create(&threadTickGenerator, NULL, functionTickGenerator, NULL);

    while (1) {
        int err = cpap2psg(deviceDesc, cmdBuffer, cmdSize, checkedXor);
        if (err == -2) {
            deviceDesc = openCPAPDevice();
        }
        sleep(1);

        for (int channel = 0; channel < MAX_CHANNEL; channel++) {
            if (threadFIFO2DA[channel] == 0) {
                pthread_create(&threadFIFO2DA[channel], NULL, functionFIFO2DA, (void *)(intptr_t)channel);
            }
        }
    }

    rs232_close(rs232_descriptor);

    return 0;
}

