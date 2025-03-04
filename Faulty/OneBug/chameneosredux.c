#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <semaphore.h>
#include <sched.h>
#include <string.h>

typedef enum _color {
  ZERO = 0,
  BLUE = 1,
  RED = 2,
  YELLOW = 3,
  INVALID = 4,
} color;

char *colors[] = { "zero",
     "blue",
     "red",
     "yellow",
         "invalid"
};

char *digits[] = { "zero", "one", "two", "three", "four", "five",
     "six", "seven", "eight", "nine"
};

sem_t at_most_two;
sem_t mutex;
sem_t sem_priv;
sem_t sem_print;

pthread_mutex_t print_mutex;

int meetings_left = 0;
int first_arrived = 0;
int done = 0;

typedef struct _creature {
  color my_color;
  pthread_t id;
  int number_of_meetings;
} chameos;

chameos A;
chameos B;

static color
compliment_color(color c1, color c2) {
  color result;

  switch(c1) {
  case BLUE:
    switch(c2) {
    case BLUE:
      result = BLUE;
      break;
    case RED:
      result = YELLOW;
      break;
    case YELLOW:
      result = RED;
      break;
    default:
      printf("error complementing colors: %d, %d\n", c1, c2);
      exit(1);
    }
    break;
  case RED:
    switch(c2) {
    case BLUE:
      result = YELLOW;
      break;
    case RED:
      result = RED;
      break;
    case YELLOW:
      result = BLUE;
      break;
    default:
      printf("error complementing colors: %d, %d\n", c1, c2);
      exit(2);
    }
    break;
  case YELLOW:
    switch(c2) {
    case BLUE:
      result = RED;
      break;
    case RED:
      result = BLUE;
      break;
    case YELLOW:
      result = YELLOW;
      break;
    default:
      printf("error complementing colors: %d, %d\n", c1, c2);
      exit(3);
    }
    break;
  default:
    printf("error complementing colors: %d, %d\n", c1, c2);
    exit(4);
  }
  return result;
}

static void
spell_the_number(int prefix, int number) {
  char *string_number;
  int string_length;
  int i;
  int digit;
  int output_so_far = 0;
  char buff[1024];

  if(prefix != -1) {
    output_so_far = sprintf(buff, "%d", prefix);
  }

  string_number = malloc(sizeof(char)*10);
  string_length = sprintf(string_number, "%d", number);
  for(i = 0; i < string_length; i++) {
    digit = string_number[i] - '0';
    output_so_far += sprintf(buff+output_so_far, " %s", digits[digit]);
  }
  printf("%s\n",buff);

}

static chameos *
meeting(chameos c) {
  chameos *other_critter;
  other_critter = malloc(sizeof(chameos));

  sem_wait(&at_most_two);
  if(done == 1) {
    sem_post(&at_most_two);
    return 0;
  }

  sem_wait(&mutex);
  if(done == 1) {
    sem_post(&mutex);
    sem_post(&at_most_two);
    return 0;
  }

  if(first_arrived == 0) {
    first_arrived = 1;

    A.my_color = c.my_color;
    A.id = c.id;

    sem_post(&mutex);
    sem_wait(&sem_priv);

    other_critter->my_color = B.my_color;
    other_critter->id = B.id;

    meetings_left--;
    if(meetings_left == 0) {
      done = 1;
    }

    sem_post(&mutex);
    sem_post(&at_most_two); sem_post(&at_most_two);
  } else {
    first_arrived = 0;

    B.my_color = c.my_color;
    B.id = c.id;

    other_critter->my_color = A.my_color;
    other_critter->id = A.id;

    sem_post(&sem_priv);
  }

  return other_critter;
}

static void *
creature(void *arg) {
  chameos critter;
  critter.my_color = (color)arg;
  critter.id = pthread_self();
  critter.number_of_meetings = 0;

  chameos *other_critter;

  int met_others = 0;
  int met_self = 0;
  int *total_meetings = 0;

  while(done != 1) {
    other_critter = meeting(critter);

    if(other_critter == 0) {
      break;
    }

    if(critter.id == other_critter->id) {
      met_self++;
    }else{
      met_others++;
    }

    critter.my_color = compliment_color(critter.my_color, other_critter->my_color);
    free(other_critter);
  }

  sem_wait(&sem_print);
  
  spell_the_number(met_others + met_self, met_self);
  

  total_meetings = malloc(sizeof(int));
  *total_meetings =met_others + met_self;

  pthread_exit((void *)total_meetings);
}

void
print_colors(void) {
  int i, j;
  color c;

  for(i = 1; i < INVALID; i++) {
    for(j = 1; j < INVALID; j++) {
      c = compliment_color(i,j);
      printf("%s + %s -> %s\n",colors[i],colors[j], colors[c]);
    }
  }
  printf("\n");
}





void
run_the_meetings(color *starting_colors, int n_colors, int total_meetings_to_run) {
   struct sched_param priority;
   priority.sched_priority = 1;

   pthread_t pid_tab[10];
   memset(pid_tab, 0, sizeof(pthread_t)*10);

   int i;
   int total = 0;
   void *rslt = 0;

   sem_init(&at_most_two, 0, 2);
   sem_init(&mutex, 0, 1);
   sem_init(&sem_priv, 0, 0);
   sem_init(&sem_print, 0, 0);

   pthread_mutex_init(&print_mutex, 0);

   meetings_left = total_meetings_to_run;
   first_arrived = 0;
   done = 0;

   sched_setscheduler(0, SCHED_FIFO, &priority);

   for(i = 0; i < n_colors; i++) {
      printf(" %s", colors[starting_colors[i]]);
      pthread_create(&pid_tab[i], 0, &creature, (void *)starting_colors[i]);
   }
   printf("\n");
   for(i = 0; i < n_colors; i++) {
      sem_post(&sem_print);
   }

   for(i = 0; i < n_colors; i++) {
      pthread_join(pid_tab[i], &rslt);
      total += *(int *)rslt;
      free(rslt);
   }
   spell_the_number(-1, total);
   printf("\n");
}

int
main(int argc, char **argv) {
   color first_generation[3] = { BLUE, RED, YELLOW };
   color second_generation[10] = {BLUE, RED, YELLOW, RED, YELLOW,
                   BLUE, RED, YELLOW, RED, BLUE};
   int number_of_meetings_to_run = 600;

   if(argc > 1) {
      number_of_meetings_to_run = strtol(argv[1], 0, 10);
   }

   print_colors();
   run_the_meetings(first_generation, 3, number_of_meetings_to_run);
   run_the_meetings(second_generation, 10, number_of_meetings_to_run);

   return 0;
}
