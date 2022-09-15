#include <string.h>
#include <stdio.h>
#include <stdlib.h> 
#include <unistd.h>
#include <pthread.h>
#include <semaphore.h>
#include <signal.h>
#include "custom_mutex.h"

pthread_mutex_t printMutex;
pthread_cond_t dataUpdatedCond;
sem_t waitPrintRoutineStart;
custom_mutex* customMutex;
pthread_t printThread, writeThread, writeThread2, readThread, readThread2, readThread3;

void* printRoutine(){
    pthread_mutex_lock(&printMutex);
    printf("começando printRoutine\n");
    sem_post(&waitPrintRoutineStart);
    while(1){
        pthread_cond_wait(&dataUpdatedCond,&printMutex);
        printf("começando print\n");
        sleep(2);
        printf("terminando print\n");
    }
}

void* readRoutine(){
    customReadMutexLock(customMutex);
    printf("começando readRoutine1\n");
    sleep(2);
    printf("terminando readRoutine1\n");
    customReadMutexUnlock(customMutex);
}

void* readRoutine2(){
    customReadMutexLock(customMutex);
    printf("começando readRoutine2\n");
    sleep(2);
    printf("terminado readRoutine2\n");
    customReadMutexUnlock(customMutex);
}

void* readRoutine3(){
    customReadMutexLock(customMutex);
    printf("começando readRoutine3\n");
    sleep(2);
    printf("terminado readRoutine3\n");
    customReadMutexUnlock(customMutex);
}

void* writeRoutine(){
    customWriteMutexLock(customMutex);
    printf("começando writeRoutine1\n");
    sleep(2);
    printf("terminado writeRoutine1\n");
    customWriteMutexUnlock(customMutex, DATA_UPDATED);
}

void* writeRoutine2(){
    customWriteMutexLock(customMutex);
    printf("começando writeRoutine2\n");
    sleep(2);
    printf("terminado writeRoutine2\n");
    customWriteMutexUnlock(customMutex, DATA_NOT_UPDATED);
}


int main(){

    sem_init(&waitPrintRoutineStart,0,0);
    pthread_mutex_init(&printMutex,NULL);
    pthread_cond_init(&dataUpdatedCond,NULL);
    createCustomMutex(&customMutex, &dataUpdatedCond, &printMutex);

    pthread_create(&printThread,NULL,printRoutine,NULL);
    sem_wait(&waitPrintRoutineStart);
    printf("começando outras threads\n");
    sleep(1);
    pthread_create(&writeThread,NULL,writeRoutine,NULL);
    pthread_create(&writeThread2,NULL,writeRoutine2,NULL);
    sleep(1);
    pthread_create(&readThread,NULL,readRoutine,NULL);
    pthread_create(&readThread2,NULL,readRoutine2,NULL);
    pthread_create(&readThread3,NULL,readRoutine3,NULL);

    pthread_join(printThread, NULL);
    pthread_join(writeThread, NULL);
    pthread_join(writeThread2, NULL);
    pthread_join(readThread, NULL);
    pthread_join(readThread2, NULL);
    pthread_join(readThread3, NULL);

    return 0;
}
