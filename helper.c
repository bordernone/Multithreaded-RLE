#include "helper.h"
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
#include <math.h>

extern unsigned long totalTasks;
extern sem_t *tasksCompleted;
extern struct ResultV2 *resultsV2;

unsigned long MIN(unsigned long a, unsigned long b) {
    if (a < b) {
        return a;
    }
    return b;
}

struct ProgramArguments parseArguments(int argc, char *argv[]) {
    // Parse command line arguments
    int option;
    char *optstring = "j:";
    int multithreaded = 0;
    int threadCount = 0;
    while ((option = getopt(argc, argv, optstring)) != -1) {
        switch (option) {
            case 'j':
                multithreaded = 1;
                threadCount = atoi(optarg);
                break;
            default:
                printf("Usage: %s [-j n] file1.txt file2.txt file3.txt \n", argv[0]);
                exit(1);
        }
    }

    char **files = argv + optind;
    int filesCount = argc - optind;
    // If no files are specified, return error
    if (filesCount == 0) {
        printf("Usage: %s [-j n] file1.txt file2.txt file3.txt \n", argv[0]);
        exit(1);
    }

    struct ProgramArguments args;
    args.multithreaded = multithreaded;
    args.threadCount = threadCount;
    args.files = files;
    args.filesCount = filesCount;

    if (DEBUG) {
        // Print all files
        for (int i = 0; i < args.filesCount; i++) {
            fprintf(stderr, "%s	", args.files[i]);
        }
        fprintf(stderr, "\n");
        fprintf(stderr, "Multithreaded: %d, Thread Count: %d Files Count: %d \n", args.multithreaded, args.threadCount, filesCount);
    }


    return args;
}

void writeTuple(char c, uint8_t count) {
	fwrite(&c, sizeof(char), 1, stdout);
	fwrite(&count, sizeof(uint8_t), 1, stdout);
}


int sequential(struct ProgramArguments *args) {
    int filesCount = args->filesCount;
    int fp;
    char **files = args->files;
	uint8_t count = 0;
	char lastChar = 0;
	bool first = true;
	for (int i = 0; i < filesCount; i++) {
		if ((fp = open(files[i], O_RDONLY)) < 0) {
			printf("Error: Could not open file %s \n", files[i]);
			return 1;
		}

		struct stat st;
		stat(files[i], &st);
		if (fstat(fp, &st) < 0) {
			printf("Error: Could not stat file %s \n", files[i]);
			return 1;
		}

		if (DEBUG) {
			printf("File size: %ld \n", st.st_size);
		}
		
		int size = st.st_size;

		// Load the file into memory
		char *fileContents = mmap(NULL, size, PROT_READ, MAP_PRIVATE, fp, 0);

		// Read the file
		for (int j = 0; j < size; j++) {
			if (lastChar == fileContents[j]) {
				count++;
			} else {
				if (!first) {
					writeTuple(lastChar, count);
				}
				count = 1;
			}
			first = false;
			lastChar = fileContents[j];
		}

		// Close the file
		munmap(fileContents, size);
		close(fp);
	}
	writeTuple(lastChar, count);
    return 0;
}

unsigned long *getFileSizes(struct ProgramArguments args) {
    unsigned long *sizes = malloc(sizeof(unsigned long) * args.filesCount);
    for (int i = 0; i < args.filesCount; i++) {
        struct stat st;
        stat(args.files[i], &st);
        sizes[i] = st.st_size;
    }
    return sizes;
}

unsigned long getTasksCount(unsigned long *fileSizes, int filesCount) {
    unsigned long count = 0;
    for (int i = 0; i < filesCount; i++) {
        if (fileSizes[i] > 0) {
            count += ceil((double) fileSizes[i] / CHUNK_SIZE);
        }
    }
    return count;
}

struct ResultV2 * performRLEV2(struct TaskV2 task) {
    struct ResultV2 *result = malloc(sizeof(struct ResultV2));
    result->characters = malloc(sizeof(char) * task.numberOfCharacters);
    result->counts = malloc(sizeof(unsigned long) * task.numberOfCharacters);
    unsigned long size = 0;
    unsigned long lastCharCount = 1;
    char lastChar = task.startChar[0];
    for (int i = 1; i < task.numberOfCharacters; i++) {
        if (lastChar != task.startChar[i]) {
            result->characters[size] = lastChar;
            result->counts[size++] = lastCharCount;
            lastChar = task.startChar[i];
            lastCharCount = 1;
        } else {
            lastCharCount++;
        }
    }
    result->characters[size] = lastChar;
    result->counts[size++] = lastCharCount;
    result->size = size;
    return result;
}

void printResultParallelV2() {
    char lastChar = 0;
    uint8_t lastCount = 0;
    for (unsigned long currentTaskIndex = 0; currentTaskIndex < totalTasks; currentTaskIndex++) {
        if (currentTaskIndex >= 2) {
            // free the previous result
            free(resultsV2[currentTaskIndex - 2].characters);
            free(resultsV2[currentTaskIndex - 2].counts);
        }
        sem_wait(&tasksCompleted[currentTaskIndex]);
        struct ResultV2 result = resultsV2[currentTaskIndex];
        for (long unsigned int i = 0; i < result.size; i++) {
            if (lastChar == result.characters[i]) {
                lastCount += result.counts[i];
            } else {
                if (lastCount != 0) {
                    writeTuple(lastChar, lastCount);
                }
                lastChar = result.characters[i];
                lastCount = result.counts[i];
            }
        }
    }
    if (lastCount != 0) {
        writeTuple(lastChar, lastCount);
    }
    // writeTuple(lastChar, lastCount);
}