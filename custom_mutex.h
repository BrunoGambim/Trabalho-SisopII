#include <pthread.h>

#ifndef CUSTOM_MUTEX_H
#define CUSTOM_MUTEX_H

#define DATA_NOT_UPDATED 0
#define DATA_UPDATED 1

typedef struct _custom_mutex{
    int readCount;
    pthread_cond_t printCond;
    pthread_cond_t rplCond;
    pthread_mutex_t printMutex;
    pthread_mutex_t replicationMutex;
    pthread_mutex_t mutex;
    pthread_mutex_t rwMutex;
} custom_mutex;

void createCustomMutex(custom_mutex** customMutex);
void customWriteMutexLock(custom_mutex* customMutex);
void customWriteMutexUnlock(custom_mutex* customMutex, int dataUpdated);
void customReadMutexLock(custom_mutex* customMutex);
void customReadMutexUnlock(custom_mutex* customMutex);
void customReplicationMutexLock(custom_mutex* customMutex);
void customReplicationMutexUnlock(custom_mutex* customMutex);
void customPrintMutexLock(custom_mutex* customMutex);
void customPrintMutexUnlock(custom_mutex* customMutex);
void customPrintWait(custom_mutex* customMutex);
void customReplicationWait(custom_mutex* customMutex);

#endif
