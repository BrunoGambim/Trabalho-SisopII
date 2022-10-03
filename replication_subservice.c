#include <string.h>
#include <stdio.h>
#include <stdlib.h> 
#include <unistd.h>
#include <pthread.h>
#include <semaphore.h>
#include <signal.h>
#include "members_table.h"
#include "network.h"
#include "replication_subservice.h"

#define REPLICATION_SENDER_PERIOD 3
#define TEST_BUFFER_PERIOD 1
#define TRUE 1
#define RUNNING_AS_MEMBER 1
#define RUNNING_AS_MANAGER 2
#define REPLICATION_FINISHED 0
#define REPLICATION_RUNNING 1

pthread_mutex_t replicationSubserviceStateMutex, replicationBufferMutex;
pthread_cond_t replicationSubserviceStateChanged;
sem_t waitReplicationSubserviceStart, waitReplicationRoutineStart;

pthread_t replicationMemberThread, replicationManagerThread, replicationDataSenderThread,
          replicationReceiverThread, replicationSubserviceControllerThread;

custom_mutex *customMutex;

int replicationSocket;
int replicationBroadcastSocket;
replication_buffer *buffer;

int replicationSubserviceState;
int replicationState;

void sendAckPackage(char* managerIp, char* hostname){
    package* pack;

    createHostnamePackage(&pack, hostname);
    sendPackage(pack,REPLICATION_PORT,managerIp);

    freePackage(pack);
}

void* replicationMemberRoutine(){
    package* pack;
    char* hostname;
    char* macAddress;
    char* ipAddress;
    char* managerIp;
    char* status;
    char* mark;
    while(TRUE){
        pthread_setcancelstate(PTHREAD_CANCEL_ENABLE,NULL);
        serve(&pack,&managerIp,replicationBroadcastSocket);
        pthread_testcancel();
        pthread_setcancelstate(PTHREAD_CANCEL_DISABLE,NULL);

        unpackDataPackage(pack,&hostname,&macAddress,&ipAddress,&status,&mark);
        sendAckPackage(managerIp, hostname);

        customWriteMutexLock(customMutex);
        if(!isHostnameInTheTable(hostname) && strcmp(mark,NOT_MARKED) == 0){
            addLine(hostname,macAddress,ipAddress,status);
            customWriteMutexUnlock(customMutex,DATA_UPDATED);
        }else if(!memberStatusIs(hostname,status) && strcmp(mark,NOT_MARKED) == 0){
            removeLineByHostname(hostname);
            addLine(hostname,macAddress,ipAddress,status);
            customWriteMutexUnlock(customMutex,DATA_UPDATED);
        }else if(isHostnameInTheTable(hostname) && strcmp(mark,MARKED) == 0){
            removeLineByHostname(hostname);
            customWriteMutexUnlock(customMutex,DATA_UPDATED);
        }else{
            customWriteMutexUnlock(customMutex,DATA_NOT_UPDATED);
        }

        free(hostname);
        free(macAddress);
        free(ipAddress);
        free(status);
    }
}

void* replicationDataSenderManagerRoutine(){
    package *pack;
    data_node *node;
    char *broadcastIp;
    int i;
    getBroadcastIPAddress(&broadcastIp);
    while(TRUE){

	pthread_mutex_lock(&replicationBufferMutex);
        node = buffer->dataList;
        while(node != NULL){
            createDataPackage(&pack,node->hostname,node->macAddress,node->ipAddress,node->status, node->mark);
            sendPackage(pack,REPLICATION_PORT,broadcastIp);
            freePackage(pack);
            pack = NULL;
            node = node->nextNode;
        }
	pthread_mutex_unlock(&replicationBufferMutex);

        pthread_setcancelstate(PTHREAD_CANCEL_ENABLE,NULL);
        sleep(REPLICATION_SENDER_PERIOD);
        pthread_setcancelstate(PTHREAD_CANCEL_DISABLE,NULL);

        pthread_mutex_lock(&replicationBufferMutex);
	if(buffer != NULL){
		customReadMutexLock(customMutex);
		updateBuffer(buffer);
		customReadMutexUnlock(customMutex);
	}

        if(isBufferAcked(buffer)){
            pthread_mutex_unlock(&replicationBufferMutex);

            pthread_cancel(replicationReceiverThread);
            pthread_cancel(replicationDataSenderThread);

            pthread_join(replicationReceiverThread,NULL);

            replicationState = REPLICATION_FINISHED;
            pthread_setcancelstate(PTHREAD_CANCEL_ENABLE,NULL);
	    break;
        }else{
	    pthread_mutex_unlock(&replicationBufferMutex);
	}
    }
}

void* replicationReceiverManagerRoutine(){
    char *hostname, *ipAddress;
    package* ack;
    while(TRUE){
            pthread_setcancelstate(PTHREAD_CANCEL_ENABLE,NULL);
            serve(&ack,&ipAddress,replicationSocket);
            pthread_testcancel();
            pthread_setcancelstate(PTHREAD_CANCEL_DISABLE,NULL);

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

void* replicationManagerRoutine(){
    customReplicationMutexLock(customMutex);
    sem_post(&waitReplicationRoutineStart);
    while(TRUE){
        customReplicationWait(customMutex);
        pthread_mutex_lock(&replicationSubserviceStateMutex);
        if(replicationState == REPLICATION_FINISHED && replicationSubserviceState == RUNNING_AS_MANAGER){

            if(buffer != NULL){
                freeBuffer(buffer);
                buffer = NULL;
            }
            createReplicationBuffer(&buffer);

            customReadMutexLock(customMutex);
            addBufferData(buffer);
            customReadMutexUnlock(customMutex);
            pthread_create(&replicationDataSenderThread,NULL,replicationDataSenderManagerRoutine,NULL);
            pthread_create(&replicationReceiverThread,NULL,replicationReceiverManagerRoutine,NULL);
            replicationState = REPLICATION_RUNNING;
        }
        pthread_mutex_unlock(&replicationSubserviceStateMutex);
    }
}

void* replicationSubserviceControllerRoutine(){
    pthread_mutex_lock(&replicationSubserviceStateMutex);
    sem_post(&waitReplicationSubserviceStart);
    while(TRUE){
        if(replicationSubserviceState == RUNNING_AS_MANAGER){
            pthread_cancel(replicationMemberThread);
            pthread_join(replicationMemberThread,NULL);
            replicationState = REPLICATION_FINISHED;
        }else{
            pthread_cancel(replicationDataSenderThread);
            pthread_cancel(replicationReceiverThread);
            pthread_join(replicationDataSenderThread,NULL);
            pthread_join(replicationReceiverThread,NULL);
            pthread_create(&replicationMemberThread,NULL,replicationMemberRoutine,NULL);
        }

        pthread_cond_wait(&replicationSubserviceStateChanged,&replicationSubserviceStateMutex);
    }
}

void changeReplicationSubserviceToMember(){
    pthread_mutex_lock(&replicationSubserviceStateMutex);
    if(replicationSubserviceState == RUNNING_AS_MANAGER){
        pthread_cond_signal(&replicationSubserviceStateChanged);
        replicationSubserviceState = RUNNING_AS_MEMBER;
    }
    pthread_mutex_unlock(&replicationSubserviceStateMutex);
}

void changeReplicationSubserviceToManager(){
    pthread_mutex_lock(&replicationSubserviceStateMutex);
    if(replicationSubserviceState == RUNNING_AS_MEMBER){
        pthread_cond_signal(&replicationSubserviceStateChanged);
        replicationSubserviceState = RUNNING_AS_MANAGER;
    }
    pthread_mutex_unlock(&replicationSubserviceStateMutex);
}

void stopReplicationSubservice(){
    closeSocket(replicationSocket);
    closeSocket(replicationBroadcastSocket);
}

void runReplicationSubservice(custom_mutex *mutex){
    char *ipAddress;
    char *broadcastIPAddress;
    getIPAddress(&ipAddress);
    getBroadcastIPAddress(&broadcastIPAddress);

    replicationSocket = createSocket(REPLICATION_PORT,ipAddress);
    replicationBroadcastSocket = createSocket(REPLICATION_PORT,broadcastIPAddress);

    replicationSubserviceState = RUNNING_AS_MEMBER;

    pthread_mutex_init(&replicationSubserviceStateMutex,NULL);
    pthread_mutex_init(&replicationBufferMutex,NULL);
    pthread_cond_init(&replicationSubserviceStateChanged,NULL);
    sem_init(&waitReplicationSubserviceStart,0,0);
    sem_init(&waitReplicationRoutineStart,0,0);
    customMutex = mutex;

    pthread_create(&replicationManagerThread,NULL,replicationManagerRoutine,NULL);
    pthread_create(&replicationSubserviceControllerThread,NULL,replicationSubserviceControllerRoutine,NULL);

    sem_wait(&waitReplicationRoutineStart);
    sem_wait(&waitReplicationSubserviceStart);
}
