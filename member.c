#include <string.h>
#include <stdio.h>
#include <stdlib.h> 
#include <unistd.h>
#include <pthread.h>
#include <semaphore.h>
#include <signal.h>
#include "network.h"
#include "member_data.h"
#include "custom_mutex.h"

#define TRUE 1
#define BUFFER_SIZE 50
#define EXIT_CMD "EXIT"
#define CLEAR "clear"

pthread_mutex_t printMutex;
pthread_cond_t dataUpdatedCond;
sem_t waitPrintRoutineStart;
custom_mutex* customMutex;
pthread_t printThread, interfaceReaderThread,
        discoveryReceiverThread, updateSenderThread;

int discoverySocket;
member_data* managerData;

void updateManagerData(member_data* data){
    if(managerData != NULL){
        freeMemberData(managerData);
    }
    managerData = data;
}

void updateData(char *hostname, char *macAddress, char *ipAddress, char *status){
    member_data* data;
    createMemberData(hostname,macAddress,ipAddress,status,&data);

    customReadMutexLock(customMutex);
    if(managerData == NULL){
        customReadMutexUnlock(customMutex);
        
        customWriteMutexLock(customMutex);
        updateManagerData(data);
        customWriteMutexUnlock(customMutex,DATA_UPDATED);
    }else if(strcmp(managerData->hostname,data->hostname) != 0){
        customReadMutexUnlock(customMutex);
        
        customWriteMutexLock(customMutex);
        updateManagerData(data);
        customWriteMutexUnlock(customMutex,DATA_UPDATED);
    }else{
        customReadMutexUnlock(customMutex);
    }
}

void printManagerData(){
    printf("Hostname        Endereco_MAC        Endereco_IP\n");
    printf("%s           %s        %s\n", managerData->hostname, managerData->macAddress, managerData->ipAddress);
}

void* printDataRoutine(){
    pthread_mutex_lock(&printMutex);
    sem_post(&waitPrintRoutineStart);
    while (TRUE){
        pthread_cond_wait(&dataUpdatedCond,&printMutex);

        if(managerData != NULL){
            system(CLEAR);
            printManagerData();
        }
    }
}


void discoverySender(char* managerIp){
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
    char* hostname;
    char* macAddress;
    char* ipAddress;
    while(TRUE){
        serve(&pack,&ipAddress,discoverySocket);
        unpackDataPackage(pack,&hostname,&macAddress);
        discoverySender(ipAddress);
        updateData(hostname,macAddress,ipAddress,"");
        free(hostname);
        free(macAddress);
        free(ipAddress);
    }
    closeSocket(discoverySocket);
}

void* updateSenderRoutine(){
    package* pack;
    char* hostname;
    getHostname(&hostname);
    createHostnamePackage(&pack, hostname);
    while(TRUE){

        customReadMutexLock(customMutex);
        if(managerData != NULL){
            sendPackage(pack,UPDATE_PORT,managerData->ipAddress);
        }
        customReadMutexUnlock(customMutex);
        
        sleep(1);
    }
    freePackage(pack);
}

void sendExitPackageToTheManager(){
    char* hostname;
    package* pack;
    getHostname(&hostname);
    createHostnamePackage(&pack, hostname);

    customReadMutexLock(customMutex);
    if(managerData != NULL){
        sendPackage(pack,EXIT_PORT,managerData->ipAddress);
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
