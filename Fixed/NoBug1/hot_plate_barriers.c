#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <pthread.h>
#include <sys/time.h>
#include <math.h>

#define GRID_SIZE 16384

float** main_plate;
float** main_prev_plate;
char** main_locked_cells;

pthread_barrier_t barrier_first;
pthread_barrier_t barrier_second;
pthread_mutex_t critical_begin_end;
pthread_mutex_t runnable;

typedef struct arg_plate {
 int nthreads;
 int begin;
 int end;
} arg_plate_t;

double when() {
 struct timeval tp;
 gettimeofday(&tp, 0);
 return ((double)tp.tv_sec + (double)tp.tv_usec * 1e-6);
}


float** createPlate() {
 float** plate = (float**)malloc(16384 * sizeof(float*));
 int k;
 for (k = 0; k < 16384; k++) {
  plate[k] = (float*)malloc(16384 * sizeof(float));
 }
 return plate;
}
char** createCharPlate() {
 char** plate = (char**)malloc(16384 * sizeof(char*));
 int k;
 for (k = 0; k < 16384; k++) {
  plate[k] = (char*)malloc(16384 * sizeof(char));
 }
 return plate;
}
void copy(float** main, float** result) {
 int i,j;
 for (i = 0; i < 16384; i++) {
  for (j = 0; j < 16384; j++) {

   result[i][j] = main[i][j];
  }
 }
}

void initPlate(float** plate, float** prev_plate) {
 int i, j;
 for (i = 0; i < 16384; i++) {
  for (j = 0; j < 16384; j++) {
   if (i == 0 || j == 0 || j == 16384 -1) {
    plate[i][j] = 0;
    prev_plate[i][j] = 0;
    main_locked_cells[i][j] = '1';
   }
   else if (i == 16384 -1) {
    plate[i][j] = 100;
    prev_plate[i][j] = 100;
    main_locked_cells[i][j] = '1';

   }
   else if (i == 400 && j >= 0 && j <= 330) {
    plate[i][j] = 100;
    prev_plate[i][j] = 100;
    main_locked_cells[i][j] = '1';
   }
   else if (i == 200 && j == 500) {
    plate[i][j] = 100;
    prev_plate[i][j] = 100;
    main_locked_cells[i][j] = '1';
   }
   else {
    plate[i][j] = 50;
    prev_plate[i][j] = 50;
    main_locked_cells[i][j] = '0';
   }
  }
 }
 for (i = 0; i < 16384; i++) {
  if ((i % 20) == 0) {
   for (j = 0; j < 16384; j++) {
    plate[i][j] = 100;
    prev_plate[i][j] = 100;
    main_locked_cells[i][j] = '1';
   }
  }
 }
 for (j = 0; j < 16384; j++) {
  if ((j % 20) == 0) {
   for (i = 0; i < 16384; i++) {
    plate[i][j] = 0;
    prev_plate[i][j] = 0;
    main_locked_cells[i][j] = '1';
   }
  }
 }
}

void cleanupFloat(float** plate) {
 int i, j;
 for (i = 0; i < 16384; i++) {
  free(plate[i]);
 }
 free(plate);
}
void cleanupChar(char** plate) {
 int i, j;
 for (i = 0; i < 16384; i++) {
  free(plate[i]);
 }
 free(plate);
}

void* update_plate(void* plate_arguments) {
 for(;;) {
  pthread_barrier_wait(&barrier_first);

  arg_plate_t* plate_args = (arg_plate_t*)plate_arguments;

  pthread_mutex_lock(&runnable);
  int begin = plate_args->begin;
  int end = plate_args->end;
  pthread_mutex_unlock(&runnable);


  int i, j;
  for (i = begin; i < end; i++) {
   for (j = 0; j < 16384; j++) {
    if (main_locked_cells[i][j] == '0') {
     main_plate[i][j] = (main_prev_plate[i+1][j] + main_prev_plate[i][j+1] + main_prev_plate[i-1][j]
       + main_prev_plate[i][j-1] + 4 * main_prev_plate[i][j]) * 0.125;
    }
   }
  }

  pthread_barrier_wait(&barrier_second);
 }
}
char steady(float** current_plate) {
 int count = 0;
 int i, j;
 float main_diff = 0;
 for (i = 0; i < 16384; i++) {
  for (j = 0; j < 16384; j++) {
   if (main_locked_cells[i][j] == '0') {
    if (current_plate[i][j] > 50)
     count++;

    float diff = fabs(current_plate[i][j] - (current_plate[i+1][j] + current_plate[i-1][j]
       + current_plate[i][j+1] + current_plate[i][j-1]) * 0.25);
    if (diff > main_diff)
     main_diff = diff;
   }
  }
 }

 if (main_diff > 0.1)
  return (1);
 else
  return (0);
}

void allocateWorkload(int nthreads, int* begin_end) {
 int step = 16384 / nthreads;
 int i;
 int begin = 0;
 for (i = 0; i < nthreads*2; i++) {
  begin_end[i] = begin;

  begin = begin+step;
  i += 1;

  begin_end[i] = begin;
 }
}

int startUpdate(pthread_t* threads, int nthreads) {
 printf("Updating plate to steady state\n");

 int iterations = 0;
 int* begin_end = (int*)malloc((nthreads*2) * sizeof(int));
 allocateWorkload(nthreads, begin_end);

 int i;
 int j;
 pthread_t worker[nthreads];
 arg_plate_t* plate_args;
 for (i = 0; i < nthreads; i++) {
  pthread_mutex_lock(&critical_begin_end);

  plate_args = (arg_plate_t*)malloc(sizeof(arg_plate_t));
  j = i * 2;
  plate_args->begin = begin_end[j];
  plate_args->end = begin_end[j+1];

  pthread_create(&worker[i], 0, &update_plate, (void*)plate_args);


  pthread_mutex_unlock(&critical_begin_end);
 }
 do {
  iterations++;
  printf("Iteration: %d\n", iterations);

  pthread_barrier_wait(&barrier_first);
  pthread_barrier_wait(&barrier_second);

  copy(main_plate, main_prev_plate);

 } while(steady(main_plate));
 return iterations;
}


int main(int argc, char* argv[]) {

 double start = when();
 printf("Starting time: %f\n", start);


 int nthreads = atoi(argv[1]);
 pthread_t threads[nthreads];


 main_plate = createPlate();
 main_prev_plate = createPlate();
 main_locked_cells = createCharPlate();


 initPlate(main_plate, main_prev_plate);


 pthread_barrier_init(&barrier_first,0,nthreads+1);
 pthread_barrier_init(&barrier_second,0,nthreads+1);
 pthread_mutex_init(&critical_begin_end,0);
 pthread_mutex_init(&runnable,0);


 int iterations = startUpdate(threads, nthreads);


 double end = when();
 printf("\nEnding time: %f\n", end);
 printf("Total execution time: %f\n", end - start);
 printf("Number of iterations: %d\n\n", iterations);


 pthread_barrier_destroy(&barrier_first);
 pthread_barrier_destroy(&barrier_second);
 pthread_mutex_destroy(&critical_begin_end);
 pthread_mutex_destroy(&runnable);


 printf("Cleanup\n");
 cleanupFloat(main_plate);
 cleanupFloat(main_prev_plate);
 cleanupChar(main_locked_cells);

 return 0;
}
