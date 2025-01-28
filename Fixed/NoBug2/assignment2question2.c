#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <pthread.h>

struct node {
    char user[5];
    char process;
    int arrival;
    int duration;
    int priority;
    struct node *next;
}*head;

struct display {
    char user[5];
    int timeLastCalculated;
    struct display *next;
}*frontDisplay, *tempDisplay, *rearDisplay;

pthread_mutex_t m1 = PTHREAD_MUTEX_INITIALIZER;
pthread_mutex_t m2 = PTHREAD_MUTEX_INITIALIZER;

void initialise();
void insert(char[5], char, int, int, int);
void add(char[5], char, int, int, int);
int count();
void addafter(char[5], char, int, int, int, int);
void* jobDisplay();
void addToSummary(char[], int);
void summaryDisplay();

int main(int argc, char *argv[]) {
    int n;
    if (argc == 2) {
        n = atoi(argv[1]);
    } else if (argc > 2) {
        printf("Too many arguments supplied.\n");
        return 0;
    } else {
        printf("One argument expected.\n");
        return 0;
    }

    char user[5], process;
    int arrivalInput, durationInput, priorityInput;

    initialise();
    printf("\n\tUser\tProcess\tArrival\tRuntime\tPriority\n");

    int i = 1;
    while (i < 5) {
        printf("\t");
        if (scanf("%s %c %d %d %d", user, &process, &arrivalInput, &durationInput, &priorityInput) < 5) {
            printf("\nThe arguments supplied are incorrect. Try again.\n");
            return 0;
        }
        insert(user, process, arrivalInput, durationInput, priorityInput);
        i++;
    }

    printf("\nThis would result in:\n\tTime\tJob\n");

    pthread_t threadID[n];
    for (i = 0; i < n; i++) {
        if (pthread_create(&threadID[i], NULL, &jobDisplay, NULL) != 0) {
            perror("Error creating thread");
            return 1;
        }
    }

    for (i = 0; i < n; i++) {
        pthread_join(threadID[i], NULL);
    }

    pthread_mutex_destroy(&m1);
    pthread_mutex_destroy(&m2);
    summaryDisplay();

    return 0;
}

void initialise() {
    head = NULL;
    rearDisplay = NULL;
    tempDisplay = NULL;
    frontDisplay = NULL;
}

void insert(char user[5], char process, int arrival, int duration, int priority) {
    int c = 0;
    struct node *temp = head;

    if (temp == NULL) {
        add(user, process, arrival, duration, priority);
    } else {
        while (temp != NULL) {
            if ((temp->arrival + temp->duration) < (arrival + duration)) {
                c++;
            }
            temp = temp->next;
        }
        if (c == 0) {
            add(user, process, arrival, duration, priority);
        } else if (c < count()) {
            addafter(user, process, arrival, duration, priority, ++c);
        } else {
            struct node *right;
            temp = (struct node *)malloc(sizeof(struct node));
            if (temp == NULL) {
                perror("Memory allocation failed");
                exit(1);
            }

            strcpy(temp->user, user);
            temp->process = process;
            temp->arrival = arrival;
            temp->duration = duration;
            temp->priority = priority;

            right = head;
            while (right->next != NULL) {
                right = right->next;
            }

            right->next = temp;
            temp->next = NULL;
        }
    }
}

void add(char user[5], char process, int arrival, int duration, int priority) {
    struct node *temp = (struct node *)malloc(sizeof(struct node));
    if (temp == NULL) {
        perror("Memory allocation failed");
        exit(1);
    }

    strcpy(temp->user, user);
    temp->process = process;
    temp->arrival = arrival;
    temp->duration = duration;
    temp->priority = priority;

    if (head == NULL) {
        head = temp;
        head->next = NULL;
    } else {
        temp->next = head;
        head = temp;
    }
}

int count() {
    struct node *n = head;
    int c = 0;

    while (n != NULL) {
        n = n->next;
        c++;
    }
    return c;
}

void addafter(char user[5], char process, int arrival, int duration, int priority, int loc) {
    int i;
    struct node *temp, *left, *right;
    right = head;

    for (i = 1; i < loc; i++) {
        left = right;
        right = right->next;
    }

    temp = (struct node *)malloc(sizeof(struct node));
    if (temp == NULL) {
        perror("Memory allocation failed");
        exit(1);
    }

    strcpy(temp->user, user);
    temp->process = process;
    temp->arrival = arrival;
    temp->duration = duration;
    temp->priority = priority;

    left->next = temp;
    temp->next = right;
}

void* jobDisplay() {
    pthread_mutex_lock(&m1);
    struct node* placeholder = head;
    pthread_mutex_unlock(&m1);

    if (placeholder == NULL) {
        pthread_exit(NULL);
    }

    int myTime = 0;
    while (placeholder != NULL) {
        if (myTime < placeholder->arrival) {
            sleep(placeholder->arrival - myTime);
            myTime = placeholder->arrival;
        }

        pthread_mutex_lock(&m1);
        struct node* currentJob = placeholder;
        pthread_mutex_unlock(&m1);

        for (int i = 0; i < currentJob->duration; i++) {
            printf("\t%d\t%c\n", myTime, currentJob->process);
            myTime++;
            sleep(1);
        }

        addToSummary(currentJob->user, myTime);

        pthread_mutex_lock(&m1);
        placeholder = placeholder->next;
        pthread_mutex_unlock(&m1);
    }

    printf("\t%d\tIDLE\n", myTime);
    pthread_exit(NULL);
}

void addToSummary(char name[], int timeLeft) {
    pthread_mutex_lock(&m2);

    if (rearDisplay == NULL) {
        rearDisplay = (struct display *)malloc(sizeof(struct display));
        if (rearDisplay == NULL) {
            perror("Memory allocation failed");
            pthread_mutex_unlock(&m2);
            exit(1);
        }

        rearDisplay->next = NULL;
        strcpy(rearDisplay->user, name);
        rearDisplay->timeLastCalculated = timeLeft;
        frontDisplay = rearDisplay;
    } else {
        tempDisplay = frontDisplay;
        while (tempDisplay != NULL) {
            if (strcmp(tempDisplay->user, name) == 0) {
                tempDisplay->timeLastCalculated = timeLeft;
                pthread_mutex_unlock(&m2);
                return;
            }
            tempDisplay = tempDisplay->next;
        }

        tempDisplay = (struct display *)malloc(sizeof(struct display));
        if (tempDisplay == NULL) {
            perror("Memory allocation failed");
            pthread_mutex_unlock(&m2);
            exit(1);
        }

        rearDisplay->next = tempDisplay;
        strcpy(tempDisplay->user, name);
        tempDisplay->timeLastCalculated = timeLeft;
        tempDisplay->next = NULL;
        rearDisplay = tempDisplay;
    }

    pthread_mutex_unlock(&m2);
}

void summaryDisplay() {
    printf("\n\tSummary\n");

    while (frontDisplay != NULL) {
        printf("\t%s\t%d\n", frontDisplay->user, frontDisplay->timeLastCalculated);
        tempDisplay = frontDisplay->next;
        free(frontDisplay);
        frontDisplay = tempDisplay;
    }
    printf("\n");
}

