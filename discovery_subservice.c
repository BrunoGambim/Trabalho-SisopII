#include <string.h>
#include <stdio.h>
#include <stdlib.h> 
#include <unistd.h>
#include <pthread.h>
#include <semaphore.h>
#include <signal.h>
#include "members_table.h"
#include "network.h"
#include "discovery_subservice.h"
#include "election_subservice.h"

#define DISCOVERY_SENDER_PERIOD 2
#define UPDATE_MANAGER_STATUS_PERIOD 5
#define TRUE 1
#define RUNNING_AS_MEMBER 1
#define RUNNING_AS_MANAGER 2
#define WAITING_FOR_MESSAGE 1
#define MESSAGE_RECEIVED 2

pthread_mutex_t discoverySubserviceStateMutex, managerStatusMutex;
pthread_cond_t discoverySubserviceStateChanged;
sem_t waitDiscoverySubserviceStart;

pthread_t discoverySenderThread, updateManagerStatusThread, discoveryReceiverResponseThread, discoveryReceiverRequestThread, discoverySubserviceControllerThread;

custom_mutex *customMutex;
int discoverySocket;
int discoveryBroadcastSocket;

int discoverySubserviceState;

void *(*runAsMemberDiscovery)(); 

int managerState;

void* updateManagerStatusRoutine(){
    while(TRUE){
        pthread_setcancelstate(PTHREAD_CANCEL_ENABLE,NULL);
        sleep(UPDATE_MANAGER_STATUS_PERIOD);
        pthread_setcancelstate(PTHREAD_CANCEL_DISABLE,NULL);

        if(managerState == WAITING_FOR_MESSAGE && discoverySubserviceState == RUNNING_AS_MEMBER){
            startElection();
        }else {
            managerState = WAITING_FOR_MESSAGE;
        }
    }
}

void sendDiscoveryPackage(char* managerIp){
    package* pack;
    char* hostname;
    char* macAddress;
    getHostname(&hostname);
    getMACAddress(&macAddress);
    createDiscoveryPackage(&pack, hostname, macAddress);
    sendPackage(pack,DISCOVERY_PORT,managerIp);
    freePackage(pack);
}

void* discoverySenderManagerRoutine(){
    package* pack;
    char* hostname;
    char* macAddress;
    char* broadcastIp;
    getBroadcastIPAddress(&broadcastIp);
    getHostname(&hostname);
    getMACAddress(&macAddress);

    while(TRUE){
        createDiscoveryPackage(&pack, hostname, macAddress);
        sendPackage(pack,DISCOVERY_BROADCAST_PORT,broadcastIp);
        freePackage(pack);

        pthread_setcancelstate(PTHREAD_CANCEL_ENABLE,NULL);
        sleep(DISCOVERY_SENDER_PERIOD);
        pthread_setcancelstate(PTHREAD_CANCEL_DISABLE,NULL);
    }
}

void* discoveryReceiverResponseRoutine(){
    package* pack;
    int isHostInTheTable;
    char* hostname;
    char* macAddress;
    char* ipAddress;
    while(TRUE){
        pthread_setcancelstate(PTHREAD_CANCEL_ENABLE,NULL);
        serve(&pack,&ipAddress,discoverySocket);
        pthread_testcancel();
        pthread_setcancelstate(PTHREAD_CANCEL_DISABLE,NULL);
        
        unpackDiscoveryPackage(pack,&hostname,&macAddress);

        customReadMutexLock(customMutex);
        isHostInTheTable = isHostnameInTheTable(hostname);
        customReadMutexUnlock(customMutex);
        
        if(!isHostInTheTable){
            customWriteMutexLock(customMutex);
            addLine(hostname,macAddress,ipAddress,AWAKEN);
            customWriteMutexUnlock(customMutex, DATA_UPDATED);
        }

        free(hostname);
        free(macAddress);
        free(ipAddress);
    }
}

void* discoveryReceiverRequestRoutine(){
    package* pack;
    int managerIsNull;
    char* managerHostname;
    char* hostname;
    char* macAddress;
    char* ipAddress;
    while(TRUE){
        pthread_setcancelstate(PTHREAD_CANCEL_ENABLE,NULL);
        serve(&pack,&ipAddress,discoveryBroadcastSocket);
        pthread_testcancel();
        pthread_setcancelstate(PTHREAD_CANCEL_DISABLE,NULL);

        unpackDiscoveryPackage(pack,&hostname,&macAddress);
        managerState = MESSAGE_RECEIVED;

        customReadMutexLock(customMutex);
        managerIsNull = !hasManager();
        customReadMutexUnlock(customMutex);
        
        if(managerIsNull){
            customWriteMutexLock(customMutex);
            if(!isHostnameInTheTable(hostname)){
                addLine(hostname,macAddress,ipAddress,AWAKEN);
            }
            setManagerByHostname(hostname);
            customWriteMutexUnlock(customMutex,DATA_UPDATED);
            sendDiscoveryPackage(ipAddress);
        } else {
            customReadMutexLock(customMutex);
            getManagerHostname(&managerHostname);
            customReadMutexUnlock(customMutex);

            if(strcmp(hostname, managerHostname) > 0){
                customWriteMutexLock(customMutex);
                if(!isHostnameInTheTable(hostname)){
                    addLine(hostname,macAddress,ipAddress,AWAKEN);
                }
                setManagerByHostname(hostname);
                sendDiscoveryPackage(ipAddress);
                if(discoverySubserviceState == RUNNING_AS_MANAGER){
                    runAsMemberDiscovery();
                }
                customWriteMutexUnlock(customMutex, DATA_UPDATED);
            }
            free(managerHostname);
        }

        free(hostname);
        free(macAddress);
        free(ipAddress);
    }
}

void* discoverySubserviceControllerRoutine(){
    pthread_mutex_lock(&discoverySubserviceStateMutex);
    sem_post(&waitDiscoverySubserviceStart);
    while(TRUE){
        if(discoverySubserviceState == RUNNING_AS_MANAGER){
            pthread_cancel(updateManagerStatusThread);
            pthread_join(updateManagerStatusThread,NULL);
            pthread_create(&discoveryReceiverResponseThread,NULL,discoveryReceiverResponseRoutine,NULL);
            pthread_create(&discoverySenderThread,NULL,discoverySenderManagerRoutine,NULL);
        }else{
            pthread_cancel(discoveryReceiverResponseThread);
            pthread_cancel(discoverySenderThread);
            pthread_join(discoveryReceiverResponseThread,NULL);
            pthread_join(discoverySenderThread,NULL);
            pthread_create(&updateManagerStatusThread,NULL,updateManagerStatusRoutine,NULL);
        }

        pthread_cond_wait(&discoverySubserviceStateChanged,&discoverySubserviceStateMutex);
    }
}

void changeDiscoverySubserviceToMember(){
    pthread_mutex_lock(&discoverySubserviceStateMutex);
    if(discoverySubserviceState == RUNNING_AS_MANAGER){
        pthread_cond_signal(&discoverySubserviceStateChanged);
        discoverySubserviceState = RUNNING_AS_MEMBER;
    }
    pthread_mutex_unlock(&discoverySubserviceStateMutex);
}

void changeDiscoverySubserviceToManager(){
    pthread_mutex_lock(&discoverySubserviceStateMutex);
    if(discoverySubserviceState == RUNNING_AS_MEMBER){
        pthread_cond_signal(&discoverySubserviceStateChanged);
        discoverySubserviceState = RUNNING_AS_MANAGER;
    }
    pthread_mutex_unlock(&discoverySubserviceStateMutex);
}

void stopDiscoverySubservice(){
    pthread_cancel(discoveryReceiverRequestThread);
    pthread_cancel(discoveryReceiverResponseThread);
    pthread_cancel(discoverySenderThread);
    pthread_cancel(updateManagerStatusThread);
    pthread_cancel(discoverySubserviceControllerThread);
    closeSocket(discoverySocket);
    closeSocket(discoveryBroadcastSocket);
}

void runDiscoverySubservice(custom_mutex *mutex, void *(*runAsMem)()){
    char *ipAddress;
    char *broadcastIPAddress;
    getIPAddress(&ipAddress);
    getBroadcastIPAddress(&broadcastIPAddress);

    discoverySocket = createSocket(DISCOVERY_PORT,ipAddress);
    discoveryBroadcastSocket = createSocket(DISCOVERY_BROADCAST_PORT,broadcastIPAddress);
    discoverySubserviceState = RUNNING_AS_MEMBER;
    managerState = WAITING_FOR_MESSAGE;
    
    runAsMemberDiscovery = runAsMem;

    pthread_mutex_init(&discoverySubserviceStateMutex,NULL);
    pthread_cond_init(&discoverySubserviceStateChanged,NULL);
    sem_init(&waitDiscoverySubserviceStart,0,0);
    customMutex = mutex;

    pthread_create(&discoveryReceiverRequestThread,NULL,discoveryReceiverRequestRoutine,NULL);
    pthread_create(&discoverySubserviceControllerThread,NULL,discoverySubserviceControllerRoutine,NULL);

    sem_wait(&waitDiscoverySubserviceStart);
}