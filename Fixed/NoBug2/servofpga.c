#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/ioctl.h>
#include <pthread.h>
#include <linux/spi/spidev.h>
#include <string.h>
#include <errno.h>


#define SPI_MODE SPI_MODE_0
#define SPI_BITS_PER_WORD 8
#define SPI_SPEED_HZ 500000
#define SPI_DELAY 0
#define FPGA_SPI_DEV "/dev/spidev0.0"


#define CMD_SERVO 0x01
#define CMD_SPEED_ACC_SWITCH 0x02
#define CMD_AS 0x03
#define CMD_LED 0x04
#define CMD_SPEED 0x05
#define CMD_SPEEDPOLL 0x06
#define CMD_SPEEDRAW 0x07
#define SPI_PREAMBLE 0xAA


#define HIGHBYTE(x) ((x) >> 8)
#define LOWBYTE(x) ((x) & 0xFF)

int fd;
FILE *logfd = NULL;

static uint8_t spi_mode = SPI_MODE;
static uint8_t spi_bits = SPI_BITS_PER_WORD;
static uint32_t spi_speed = SPI_SPEED_HZ;
static uint16_t spi_delay = SPI_DELAY;

pthread_mutex_t spi_mutex;

int fpga_open() {
    int ret;

    pthread_mutex_init(&spi_mutex, NULL);

    printf("Will use SPI to send commands to FPGA\n");
    printf("SPI configuration:\n");
    printf("    + dev: %s\n", FPGA_SPI_DEV);
    printf("    + mode: %d\n", spi_mode);
    printf("    + bits per word: %d\n", spi_bits);
    printf("    + speed: %d Hz (%d KHz)\n\n", spi_speed, spi_speed / 1000);

    if ((fd = open(FPGA_SPI_DEV, O_RDWR)) < 0) {
        perror("E: fpga: spi: Failed to open dev");
        return -1;
    }

    if ((ret = ioctl(fd, SPI_IOC_WR_MODE, &spi_mode)) < 0) {
        perror("E: fpga: spi: can't set spi mode wr");
        return -1;
    }

    if ((ret = ioctl(fd, SPI_IOC_RD_MODE, &spi_mode)) < 0) {
        perror("E: fpga: spi: can't set spi mode rd");
        return -1;
    }

    if ((ret = ioctl(fd, SPI_IOC_WR_BITS_PER_WORD, &spi_bits)) < 0) {
        perror("E: fpga: spi: can't set bits per word wr");
        return -1;
    }

    if ((ret = ioctl(fd, SPI_IOC_RD_BITS_PER_WORD, &spi_bits)) < 0) {
        perror("E: fpga: spi: can't set bits per word rd");
        return -1;
    }

    if ((ret = ioctl(fd, SPI_IOC_WR_MAX_SPEED_HZ, &spi_speed)) < 0) {
        perror("E: fpga: spi: can't set speed wr");
        return -1;
    }

    if ((ret = ioctl(fd, SPI_IOC_RD_MAX_SPEED_HZ, &spi_speed)) < 0) {
        perror("E: fpga: spi: can't set speed rd");
        return -1;
    }

    if (fpga_logopen() < 0) {
        fprintf(stderr, "E: fpga: could not open log\n");
        return -1;
    }

    return 0;
}

void fpga_close() {
    if (fd >= 0) {
        close(fd);
    }
    if (logfd) {
        fclose(logfd);
    }
    pthread_mutex_destroy(&spi_mutex);
}

int fpga_logopen() {
    logfd = fopen("/tmp/ourlog", "a");
    if (!logfd) {
        perror("E: fpga: could not open log file");
        return -1;
    }
    fprintf(logfd, "--------reopened--------\n");
    return 0;
}

int spisend(unsigned char *rbuf, unsigned char *wbuf, int len) {
    int ret;

    pthread_mutex_lock(&spi_mutex);

    struct spi_ioc_transfer tr = {
        .tx_buf = (unsigned long)wbuf,
        .rx_buf = (unsigned long)rbuf,
        .len = len,
        .delay_usecs = spi_delay,
        .speed_hz = spi_speed,
        .bits_per_word = spi_bits,
    };

    ret = ioctl(fd, SPI_IOC_MESSAGE(1), &tr);
    if (ret < 1) {
        perror("E: fpga: can't send SPI message");
        pthread_mutex_unlock(&spi_mutex);
        return -1;
    }

    pthread_mutex_unlock(&spi_mutex);
    return ret;
}



void fpga_testservos() {
    if (fpga_open() < 0) {
        fprintf(stderr, "E: FPGA: Could not open SPI to FPGA\n");
        exit(EXIT_FAILURE);
    }

    printf("Moving servo left\n");
    fpga_setservo(1, 0);
    sleep(2);

    printf("Moving servo centre\n");
    fpga_setservo(1, 4000);
    sleep(2);

    printf("Moving servo right\n");
    fpga_setservo(1, 8000);
    sleep(2);

    printf("Moving servo centre\n");
    fpga_setservo(1, 4000);

    fpga_close();
}

