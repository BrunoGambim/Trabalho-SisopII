#include <stdlib.h>
#include <stdio.h>
#include "custom_mutex.h"

void createCustomMutex(custom_mutex** customMutex){
    *customMutex = (custom_mutex*) malloc(sizeof(custom_mutex));
    pthread_mutex_init(&(*customMutex)->replicationMutex,NULL);
    pthread_mutex_init(&(*customMutex)->printMutex,NULL);
    pthread_cond_init(&(*customMutex)->printCond,NULL); 
    pthread_cond_init(&(*customMutex)->rplCond,NULL);
    (*customMutex)->readCount = 0;
    pthread_mutex_init(&((*customMutex)->mutex),NULL);
    pthread_mutex_init(&((*customMutex)->rwMutex),NULL);
}

void customWriteMutexLock(custom_mutex* customMutex){
    pthread_mutex_lock(&(customMutex->printMutex));
    pthread_mutex_lock(&(customMutex->replicationMutex));
    pthread_mutex_lock(&(customMutex->rwMutex));
}

void customWriteMutexUnlock(custom_mutex* customMutex, int dataUpdated){
    if(dataUpdated){
        pthread_cond_signal(&(customMutex->rplCond));
        pthread_cond_signal(&(customMutex->printCond));
    }
    pthread_mutex_unlock(&(customMutex->rwMutex));
    pthread_mutex_unlock(&(customMutex->replicationMutex));
    pthread_mutex_unlock(&(customMutex->printMutex));
}

void customReadMutexLock(custom_mutex* customMutex){
    pthread_mutex_lock(&(customMutex->mutex));
    customMutex->readCount++;
    if(customMutex->readCount == 1){
        pthread_mutex_lock(&(customMutex->rwMutex));
    }
    pthread_mutex_unlock(&(customMutex->mutex));
}

void customReadMutexUnlock(custom_mutex* customMutex){
    pthread_mutex_lock(&(customMutex->mutex));
    customMutex->readCount--;
    if(customMutex->readCount == 0){
        pthread_mutex_unlock(&(customMutex->rwMutex));
    }
    pthread_mutex_unlock(&(customMutex->mutex));
}

void customReplicationMutexLock(custom_mutex* customMutex){
    pthread_mutex_lock(&(customMutex->replicationMutex));
}

void customReplicationMutexUnlock(custom_mutex* customMutex){
    pthread_mutex_unlock(&(customMutex->replicationMutex));
}

void customPrintMutexLock(custom_mutex* customMutex){
    pthread_mutex_lock(&(customMutex->printMutex));
}

void customPrintMutexUnlock(custom_mutex* customMutex){
    pthread_mutex_unlock(&(customMutex->printMutex));
}

void customPrintWait(custom_mutex* customMutex){
    pthread_cond_wait(&(customMutex->printCond),&(customMutex->printMutex));
}

void customReplicationWait(custom_mutex* customMutex){
    pthread_cond_wait(&(customMutex->rplCond),&(customMutex->replicationMutex));
}
