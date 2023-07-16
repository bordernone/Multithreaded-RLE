#include <ctype.h>
#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include <pthread.h>
#include <stdbool.h>
#include <sys/stat.h>
#include <sys/mman.h>
#include <fcntl.h>
#include <semaphore.h>
#include <limits.h>
#include <math.h>
#include "macros.h"
#include "helper.h"
#include "thread_pool.h"

sem_t tasksAvailable;
sem_t *tasksCompleted;
pthread_mutex_t mutex;

int allDone = 0;

struct TaskV2 *TASK_QUEUE_V2;
struct ResultV2 *resultsV2;
unsigned long currentTaskIndex = 0;
unsigned long totalTasks;

int main(int argc, char *argv[]) {
    struct ProgramArguments args = parseArguments(argc, argv);

    if (!args.multithreaded) {
        return sequential(&args);
    } else {
        unsigned long *fileSizes = getFileSizes(args);
    
        totalTasks = getTasksCount(fileSizes, args.filesCount);
        
        TASK_QUEUE_V2 = malloc(sizeof(struct TaskV2) * totalTasks);
        resultsV2 = malloc(sizeof(struct ResultV2) * totalTasks);
        // Initialize the results
        for (unsigned long i = 0; i < totalTasks; i++) {
            resultsV2[i].size = 0;
        }
        // Initialize the semaphore
	    sem_init(&tasksAvailable, 0, 0);
        tasksCompleted = malloc(sizeof(sem_t) * totalTasks);
        for (unsigned long i = 0; i < totalTasks; i++) {
            sem_init(&tasksCompleted[i], 0, 0);
        }

        pthread_t *threads = malloc(sizeof(pthread_t) * args.threadCount);
        for (int i = 0; i < args.threadCount; i++) {
            pthread_create(&threads[i], NULL, solveSubproblem, NULL);
        }
        if (DEBUG)
            fprintf(stderr, "Total tasks: %lu \n", totalTasks);
        unsigned long thisTaskIndex = 0;
        for (int i = 0; i < args.filesCount; i++) {
            struct stat st;
            stat(args.files[i], &st);
            int fd = open(args.files[i], O_RDONLY);
            char *file = mmap(NULL, st.st_size, PROT_READ, MAP_PRIVATE, fd, 0);
            // Check if mmap failed
            if (file == MAP_FAILED) {
                if (DEBUG) {
                    fprintf(stderr, "Error: mmap failed \n");
                    exit(1);
                }
            } else {
                unsigned long thisFileTasks = ceil((double) st.st_size / CHUNK_SIZE);
                for (unsigned long j = 0; j < thisFileTasks; j++) {
                    unsigned long startInd = j * CHUNK_SIZE;
                    unsigned long numberOfCharacters = CHUNK_SIZE;
                    if (startInd + numberOfCharacters > (unsigned long) st.st_size) {
                        numberOfCharacters = st.st_size - startInd;
                    }
                    char *start = file + startInd;
                    struct TaskV2 task = {start, numberOfCharacters};
                    TASK_QUEUE_V2[thisTaskIndex] = task;
                    thisTaskIndex++;
                    sem_post(&tasksAvailable);
                }
            }

            close(fd);
        }

        printResultParallelV2();

        allDone = 1;

        for (int i = 0; i < args.threadCount; i++) {
            sem_post(&tasksAvailable);
        }

        for (int i = 0; i < args.threadCount; i++) {
            pthread_join(threads[i], NULL);
        }
    }

	return 0;
}
