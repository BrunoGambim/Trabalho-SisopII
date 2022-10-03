#include <string.h>
#include <stdio.h>
#include <stdlib.h> 
#include <unistd.h>
#include <pthread.h>
#include <semaphore.h>
#include <signal.h>
#include "members_table.h"
#include "network.h"
#include "print_subservice.h"

#define TRUE 1
#define CLEAR "clear"

sem_t waitDiscoverySubserviceStart;

pthread_t printThread;

custom_mutex *customMutex;


void* printMembersRoutine(){
    customPrintMutexLock(customMutex);
    sem_post(&waitDiscoverySubserviceStart);
    while (TRUE){
        customPrintWait(customMutex);

        system(CLEAR);
        printManager();
        printMembers();
    }
}


void runPrintSubservice(custom_mutex *mutex){
    sem_init(&waitDiscoverySubserviceStart,0,0);

    customMutex = mutex;
    pthread_create(&printThread,NULL,printMembersRoutine,NULL);

    sem_wait(&waitDiscoverySubserviceStart);
}
