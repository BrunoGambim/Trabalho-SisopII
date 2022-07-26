#include <stdlib.h>
#include "custom_mutex.h"

void createCustomMutex(custom_mutex** customMutex, pthread_cond_t* pCond, pthread_mutex_t* pMutex){
    *customMutex = (custom_mutex*) malloc(sizeof(custom_mutex));
    (*customMutex)->printCond = pCond;
    (*customMutex)->printMutex = pMutex;
    (*customMutex)->readCount = 0;
    pthread_mutex_init(&((*customMutex)->mutex),NULL);
    pthread_mutex_init(&((*customMutex)->rwMutex),NULL);
}

void customWriteMutexLock(custom_mutex* customMutex){
    pthread_mutex_lock(customMutex->printMutex);
    pthread_mutex_lock(&(customMutex->rwMutex));
}

void customWriteMutexUnlock(custom_mutex* customMutex, int dataUpdated){
    if(dataUpdated){
        pthread_cond_signal(customMutex->printCond);
    }
    pthread_mutex_unlock(&(customMutex->rwMutex));
    pthread_mutex_unlock(customMutex->printMutex);
}

void customReadMutexLock(custom_mutex* customMutex){
    pthread_mutex_lock(&(customMutex->mutex));
    customMutex->readCount++;
    if(customMutex->readCount == 1)
        pthread_mutex_lock(&(customMutex->rwMutex));
    pthread_mutex_unlock(&(customMutex->mutex));
}

void customReadMutexUnlock(custom_mutex* customMutex){
    pthread_mutex_lock(&(customMutex->mutex));
    customMutex->readCount--;
    if(customMutex->readCount == 0)
        pthread_mutex_unlock(&(customMutex->rwMutex));
    pthread_mutex_unlock(&(customMutex->mutex));
}