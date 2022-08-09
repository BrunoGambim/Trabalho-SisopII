#include <string.h>
#include <stdio.h>
#include <stdlib.h> 
#include <unistd.h>
#include <pthread.h>
#include <semaphore.h>
#include <signal.h>
#include "members_table.h"
#include "network.h"
#include "custom_mutex.h"

#define TRUE 1
#define BUFFER_SIZE 50
#define WAKEUP_CMD "WAKEUP"
#define CLEAR "clear"
#define DISCOVERY_SENDER_PERIOD 3
#define UPDATE_STATUS_PERIOD 1

pthread_mutex_t printMutex;
pthread_cond_t dataUpdatedCond;
sem_t waitPrintRoutineStart;
custom_mutex* customMutex;

pthread_t printThread, updateStatusThread, 
          discoverySenderThread, discoveryReceiverThread,
          updateStatusReceiverThread, interfaceReaderThread,
          exitReceiverThread;

int updateSocket;
int discoverySocket;
int exitSocket;

void* printDataRoutine(){
    pthread_mutex_lock(&printMutex);
    sem_post(&waitPrintRoutineStart);
    while (TRUE){
        pthread_cond_wait(&dataUpdatedCond,&printMutex);

        system(CLEAR);
        printTable();
    }
}


void addData(char *hostname, char *macAddress, char *ipAddress, char *status){
    customWriteMutexLock(customMutex);

    addLine(hostname,macAddress,ipAddress,status);

    customWriteMutexUnlock(customMutex, DATA_UPDATED);

    free(hostname);
    free(macAddress);
    free(ipAddress);
    
}

void* discoverySenderRoutine(){
    package* pack;
    char* hostname;
    char* macAddress;
    char* broadcastIp;
    getBroadcastIPAddress(&broadcastIp);
    getHostname(&hostname);
    getMACAddress(&macAddress);

    createDataPackage(&pack, hostname, macAddress);
    while(TRUE){
        sendPackage(pack,BROADCAST_PORT,broadcastIp);
        sleep(DISCOVERY_SENDER_PERIOD);
    }
    freePackage(pack);
}

void* discoveryReceiverRoutine(){
    package* pack;
    int isHostInTheTable;
    char* hostname;
    char* macAddress;
    char* ipAddress;
    
    while(TRUE){
        serve(&pack,&ipAddress,discoverySocket);
        unpackDataPackage(pack,&hostname,&macAddress);
        
        customReadMutexLock(customMutex);
        isHostInTheTable = isHostnameInTheTable(hostname);
        customReadMutexUnlock(customMutex);

        if(isHostInTheTable){
            free(hostname);
            free(macAddress);
            free(ipAddress);
        }else{
            addData(hostname,macAddress,ipAddress,AWAKEN);
        }
        
    }
}

void* updateStatusReceiverRoutine(){
    package* pack;
    char* hostname;
    while(TRUE){
        serve(&pack,NULL,updateSocket);
        unpackHostnamePackage(pack,&hostname);

        customWriteMutexLock(customMutex);

        updateTimestamp(hostname);

        customWriteMutexUnlock(customMutex, DATA_NOT_UPDATED);
        free(hostname);
    }
}

void* updateStatusRoutine(void* aux){
    int dataUpdated;
    while(TRUE){
        customWriteMutexLock(customMutex);

        dataUpdated = updateMembersStatus();

        customWriteMutexUnlock(customMutex, dataUpdated);
        sleep(UPDATE_STATUS_PERIOD);
    }
}

void* interfaceReaderRoutine(){
    char cmd[BUFFER_SIZE], hostname[BUFFER_SIZE], *macAddress;
    package* pack;
    char* broadcastIp;
    getBroadcastIPAddress(&broadcastIp);
    while (TRUE){
        scanf("%s %s",cmd, hostname);
        findMACAddressByHostname(hostname, &macAddress);
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

void removeHostFromTable(char* hostname){
    customWriteMutexLock(customMutex);

    removeLineByHostname(hostname);

    customWriteMutexUnlock(customMutex, DATA_UPDATED);
}

void* exitReceiverRoutine(){
    package* pack;
    char* hostname;
    while(TRUE){
        serve(&pack,NULL,exitSocket);
        unpackHostnamePackage(pack,&hostname);
        
        removeHostFromTable(hostname);
        
        free(hostname);
        hostname == NULL;
    }
}

void finishProcess(){
    closeSocket(updateSocket);
    closeSocket(discoverySocket);
    closeSocket(exitSocket);
    exit(EXIT_SUCCESS);
}

int main(){
    char* managerIp;
    getIPAddress(&managerIp);
    updateSocket = createSocket(UPDATE_PORT,managerIp);
    discoverySocket = createSocket(DISCOVERY_PORT,managerIp);
    exitSocket = createSocket(EXIT_PORT,managerIp);
    
    pthread_mutex_init(&printMutex,NULL);
    pthread_cond_init(&dataUpdatedCond,NULL);
    createCustomMutex(&customMutex, &dataUpdatedCond, &printMutex);
    sem_init(&waitPrintRoutineStart,0,0);

    pthread_create(&printThread,NULL,printDataRoutine,NULL);

    sem_wait(&waitPrintRoutineStart);
    pthread_create(&discoveryReceiverThread,NULL,discoveryReceiverRoutine,NULL);
    pthread_create(&discoverySenderThread,NULL,discoverySenderRoutine,NULL);
    pthread_create(&interfaceReaderThread,NULL,interfaceReaderRoutine,NULL);
    pthread_create(&exitReceiverThread,NULL,exitReceiverRoutine,NULL);
    pthread_create(&updateStatusReceiverThread,NULL,updateStatusReceiverRoutine, NULL);
    pthread_create(&updateStatusThread,NULL,updateStatusRoutine,NULL);

    signal(SIGINT, finishProcess);

    pthread_join(printThread, NULL);
    pthread_join(discoveryReceiverThread, NULL);
    pthread_join(discoverySenderThread, NULL);
    pthread_join(updateStatusThread, NULL);
    pthread_join(updateStatusReceiverThread, NULL);
    pthread_join(exitReceiverThread, NULL);
    
    closeSocket(updateSocket);
    closeSocket(discoverySocket);
    closeSocket(exitSocket);

    return 0;
}
