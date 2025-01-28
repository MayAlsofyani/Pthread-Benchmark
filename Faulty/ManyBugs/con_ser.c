#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <pthread.h>
#include <unistd.h>
#include <stdint.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <fcntl.h>

static int fd;
static int disconnect;

static char *ip;
static char *port;
static uint32_t cuid;

static pthread_t hthid;
static pthread_t rthid;

static pthread_mutex_t fdlock;

typedef struct _threc {
 pthread_t thid;
 pthread_mutex_t mutex;
 pthread_cond_t cond;
 uint8_t *buff;
 uint32_t buffsize;
 uint8_t sent;
 uint8_t status;
 uint8_t release;
 uint32_t size;
 uint32_t cmd;
 uint32_t packetid;
 struct _threc *next;
} threc;

static threc *threc_head = 0;

void fs_buffer_init(threc *rec,uint32_t size) {
 if (size > 10000) {
  rec->buff = realloc(rec->buff,size);
  rec->buffsize = size;
 } else if (rec->buffsize > 10000) {
  rec->buff = realloc(rec->buff,10000);
  rec->buffsize = 10000;
 } else {
  fprintf(stderr, "null\n");
 }
}

threc* fs_get_threc_by_id(uint32_t packetid) {
 threc *rec;
 for (rec = threc_head ; rec ; rec=rec->next) {
  if (rec->packetid==packetid) {
   return rec;
  }
 }
 return 0;
}

void get_chunkid(uint8_t *data)
{
 uint64_t chunkid = 0;
 int8_t status = 0;
 uint8_t *ptr, hdr[16];

if (DEBUG) {
 fprintf(stderr, "----get create chunk info ---\n");
}
 GET64BIT(chunkid, data);

 status = create_file(chunkid);

 if (status != 0) {
  fprintf(stderr, "create chunk file failed\n");
 }


 if (disconnect == 0 && fd >= 0) {
 printf("--------------\n");
  ptr = hdr;
  PUT32BIT(DATTOSER_CREAT_CHUNK, ptr);
  PUT32BIT(8, ptr);
  PUT32BIT(0, ptr);
  PUT32BIT(status, ptr);

  if (tcptowrite(fd, hdr, 16, 1000) != 16) {
   disconnect = 1;
  }
 }

}

void analyze_ser_packet(uint32_t type, uint32_t size)
{
 uint8_t *data = 0;

 data = malloc(size);
 if (tcptoread(fd, data, size, 1000) != (int32_t)(size)) {
  fprintf(stderr,"ser: tcp recv error(3)\n");
  disconnect=1;
  free(data);
  return;
 }
 switch (type) {
  case ANTOAN_NOP:
   break;
  case SERTODAT_DISK_INFO:
   break;
  case SERTODAT_CREAT_CHUNK:
   get_chunkid(data);
   break;
  default:
   break;

 }
 free(data);
}

int8_t analyze_ser_cmd(uint32_t type)
{
 switch (type) {
  case ANTOAN_NOP:
  case SERTODAT_DISK_INFO:
  case SERTODAT_CREAT_CHUNK:
   return 0;
  default:
   break;
 }
 return 1;
}


void connect_ser()
{

 fd = tcpsocket();

 if (tcpconnect(fd, ip, port) < 0) {
  fprintf(stderr, "can't connect to ser(ip:%s,port:%s)\n", ip, port);
  tcpclose(fd);
  fd = -1;
  return;
 } else {
  fprintf(stderr, "connect to ser(ip:%s,port:%s)\n", ip, port);
  disconnect = 0;
 }

}

void *heartbeat_thread(void *arg)
{
 uint8_t *ptr, hdr[28];
 uint64_t totalspace;
 uint64_t freespace;

 totalspace = 201;
 freespace = 102;
 while (1) {

  if (disconnect == 0 && fd >= 0) {
   ptr = hdr;
   PUT32BIT(DATTOSER_DISK_INFO, ptr);
   PUT32BIT(20, ptr);
   PUT32BIT(0, ptr);
   PUT64BIT(totalspace, ptr);
   PUT64BIT(freespace, ptr);

  printf("heart beat\n");
   if (tcptowrite(fd, hdr, 28, 1000) != 28) {
    disconnect = 1;
   }
  }

  sleep(5);
 }
}

void *receive_thread(void *arg)
{
 uint8_t *ptr = 0;
 uint8_t hdr[12] = {0};
 uint32_t cmd = 0;
 uint32_t size = 0;
 uint32_t packetid = 0;
 int r = 0;
 threc *rec = 0;

 for (;;) {
  pthread_mutex_lock(&fdlock);
  if (fd == -1 && disconnect) {
   connect_ser();
  }

  if (disconnect) {
   tcpclose(fd);
   fd = -1;

   for (rec = threc_head; rec; rec=rec->next) {

   }
  }

  if (fd == -1) {
   fprintf(stderr, "reconnect ser(ip:%s,port:%s)\n", ip, port);
   sleep(2);

   continue;
  }


  r = read(fd,hdr,12);

  if (r==0) {
   fprintf(stderr, "ser: connection lost (1)\n");
   disconnect=1;
   continue;
  }
  if (r!=12) {
   fprintf(stderr,"ser: tcp recv error(1), %d\n", r);
   disconnect=1;
   continue;
  }

  ptr = hdr;
  GET32BIT(cmd,ptr);
  GET32BIT(size,ptr);
  GET32BIT(packetid,ptr);

 fprintf(stderr, "read, cmd:%u\n", cmd);
 fprintf(stderr, "read, size:%u\n", size);
 fprintf(stderr, "read, packetid:%u\n", packetid);

  if (cmd==ANTOAN_NOP && size==4) {

   continue;
  }
  if (size < 4) {
   fprintf(stderr,"ser: packet too small\n");
   disconnect=1;
   continue;
  }
  size -= 4;


  if (analyze_ser_cmd(cmd) == 0) {
   analyze_ser_packet(cmd, size);
   continue;
  }

  rec = fs_get_threc_by_id(packetid);
  if (rec == 0) {
   fprintf(stderr, "ser: get unexpected queryid\n");
   disconnect=1;
   continue;
  }
  fs_buffer_init(rec,rec->size+size);
  if (rec->buff == 0) {
   disconnect=1;
   continue;
  }

  if (size>0) {
   r = tcptoread(fd,rec->buff+rec->size,size,1000);

int i;
fprintf(stderr, "read buf:%s, size:%d\n", rec->buff, size);
for(i = 0; i< size; i++) {
 fprintf(stderr, "%u-", rec->buff[rec->size+i]);
}
fprintf(stderr, "\n");


   if (r == 0) {
    fprintf(stderr,"ser: connection lost (2)\n");
    disconnect=1;
    continue;
   }
   if (r != (int32_t)(size)) {
    fprintf(stderr,"ser: tcp recv error(2)\n");
    disconnect=1;
    continue;
   }
  }
  rec->sent=0;
  rec->status=0;
  rec->size = size;
  rec->cmd = cmd;

  rec->release = 1;

    

  pthread_cond_signal(&(rec->cond));
 }
}

void ser_init(char *_ip, char *_port)
{

 ip = strdup(_ip);
 port = strdup(_port);
 cuid = 0;
 fd = -1;
 disconnect = 1;

 pthread_mutex_init(&fdlock,0);
 pthread_create(&rthid, 0, receive_thread, 0);


}
