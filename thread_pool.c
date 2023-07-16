#include "thread_pool.h"
#include "helper.h"
#include <pthread.h>
#include <semaphore.h>
#include <stdlib.h>
#include <stdio.h>

extern sem_t tasksAvailable;
extern pthread_mutex_t mutex;
extern sem_t *tasksCompleted;

extern int allDone;

extern struct ResultV2 *resultsV2;
extern unsigned long currentTaskIndex;
extern struct TaskV2 *TASK_QUEUE_V2;
extern unsigned long totalTasks;

void *solveSubproblem() {
    while (1 == 1) {
        // Grab a task
        sem_wait(&tasksAvailable);
        if (allDone) {
            pthread_exit(NULL);
        }
        // grab mutex
        pthread_mutex_lock(&mutex);
        int myTaskIndex = currentTaskIndex;
        currentTaskIndex++;
        pthread_mutex_unlock(&mutex);
        struct TaskV2 currentTask = TASK_QUEUE_V2[myTaskIndex];
        struct ResultV2 *result = performRLEV2(currentTask);
        resultsV2[myTaskIndex] = *result;
        sem_post(&tasksCompleted[myTaskIndex]);
    }
}
