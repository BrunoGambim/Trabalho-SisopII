#include <string.h>
#include <stdio.h>
#include <stdlib.h> 
#include <unistd.h>
#include <pthread.h>
#include <semaphore.h>
#include <signal.h>
#include "members_table.h"
#include "network.h"
#include "interface_reader_subservice.h"
#include "exit_subservice.h"

#define BUFFER_SIZE 50
#define WAKEUP_CMD "WAKEUP"
#define EXIT_CMD "EXIT"
#define TRUE 1
#define RUNNING_AS_MEMBER 1
#define RUNNING_AS_MANAGER 2

pthread_mutex_t interfaceReaderSubserviceStateMutex;
pthread_cond_t interfaceReaderSubserviceStateChanged;
sem_t waitInterfaceReaderSubserviceStart;

pthread_t interfaceReaderMemberThread, interfaceReaderManagerThread, interfaceReaderSubserviceControllerThread;

custom_mutex *customMutex;

int interfaceReaderSubserviceState;

void* interfaceReaderMemberRoutine(){
    char cmd[BUFFER_SIZE];
    
    while (TRUE){
        if(feof(stdin)){
            finishProcess();
        }else{
            scanf("%s",cmd);
            if(strcmp(cmd,EXIT_CMD) == 0){
                finishProcess();
            }
        }
    }
}

void* interfaceReaderManagerRoutine(){
    char cmd[BUFFER_SIZE], hostname[BUFFER_SIZE], *macAddress;
    package* pack;
    char* broadcastIp;
    getBroadcastIPAddress(&broadcastIp);
    while (TRUE){
        pthread_setcancelstate(PTHREAD_CANCEL_ENABLE,NULL);
        scanf("%s %s",cmd, hostname);
        pthread_setcancelstate(PTHREAD_CANCEL_DISABLE,NULL);

        customReadMutexLock(customMutex);
        findMACAddressByHostname(hostname, &macAddress);
        customReadMutexUnlock(customMutex);

        if(macAddress != NULL && strcmp(cmd,WAKEUP_CMD) == 0){
            createMagicPackage(&pack, macAddress);
            sendPackage(pack,MAGIC_PACKAGE_PORT,broadcastIp);
            freePackage(pack);
            pack = NULL;
        }

        if(macAddress != NULL){
            free(macAddress);
            macAddress = NULL;
        }
    }
}

void* interfaceReaderSubserviceControllerRoutine(){
    pthread_mutex_lock(&interfaceReaderSubserviceStateMutex);
    sem_post(&waitInterfaceReaderSubserviceStart);
    while(TRUE){
        if(interfaceReaderSubserviceState == RUNNING_AS_MANAGER){
            pthread_cancel(interfaceReaderMemberThread);
            pthread_join(interfaceReaderMemberThread,NULL);
            pthread_create(&interfaceReaderManagerThread,NULL,interfaceReaderManagerRoutine,NULL);
        }else{
            pthread_cancel(interfaceReaderManagerThread);
            pthread_join(interfaceReaderManagerThread,NULL);
            pthread_create(&interfaceReaderMemberThread,NULL,interfaceReaderMemberRoutine,NULL);
        }

        pthread_cond_wait(&interfaceReaderSubserviceStateChanged,&interfaceReaderSubserviceStateMutex);
    }
}

void changeInterfaceReaderSubserviceToMember(){
    pthread_mutex_lock(&interfaceReaderSubserviceStateMutex);
    if(interfaceReaderSubserviceState == RUNNING_AS_MANAGER){
        interfaceReaderSubserviceState = RUNNING_AS_MEMBER;
        pthread_cond_signal(&interfaceReaderSubserviceStateChanged);
    }
    pthread_mutex_unlock(&interfaceReaderSubserviceStateMutex);
}

void changeInterfaceReaderSubserviceToManager(){
    pthread_mutex_lock(&interfaceReaderSubserviceStateMutex);
    if(interfaceReaderSubserviceState == RUNNING_AS_MEMBER){
        interfaceReaderSubserviceState = RUNNING_AS_MANAGER;
        pthread_cond_signal(&interfaceReaderSubserviceStateChanged);
    }
    pthread_mutex_unlock(&interfaceReaderSubserviceStateMutex);
}

void runInterfaceReaderSubservice(custom_mutex *mutex){
    interfaceReaderSubserviceState = RUNNING_AS_MEMBER;

    pthread_mutex_init(&interfaceReaderSubserviceStateMutex,NULL);
    pthread_cond_init(&interfaceReaderSubserviceStateChanged,NULL);
    sem_init(&waitInterfaceReaderSubserviceStart,0,0);
    customMutex = mutex;

    pthread_create(&interfaceReaderSubserviceControllerThread,NULL,interfaceReaderSubserviceControllerRoutine,NULL);

    sem_wait(&waitInterfaceReaderSubserviceStart);
}
