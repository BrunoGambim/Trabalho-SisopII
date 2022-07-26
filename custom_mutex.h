#include <pthread.h>

#define DATA_NOT_UPDATED 0
#define DATA_UPDATED 1

typedef struct _custom_mutex{
    int readCount;
    pthread_cond_t* printCond;
    pthread_mutex_t* printMutex;
    pthread_mutex_t mutex;
    pthread_mutex_t rwMutex;
} custom_mutex;

void createCustomMutex(custom_mutex** customMutex, pthread_cond_t* pCond, pthread_mutex_t* pMutex);
void customWriteMutexLock(custom_mutex* customMutex);
void customWriteMutexUnlock(custom_mutex* customMutex, int dataUpdated);
void customReadMutexLock(custom_mutex* customMutex);
void customReadMutexUnlock(custom_mutex* customMutex);