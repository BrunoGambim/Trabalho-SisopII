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
#include "replication_buffer.h"

#define TRUE 1
#define BUFFER_SIZE 50
#define WAKEUP_CMD "WAKEUP"
#define CLEAR "clear"
#define DISCOVERY_SENDER_PERIOD 3
#define REPLICATION_SENDER_PERIOD 3
#define TEST_BUFFER_PERIOD 1
#define UPDATE_STATUS_PERIOD 1


pthread_mutex_t printMutex, replicationBufferMutex, replicationMutex;
pthread_cond_t dataUpdatedCond;
sem_t waitPrintRoutineStart, waitReplicationRoutineStart;
custom_mutex* customMutex;

pthread_t printThread, updateStatusThread, 
          discoverySenderThread, discoveryReceiverThread,
          updateStatusReceiverThread, interfaceReaderThread,
          exitReceiverThread, replicationThread,
          replicationDataSenderThread,replicationReceiverThread;

int updateSocket;
int discoverySocket;
int exitSocket;
int replicationSocket;

replication_buffer *buffer;

void* replicationDataSenderRoutine(){
    package *pack;
    data_node *node;
    char *broadcastIp;
    int i;
    getBroadcastIPAddress(&broadcastIp);
    while(TRUE){
        node = buffer->dataList;
        while(node != NULL){
            createDataPackage(&pack,node->hostname,node->macAddress,node->ipAddress,node->status);
            sendPackage(pack,REPLICATION_PORT,broadcastIp);
            freePackage(pack);
            pack = NULL;
            node = node->nextNode;
        }
        sleep(REPLICATION_SENDER_PERIOD);
        pthread_mutex_lock(&replicationBufferMutex);
        updateBufferMembers(buffer);
        pthread_mutex_unlock(&replicationBufferMutex);
    }
}

void* replicationReceiverRoutine(){
    char *hostname, *ipAddress;
    package* ack;
    while(TRUE){
            serve(&ack,&ipAddress,replicationSocket);
            unpackHostnamePackage(ack,&hostname);

            pthread_mutex_lock(&replicationBufferMutex);
            ackBuffer(buffer,hostname,ipAddress);
            pthread_mutex_unlock(&replicationBufferMutex);

            free(hostname);
            free(ipAddress);
            hostname = NULL;
            ipAddress = NULL;
        }
}

void* replicationRoutine(){
    
    pthread_mutex_lock(&replicationMutex);
    sem_post(&waitReplicationRoutineStart);
    while(TRUE){
        pthread_cond_wait(&dataUpdatedCond,&replicationMutex);
        buffer = NULL;
        createReplicationBuffer(&buffer);

        customReadMutexLock(customMutex);
        addBufferData(buffer);
        customReadMutexUnlock(customMutex);

        pthread_create(&replicationDataSenderThread,NULL,replicationDataSenderRoutine,NULL);
        pthread_create(&replicationReceiverThread,NULL,replicationReceiverRoutine,NULL);

        while(TRUE){
            if(isBufferAcked(buffer)){
                pthread_cancel(replicationDataSenderThread);
                pthread_cancel(replicationReceiverThread);
                break;
            }
            sleep(TEST_BUFFER_PERIOD);
        }
        
        freeBuffer(buffer);
    }
}

void* printDataRoutine(){
    pthread_mutex_lock(&printMutex);
    sem_post(&waitPrintRoutineStart);

    while (TRUE){
        pthread_cond_wait(&dataUpdatedCond,&printMutex);

        system(CLEAR);
        printMembers();
    }
}

void* discoverySenderRoutine(){
    package* pack;
    char* hostname;
    char* macAddress;
    char* broadcastIp;
    getBroadcastIPAddress(&broadcastIp);
    getHostname(&hostname);
    getMACAddress(&macAddress);

    createDiscoveryPackage(&pack, hostname, macAddress);
    while(TRUE){
        sendPackage(pack,DISCOVERY_BROADCAST_PORT,broadcastIp);
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

void* exitReceiverRoutine(){
    package* pack;
    char* hostname;
    while(TRUE){
        serve(&pack,NULL,exitSocket);
        unpackHostnamePackage(pack,&hostname);
        
        customWriteMutexLock(customMutex);
        removeLineByHostname(hostname);
        customWriteMutexUnlock(customMutex, DATA_UPDATED);
        
        free(hostname);
        hostname = NULL;
    }
}

void finishProcess(){
    closeSocket(updateSocket);
    closeSocket(discoverySocket);
    closeSocket(exitSocket);
    closeSocket(replicationSocket);
    exit(EXIT_SUCCESS);
}

int main(){
    char* managerIp;
    char* hostname;
    char* macAddress;

    initNetworkGlobalVariables();
    getIPAddress(&managerIp);
    getMACAddress(&macAddress);
    getHostname(&hostname);
    updateSocket = createSocket(UPDATE_PORT,managerIp);
    discoverySocket = createSocket(DISCOVERY_PORT,managerIp);
    replicationSocket = createSocket(REPLICATION_PORT,managerIp);
    exitSocket = createSocket(EXIT_PORT,managerIp);

    if(!isHostnameInTheTable(hostname)){
        addLine(hostname,macAddress,managerIp,AWAKEN);
    }

    pthread_mutex_init(&replicationMutex,NULL);
    pthread_mutex_init(&replicationBufferMutex,NULL);
    pthread_mutex_init(&printMutex,NULL);
    pthread_cond_init(&dataUpdatedCond,NULL);
    createCustomMutex(&customMutex, &dataUpdatedCond, &printMutex, &replicationMutex);
    sem_init(&waitPrintRoutineStart,0,0);
    sem_init(&waitReplicationRoutineStart,0,0);

    pthread_create(&printThread,NULL,printDataRoutine,NULL);
    pthread_create(&replicationDataSenderThread,NULL,replicationRoutine,NULL);

    sem_wait(&waitPrintRoutineStart);
    sem_wait(&waitReplicationRoutineStart);

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
    pthread_join(replicationDataSenderThread, NULL);
    
    closeSocket(updateSocket);
    closeSocket(discoverySocket);
    closeSocket(exitSocket);
    closeSocket(replicationSocket);

    return 0;
}
