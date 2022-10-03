#include <string.h>
#include <stdio.h>
#include <stdlib.h> 
#include <unistd.h>
#include <pthread.h>
#include <semaphore.h>
#include <signal.h>
#include "members_table.h"
#include "network.h"
#include "election_subservice.h"
#include "member_ip_list.h"

#define ELECTION_FINISHED 0
#define ELECTION_STARTED 1
#define WAITING_FOR_ANSWER 2
#define WAITING_FOR_COORDINATOR 3
#define WAITING_FOR_ANSWER_PERIOD 2
#define WAITING_FOR_COORDINATOR_PERIOD 2
#define TRUE 1
#define RUNNING_AS_MEMBER 1
#define RUNNING_AS_MANAGER 2

pthread_mutex_t electionSubserviceStateMutex, electionMutex;
pthread_cond_t electionSubserviceStateChanged;
sem_t waitElectionSubserviceStart;

pthread_t electionThread, electionSenderThread, coordinatorSenderThread;

void *(*runAsMemberElection)();
void *(*runAsManagerElection)();

custom_mutex *customMutex;
int electionSocket;

int electionState;

void* coordinatorSenderRoutine(){
    package* pack;
    char* hostname;
    member_ip_list *list;
    member_ip_node *ipNode;

    pthread_mutex_lock(&electionMutex);
    if(electionState == WAITING_FOR_ANSWER){
        pthread_mutex_unlock(&electionMutex);
        pthread_setcancelstate(PTHREAD_CANCEL_ENABLE,NULL);
        sleep(WAITING_FOR_ANSWER_PERIOD);
        pthread_setcancelstate(PTHREAD_CANCEL_DISABLE,NULL);
    }else {
        pthread_mutex_unlock(&electionMutex);
    }

    pthread_mutex_lock(&electionMutex);
    if(electionState == WAITING_FOR_ANSWER){
        getHostname(&hostname);
        createCoordinatorPackage(&pack);
        
        customReadMutexLock(customMutex);
        getMemberIpListOfLTIdMembers(&list, hostname);
        ipNode = list->rootNode;
        customReadMutexUnlock(customMutex);

        while(ipNode != NULL){
            sendPackage(pack,ELECTION_PORT,ipNode->ipAddress);
            ipNode = ipNode->nextNode;
        }

        customWriteMutexLock(customMutex);
        setManagerByHostname(hostname);
        customWriteMutexUnlock(customMutex,DATA_UPDATED);
        
        runAsManagerElection();

        electionState = ELECTION_FINISHED;
        freeMemberIpList(list);
        freePackage(pack);
    }
    pthread_mutex_unlock(&electionMutex);
}

void* electionSenderRoutine(){
    package* pack;
    char* hostname;
    member_ip_list *list;
    member_ip_node *ipNode;

    pthread_mutex_lock(&electionMutex);
    if(electionState == WAITING_FOR_COORDINATOR){
        pthread_mutex_unlock(&electionMutex);
        pthread_setcancelstate(PTHREAD_CANCEL_ENABLE,NULL);
        sleep(WAITING_FOR_COORDINATOR_PERIOD);
        pthread_setcancelstate(PTHREAD_CANCEL_DISABLE,NULL);
    }else {
        pthread_mutex_unlock(&electionMutex);
    }

    pthread_mutex_lock(&electionMutex);
    if(electionState == WAITING_FOR_COORDINATOR){
        electionState = ELECTION_STARTED;
    }
    pthread_mutex_unlock(&electionMutex);

    pthread_mutex_lock(&electionMutex);
    if(electionState == ELECTION_STARTED){
        getHostname(&hostname);
        createElectionPackage(&pack);
        
        customReadMutexLock(customMutex);
        getMemberIpListOfGTIdMembers(&list, hostname);
        ipNode = list->rootNode;
        customReadMutexUnlock(customMutex);

        if(ipNode != NULL){
            while(ipNode != NULL){
                sendPackage(pack,ELECTION_PORT,ipNode->ipAddress);
                ipNode = ipNode->nextNode;
            }
        }
	electionState = WAITING_FOR_ANSWER;

        pthread_cancel(coordinatorSenderThread);
        pthread_join(coordinatorSenderThread,NULL);
        if(electionState != ELECTION_FINISHED){
            pthread_create(&coordinatorSenderThread,NULL,coordinatorSenderRoutine,NULL);
        }

        freeMemberIpList(list);
        freePackage(pack);
    }
    pthread_mutex_unlock(&electionMutex);
}

void startElection(){
    if(electionState == ELECTION_FINISHED){
        electionState = ELECTION_STARTED;
        pthread_create(&electionSenderThread,NULL,electionSenderRoutine,NULL);
    }
}

void* electionRoutine(){
    package* pack;
    char* hostname, *managerHostname, *workstationHostname;
    char* ipAddress;
    char* msgType;
    getHostname(&workstationHostname);
    while(TRUE){
        serve(&pack,&ipAddress,electionSocket);
        unpackElectionPackage(pack,&msgType);

        pthread_mutex_lock(&electionMutex);
        if(strcmp(msgType,ELECTION) == 0){
            if(electionState == ELECTION_FINISHED){
                electionState = ELECTION_STARTED;
                pthread_create(&electionSenderThread,NULL,electionSenderRoutine,NULL);
            }
            createAnswerPackage(&pack);
            sendPackage(pack,ELECTION_PORT,ipAddress);
            freePackage(pack);
        }else if(strcmp(msgType,ANSWER) == 0){
            if(electionState == WAITING_FOR_ANSWER){
                pthread_cancel(electionSenderThread);
                pthread_join(electionSenderThread,NULL);
                electionState = WAITING_FOR_COORDINATOR;
                pthread_create(&electionSenderThread,NULL,electionSenderRoutine,NULL);
            }
        }else if(strcmp(msgType,COORDINATOR) == 0){
            findHostnameByIPAddress(&hostname,ipAddress);
            if(hostname != NULL){
                if(electionState == WAITING_FOR_COORDINATOR){
                    customWriteMutexLock(customMutex);
                    setManagerByHostname(hostname);
                    electionState = ELECTION_FINISHED;
                    customWriteMutexUnlock(customMutex,DATA_UPDATED);
                }else{
                    customWriteMutexLock(customMutex);
                    if(hasManager()){
                        getManagerHostname(&managerHostname);
                        if(strcmp(hostname, managerHostname) > 0 ){
                            setManagerByHostname(hostname);
                            if(strcmp(workstationHostname, managerHostname) == 0){
                                runAsMemberElection();
                            }
                            customWriteMutexUnlock(customMutex,DATA_UPDATED);
                        }else {
                            customWriteMutexUnlock(customMutex,DATA_NOT_UPDATED);
                        }
                        free(managerHostname);
                    }else{
                        setManagerByHostname(hostname);
                        customWriteMutexUnlock(customMutex,DATA_UPDATED);
                    }
                    electionState = ELECTION_FINISHED;
                }
                free(hostname);
            }
        }
        free(ipAddress);
        free(msgType);
        pthread_mutex_unlock(&electionMutex);
    }
}

void stopElectionSubservice(){
    closeSocket(electionSocket);
}

void runElectionSubservice(custom_mutex *mutex, void *(*runAsMana)(), void *(*runAsMem)()){
    char *ipAddress;
    getIPAddress(&ipAddress);

    electionSocket = createSocket(ELECTION_PORT,ipAddress);
    
    runAsMemberElection = runAsMem;
    runAsManagerElection = runAsMana;

    electionState = ELECTION_FINISHED;

    pthread_mutex_init(&electionSubserviceStateMutex,NULL);
    pthread_cond_init(&electionSubserviceStateChanged,NULL);
    sem_init(&waitElectionSubserviceStart,0,0);
    customMutex = mutex;

    pthread_create(&electionThread,NULL,electionRoutine,NULL);

    sem_wait(&waitElectionSubserviceStart);
}
