#include <string.h>
#include <stdio.h>
#include <stdlib.h> 
#include <unistd.h>
#include <pthread.h>
#include <semaphore.h>
#include <signal.h>
#include "members_table.h"
#include "network.h"
#include "exit_subservice.h"

#define TRUE 1
#define RUNNING_AS_MEMBER 1
#define RUNNING_AS_MANAGER 2

pthread_mutex_t exitSubserviceStateMutex;
pthread_cond_t exitSubserviceStateChanged;
sem_t waitExitSubserviceStart;

pthread_t exitReceiverThread, exitSubserviceControllerThread;

custom_mutex *customMutex;
int exitSocket;
void *(*exitFunc)();

int exitSubserviceState;

void sendExitPackageToTheManager(){
    char* hostname;
    char* managerIpAddress;
    package* pack;
    getHostname(&hostname);
    createHostnamePackage(&pack, hostname);

    customReadMutexLock(customMutex);
    if(hasManager()){
        getManagerIPAddress(&managerIpAddress);
        sendPackage(pack,EXIT_PORT,managerIpAddress);
        free(managerIpAddress);
        managerIpAddress = NULL;
    }
    customReadMutexUnlock(customMutex);

    freePackage(pack);
    pack = NULL;
}

void* exitReceiverManagerRoutine(){
    package* pack;
    char* hostname;
    while(TRUE){
        pthread_setcancelstate(PTHREAD_CANCEL_ENABLE,NULL);
        serve(&pack,NULL,exitSocket);
        pthread_testcancel();
        pthread_setcancelstate(PTHREAD_CANCEL_DISABLE,NULL);

        unpackHostnamePackage(pack,&hostname);
        
        customWriteMutexLock(customMutex);
        removeLineByHostname(hostname);
        customWriteMutexUnlock(customMutex, DATA_UPDATED);
        
        free(hostname);
        hostname = NULL;
    }
}

void finishProcess(){
    if(exitSubserviceState == RUNNING_AS_MEMBER){
        sendExitPackageToTheManager();
    }
    (*exitFunc)();
    exit(EXIT_SUCCESS);
}

void* exitSubserviceControllerRoutine(){
    pthread_mutex_lock(&exitSubserviceStateMutex);
    sem_post(&waitExitSubserviceStart);
    while(TRUE){
        if(exitSubserviceState == RUNNING_AS_MANAGER){
            pthread_create(&exitReceiverThread,NULL,exitReceiverManagerRoutine,NULL);
        }else{
            pthread_cancel(exitReceiverThread);
            pthread_join(exitReceiverThread,NULL);
        }

        pthread_cond_wait(&exitSubserviceStateChanged,&exitSubserviceStateMutex);
    }
}

void changeExitSubserviceToMember(){
    pthread_mutex_lock(&exitSubserviceStateMutex);
    if(exitSubserviceState == RUNNING_AS_MANAGER){
        exitSubserviceState = RUNNING_AS_MEMBER;
        pthread_cond_signal(&exitSubserviceStateChanged);
    }
    pthread_mutex_unlock(&exitSubserviceStateMutex);
}

void changeExitSubserviceToManager(){
    pthread_mutex_lock(&exitSubserviceStateMutex);
    if(exitSubserviceState == RUNNING_AS_MEMBER){
        exitSubserviceState = RUNNING_AS_MANAGER;
        pthread_cond_signal(&exitSubserviceStateChanged);
    }
    pthread_mutex_unlock(&exitSubserviceStateMutex);
}

void stopExitSubservice(){
    pthread_cancel(exitReceiverThread);
    pthread_cancel(exitSubserviceControllerThread);
    closeSocket(exitSocket);
}

void runExitSubservice(custom_mutex *mutex, void *(*func)()){
    char *ipAddress;
    getIPAddress(&ipAddress);

    exitSocket = createSocket(EXIT_PORT,ipAddress);
    exitSubserviceState = RUNNING_AS_MEMBER;
    exitFunc = func;

    pthread_mutex_init(&exitSubserviceStateMutex,NULL);
    pthread_cond_init(&exitSubserviceStateChanged,NULL);
    sem_init(&waitExitSubserviceStart,0,0);
    customMutex = mutex;

    signal(SIGINT, finishProcess);

    pthread_create(&exitSubserviceControllerThread,NULL,exitSubserviceControllerRoutine,NULL);

    sem_wait(&waitExitSubserviceStart);
}