#include <string.h>
#include <stdio.h>
#include <stdlib.h> 
#include <unistd.h>
#include <pthread.h>
#include <semaphore.h>
#include <signal.h>
#include "network.h"
#include "manager_data.h"
#include "custom_mutex.h"

#define TRUE 1
#define BUFFER_SIZE 50
#define EXIT_CMD "EXIT"
#define CLEAR "clear"
#define UPDATE_SENDER_PERIOD 1

pthread_mutex_t printMutex;
pthread_cond_t dataUpdatedCond;
sem_t waitPrintRoutineStart;
custom_mutex* customMutex;
pthread_t printThread, interfaceReaderThread,
        discoveryReceiverThread, updateSenderThread;

int discoverySocket;

void* printDataRoutine(){
    pthread_mutex_lock(&printMutex);
    sem_post(&waitPrintRoutineStart);
    while (TRUE){
        pthread_cond_wait(&dataUpdatedCond,&printMutex);
        system(CLEAR);
        printManagerData();
    }
}


void sendDiscoveryPackage(char* managerIp){
    package* pack;
    char* hostname;
    char* macAddress;
    getHostname(&hostname);
    getMACAddress(&macAddress);

    createDataPackage(&pack, hostname, macAddress);
    sendPackage(pack,DISCOVERY_PORT,managerIp);

    freePackage(pack);
}

void* discoveryReceiverRoutine(){
    package* pack;
    int managerIsNull;
    char* hostname;
    char* macAddress;
    char* ipAddress;
    while(TRUE){
        serve(&pack,&ipAddress,discoverySocket);
        unpackDataPackage(pack,&hostname,&macAddress);
        sendDiscoveryPackage(ipAddress);

        customReadMutexLock(customMutex);
        managerIsNull = managerDataIsNull();
        customReadMutexUnlock(customMutex);
        
        if(managerIsNull){
            customWriteMutexLock(customMutex);
            updateManagerData(hostname, macAddress, ipAddress);
            customWriteMutexUnlock(customMutex,DATA_UPDATED);
        }

        free(hostname);
        free(macAddress);
        free(ipAddress);
    }
    closeSocket(discoverySocket);
}

void* updateSenderRoutine(){
    char* hostname;
    char* managerIpAddress;
    package* pack;
    getHostname(&hostname);
    createHostnamePackage(&pack, hostname);
    while(TRUE){

        customReadMutexLock(customMutex);
        if(!managerDataIsNull()){
            getManagerIPAddress(&managerIpAddress);
            sendPackage(pack,UPDATE_PORT,managerIpAddress);
        }
        customReadMutexUnlock(customMutex);
        
        sleep(UPDATE_SENDER_PERIOD);
    }
    freePackage(pack);
}

void sendExitPackageToTheManager(){
    char* hostname;
    char* managerIpAddress;
    package* pack;
    getHostname(&hostname);
    createHostnamePackage(&pack, hostname);

    customReadMutexLock(customMutex);
    if(!managerDataIsNull()){
        getManagerIPAddress(&managerIpAddress);
        sendPackage(pack,EXIT_PORT,managerIpAddress);
    }
    customReadMutexUnlock(customMutex);

    freePackage(pack);
    pack = NULL;
}

void finishProcess(){
    sendExitPackageToTheManager();
    closeSocket(discoverySocket);

    exit(EXIT_SUCCESS);
}

void* interfaceReaderRoutine(){
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

int main(){
    char* broadcastIp;
    initNetworkGlobalVariables();
    getBroadcastIPAddress(&broadcastIp);
    discoverySocket = createSocket(BROADCAST_PORT,broadcastIp);

    sem_init(&waitPrintRoutineStart,0,0);
    pthread_mutex_init(&printMutex,NULL);
    pthread_cond_init(&dataUpdatedCond,NULL);
    createCustomMutex(&customMutex, &dataUpdatedCond, &printMutex);

    pthread_create(&printThread,NULL,printDataRoutine,NULL);

    sem_wait(&waitPrintRoutineStart);
    pthread_create(&updateSenderThread,NULL,updateSenderRoutine,NULL);
    pthread_create(&discoveryReceiverThread,NULL,discoveryReceiverRoutine,NULL);
    pthread_create(&interfaceReaderThread,NULL,interfaceReaderRoutine,NULL);

    signal(SIGINT, finishProcess);

    pthread_join(printThread, NULL);
    pthread_join(interfaceReaderThread, NULL);
    pthread_join(discoveryReceiverThread, NULL);
    pthread_join(updateSenderThread, NULL);

    closeSocket(discoverySocket);

    return 0;
}
