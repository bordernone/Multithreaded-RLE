#ifndef NYUENC_HELPER_H
#define NYUENC_HELPER_H
#include "macros.h"
#include <stdint.h>

struct ProgramArguments {
    int multithreaded;
    int threadCount;
    char **files;
    int filesCount;
};

struct TaskV2 {
    char *startChar;
    unsigned short numberOfCharacters;
};
struct ResultV2 {
    char *characters;
    uint8_t *counts;
    unsigned long size;
};

unsigned long MIN(unsigned long a, unsigned long b);
struct ProgramArguments parseArguments(int argc, char *argv[]);
void writeTuple(char c, uint8_t count);
int sequential(struct ProgramArguments *args);
unsigned long *getFileSizes(struct ProgramArguments args);
unsigned long getTasksCount(unsigned long *fileSizes, int filesCount);
struct ResultV2 *performRLEV2(struct TaskV2 task);
void printResultParallelV2();

#endif 