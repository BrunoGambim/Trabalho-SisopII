#include <string.h>
#include <stdio.h>
#include <stdlib.h> 
#include <unistd.h>
#include <pthread.h>
#include <semaphore.h>
#include "network.h"
#include "members_table.h"
#include "discovery_subservice.h"
#include "election_subservice.h"
#include "exit_subservice.h"
#include "interface_reader_subservice.h"
#include "print_subservice.h"
#include "replication_subservice.h"
#include "update_status_subservice.h"

#define MANAGER_FLAG "manager"
#define NO_ARGS 1
#define ONE_ARG 2

custom_mutex* customMutex;

void* stopAllSubservices(){
    stopDiscoverySubservice();
    stopElectionSubservice();
    stopExitSubservice();
    stopInterfaceReaderSubservice();
    stopPrintSubservice();
    stopReplicationSubservice();
    stopUpdateStatusSubservice();
}

void* runAsManager(){
    changeReplicationSubserviceToManager();
    changeDiscoverySubserviceToManager();
    changeExitSubserviceToManager();
    changeInterfaceReaderSubserviceToManager();
    changeUpdateStatusSubserviceToManager();
}

void* runAsMember(){
    changeReplicationSubserviceToMember();
    changeDiscoverySubserviceToMember();
    changeExitSubserviceToMember();
    changeInterfaceReaderSubserviceToMember();
    changeUpdateStatusSubserviceToMember();
}

int main(int argc, char **argv){
    char* ipAddress;
    char* hostname;
    char* macAddress;

    initNetworkGlobalVariables();
    getIPAddress(&ipAddress);
    getMACAddress(&macAddress);
    getHostname(&hostname);

    if(!isHostnameInTheTable(hostname)){
        addLine(hostname,macAddress,ipAddress,AWAKEN);
    }


    createCustomMutex(&customMutex);

    runPrintSubservice(customMutex);
    runReplicationSubservice(customMutex);
    
    runDiscoverySubservice(customMutex, runAsMember);
    runExitSubservice(customMutex,stopAllSubservices);
    runInterfaceReaderSubservice(customMutex);
    runUpdateStatusSubservice(customMutex);
    runElectionSubservice(customMutex, runAsManager, runAsMember);

    while(1){}
    return 0;
}