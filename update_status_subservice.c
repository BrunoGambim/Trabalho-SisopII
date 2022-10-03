#include <string.h>
#include <stdio.h>
#include <stdlib.h> 
#include <unistd.h>
#include <pthread.h>
#include <semaphore.h>
#include <signal.h>
#include "members_table.h"
#include "network.h"
#include "update_status_subservice.h"

#define UPDATE_SENDER_PERIOD 1
#define UPDATE_STATUS_PERIOD 1
#define TRUE 1
#define RUNNING_AS_MEMBER 1
#define RUNNING_AS_MANAGER 2

pthread_mutex_t updateStatusSubserviceStateMutex;
pthread_cond_t updateStatusSubserviceStateChanged;
sem_t waitUpdateStatusSubserviceStart;

pthread_t updateStatusSenderThread, updateStatusThread, updateStatusReceiverThread, updateStatusSubserviceControllerThread;


custom_mutex *customMutex;
int updateStatusSocket;

int updateStatusSubserviceState;

void* updateStatusSenderMemberRoutine(){
    char* hostname;
    char* managerIpAddress;
    package* pack;
    getHostname(&hostname);
    while(TRUE){

        createHostnamePackage(&pack, hostname);

        customReadMutexLock(customMutex);
        if(hasManager()){
            getManagerIPAddress(&managerIpAddress);
            sendPackage(pack,UPDATE_PORT,managerIpAddress);
            free(managerIpAddress);
            managerIpAddress = NULL;
        }
        customReadMutexUnlock(customMutex);

        freePackage(pack);
        
        pthread_setcancelstate(PTHREAD_CANCEL_ENABLE,NULL);
        sleep(UPDATE_SENDER_PERIOD);
        pthread_setcancelstate(PTHREAD_CANCEL_DISABLE,NULL);
    }
}

void* updateStatusReceiverManagerRoutine(){
    package* pack;
    char* hostname;
    while(TRUE){
        pthread_setcancelstate(PTHREAD_CANCEL_ENABLE,NULL);
        serve(&pack,NULL,updateStatusSocket);
        pthread_testcancel();
        pthread_setcancelstate(PTHREAD_CANCEL_DISABLE,NULL);
        unpackHostnamePackage(pack,&hostname);

        customWriteMutexLock(customMutex);
            
        updateTimestamp(hostname);

        customWriteMutexUnlock(customMutex, DATA_NOT_UPDATED);
        free(hostname);
    }
}

void* updateStatusManagerRoutine(void* aux){
    int dataUpdated;
    while(TRUE){
        customWriteMutexLock(customMutex);
        dataUpdated = updateMembersStatus();

        customWriteMutexUnlock(customMutex, dataUpdated);

        pthread_setcancelstate(PTHREAD_CANCEL_ENABLE,NULL);
        sleep(UPDATE_STATUS_PERIOD);
        pthread_setcancelstate(PTHREAD_CANCEL_DISABLE,NULL);
    }
}

void* updateStatusSubserviceControllerRoutine(){
    pthread_mutex_lock(&updateStatusSubserviceStateMutex);
    sem_post(&waitUpdateStatusSubserviceStart);
    while(TRUE){
        if(updateStatusSubserviceState == RUNNING_AS_MANAGER){
            pthread_cancel(updateStatusSenderThread);
            pthread_join(updateStatusSenderThread,NULL);
            pthread_create(&updateStatusReceiverThread,NULL,updateStatusReceiverManagerRoutine,NULL);
            pthread_create(&updateStatusThread,NULL,updateStatusManagerRoutine,NULL);
        }else{
            pthread_cancel(updateStatusReceiverThread);
            pthread_cancel(updateStatusThread);
            pthread_join(updateStatusReceiverThread,NULL);
            pthread_join(updateStatusThread,NULL);
            pthread_create(&updateStatusSenderThread,NULL,updateStatusSenderMemberRoutine,NULL);
        }

        pthread_cond_wait(&updateStatusSubserviceStateChanged,&updateStatusSubserviceStateMutex);
    }
}

void changeUpdateStatusSubserviceToMember(){
    pthread_mutex_lock(&updateStatusSubserviceStateMutex);
    if(updateStatusSubserviceState == RUNNING_AS_MANAGER){
        updateStatusSubserviceState = RUNNING_AS_MEMBER;
        pthread_cond_signal(&updateStatusSubserviceStateChanged);
    }
    pthread_mutex_unlock(&updateStatusSubserviceStateMutex);
}

void changeUpdateStatusSubserviceToManager(){
    pthread_mutex_lock(&updateStatusSubserviceStateMutex);
    if(updateStatusSubserviceState == RUNNING_AS_MEMBER){
        updateStatusSubserviceState = RUNNING_AS_MANAGER;
        pthread_cond_signal(&updateStatusSubserviceStateChanged);
    }
    pthread_mutex_unlock(&updateStatusSubserviceStateMutex);
}

void stopUpdateStatusSubservice(){
    closeSocket(updateStatusSocket);
}

void runUpdateStatusSubservice(custom_mutex *mutex){
    char *ipAddress;
    getIPAddress(&ipAddress);

    updateStatusSocket = createSocket(UPDATE_PORT,ipAddress);
    
    updateStatusSubserviceState = RUNNING_AS_MEMBER;

    pthread_mutex_init(&updateStatusSubserviceStateMutex,NULL);
    pthread_cond_init(&updateStatusSubserviceStateChanged,NULL);
    sem_init(&waitUpdateStatusSubserviceStart,0,0);
    customMutex = mutex;

    pthread_create(&updateStatusSubserviceControllerThread,NULL,updateStatusSubserviceControllerRoutine,NULL);

    sem_wait(&waitUpdateStatusSubserviceStart);
}
