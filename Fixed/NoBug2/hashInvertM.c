#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <pthread.h>
#include <sys/socket.h>
#include <linux/netlink.h>
#include <errno.h>
 
extern int K;
extern int VECT_SIZE;
extern int aHash[10];
extern int bHash[10];
extern int m[10];

pthread_t ids[48];

int nextLoc = -1;
pthread_mutex_t m_nextLoc;

int totalOnes = 0;
pthread_mutex_t m_totalOnes;

int numMemberships = 0;
pthread_mutex_t m_numMemberships;

int *M;
int *checked;

pthread_mutex_t *m_M;

int nVertices;
struct bloom *a;

int ceilDiv(int a, int b);
void *countOnes(void *x);
void setM();
void resetM();
int checkLocation(int value, int *M);
int setLocation(int value, int *M);
int resetLocation(int value);
void reconstruct0(int loc);
void reconstruct1(int loc);
void *reconstruct0_thread(void *x);
void *reconstruct1_thread(void *x);

int main(int argc, char *argv[]){
 nVertices = atoi(argv[1]);
 int nNodes = atoi(argv[2]);
 K = atoi(argv[4]);
 VECT_SIZE = atoi(argv[5]);
 seiveInitial();

 struct timespec start, finish, mid;
 double elapsed, elapsed_m;

 a = (struct bloom*)malloc(sizeof(struct bloom));
 a->bloom_vector = (int*)malloc(sizeof(int)*(VECT_SIZE/NUM_BITS + 1));
 init(a);

 FILE *finput = fopen(argv[3],"r");
 int i = 0;
 int val;
 while (i<nNodes){
  val = 0;
  fscanf(finput,"%d\n",&val);
  insert(val,a);
  i++;
 }
 fclose(finput);


 int size = ceilDiv(nVertices,NUM_BITS);

 M = (int*)malloc(sizeof(int)*size);

 checked = (int*)malloc(sizeof(int)*size);
 for (i=0;i<size;i++) checked[i] = 0;

 pthread_mutex_init(&m_nextLoc,0);
 pthread_mutex_init(&m_totalOnes,0);
 pthread_mutex_init(&m_numMemberships,0);
 nextLoc = 0;
 int rc;
 clock_gettime(CLOCK_MONOTONIC, &start);
 for (i=0;i<48;i++){
  rc = pthread_create(&ids[i],0,countOnes,(void*)(i));
 }
 void *status;
 for (i=0;i<48;i++){
  pthread_join(ids[i],&status);
 }

 clock_gettime(CLOCK_MONOTONIC, &mid);

 nextLoc = 0;
 if (totalOnes < VECT_SIZE / 2){

  resetM();
  for (i=0;i<48;i++)
   rc = pthread_create(&ids[i],0,reconstruct1_thread,(void*)(i));
  for (i=0;i<48;i++)
   pthread_join(ids[i],&status);
 }
 else{

  setM();
  for (i=0;i<48;i++)
   rc = pthread_create(&ids[i],0,reconstruct0_thread,(void*)(i));
  for (i=0;i<48;i++)
   pthread_join(ids[i],&status);
 }
 clock_gettime(CLOCK_MONOTONIC, &finish);

 elapsed_m = mid.tv_sec - start.tv_sec;
 elapsed_m += (mid.tv_nsec - start.tv_nsec)/pow(10,9);
 elapsed = finish.tv_sec - start.tv_sec;
 elapsed += (finish.tv_nsec - start.tv_nsec)/pow(10,9);
 totalOnes = 0;
 for (i=0;i<size;i++) totalOnes += count_ones(M[i]);
 printf("RECONSTRUCTED SET OF SIZE %d IN TIME %lf WITH MEMBERSHIPS=%d\n",totalOnes, elapsed,numMemberships);


}


void *reconstruct0_thread(void *x){
 while(1){
  pthread_mutex_lock(&m_nextLoc);
  int nL = nextLoc;
  if (nL < ceilDiv (VECT_SIZE , NUM_BITS)){
   nextLoc++;
   pthread_mutex_unlock(&m_nextLoc);
   reconstruct0(nL);
  }
  else{
   pthread_mutex_unlock(&m_nextLoc);
   pthread_exit(0);
  }
 }
}

void *reconstruct1_thread(void *x){
 while(1){
  pthread_mutex_lock(&m_nextLoc);
  int nL = nextLoc;
  if (nL < ceilDiv (VECT_SIZE , NUM_BITS)){
   nextLoc++;
   pthread_mutex_unlock(&m_nextLoc);
   reconstruct1(nL);
  }
  else{
   pthread_mutex_unlock(&m_nextLoc);
   pthread_exit(0);
  }
 }
}

int ceilDiv(int a, int b){
 return (a+b-1)/b;
}

void *countOnes(void *x){
 while (1){
  pthread_mutex_lock(&m_nextLoc);
  int nL = nextLoc;
  if (nL < ceilDiv (VECT_SIZE , NUM_BITS)){
   nextLoc++;
   pthread_mutex_unlock(&m_nextLoc);

   int z = count_ones(a->bloom_vector[nL]);
   if (z>0){
    pthread_mutex_lock(&m_totalOnes);
    totalOnes += z;
    pthread_mutex_unlock(&m_totalOnes);
   }
  }
  else{
   pthread_mutex_unlock(&m_nextLoc);
   pthread_exit(0);
  }
 }
}

void setM(){
 int i;
 int size = ceilDiv(nVertices,NUM_BITS);
 for (i=0;i<size;i++) M[i] = M[i] | (~0);
}

void resetM(){
 int i;
 int size = ceilDiv(nVertices,NUM_BITS);
 for (i=0;i<size;i++) M[i] = 0;
}

int checkLocation(int value, int *M){
 int loc = value / NUM_BITS;
 int off = value % NUM_BITS;
 int retVal = 0;
 if ((M[loc] & (1<<off)) != 0) retVal = 1;
 return retVal;
}

int setLocation(int value, int *M){
 int loc = value / NUM_BITS;
 int off = value % NUM_BITS;
 M[loc] = M[loc] | (1 << off);
}

int resetLocation(int value){
 int loc = value / NUM_BITS;
 int off = value % NUM_BITS;
 M[loc] = M[loc] & (~(1 << off));
}

void reconstruct0(int loc){
 int i;
 for (i=loc*NUM_BITS;i<(loc+1)*NUM_BITS;i++){
  int v = a->bloom_vector[loc] & (1 << (i%NUM_BITS));
  if (v==0){
   int h = 0;
   for (h=0;h<3;h++){
    int sample = 0, count = 0;
    while (sample<nVertices){
                                 if (((i-bHash[h])%aHash[h] + (count*m[h])%aHash[h])%aHash[h] == 0){
                                         sample = ((int) (((double)i - bHash[h])/aHash[h] + ((double)count*m[h])/aHash[h]));
                                         if (sample>0){
       resetLocation(sample);
      }
                                 }
                                 sample = ((int) (((double)i - bHash[h])/aHash[h] + ((double)count*m[h])/aHash[h]));
                                 count++;
                         }
                        }
  }
 }
}


void reconstruct1(int loc){
 int i;
 int localCount = 0;
 for (i=loc*NUM_BITS;i<(loc+1)*NUM_BITS;i++){
  int v = a->bloom_vector[loc] & (1 << (i%NUM_BITS));

  if (v!=0){
   int h;
   for (h=0;h<3;h++){
    int sample = 0, count = 0;
    while (sample<nVertices){
     double cd = count, vd = m[h], ad = aHash[h], bd = bHash[h], s;
     s = (i- bd + cd*vd)/ad;
     sample = (int) s;
     if (floor(s)==s){
      if ((sample>0)&&(sample<nVertices)&&(!checkLocation(sample,checked))){
       setLocation(sample,checked);
       localCount++;
       if (is_in(sample,a)){
        setLocation(sample,M);
       }
      }
     }
     count++;
    }
   }
  }
 }
 pthread_mutex_lock(&m_numMemberships);
 numMemberships += localCount;
 pthread_mutex_unlock(&m_numMemberships);
}
