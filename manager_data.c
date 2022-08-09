#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include "manager_data.h"

manager_data* managerData;

void freeManagerData(manager_data* data){
    free(data->hostname);
    free(data->macAddress);
    free(data->ipAddress);
    free(data);
    data = NULL;
}

int managerDataIsNull(){
    return managerData == NULL;
}

void getManagerIPAddress(char** ipAdress){
    *ipAdress = managerData->ipAddress;
}

void createManagerData(char *hostname, char *macAddress, char *ipAddress, manager_data** data){
    *data = (manager_data*) malloc(sizeof(manager_data));
    (*data)->hostname = strdup(hostname);
    (*data)->macAddress = strdup(macAddress);
    (*data)->ipAddress = strdup(ipAddress);
}

void updateData(char *hostname, char *macAddress, char *ipAddress){
    manager_data* data;
    createManagerData(hostname,macAddress,ipAddress,&data);
    managerData = data;
}

int updateManagerData(char *hostname, char *macAddress, char *ipAddress){
    if(managerDataIsNull()) {
        updateData(hostname,macAddress,ipAddress);
        return 1;
    }else if(strcmp(managerData->hostname,hostname) != 0){
        freeManagerData(managerData);
        updateData(hostname,macAddress,ipAddress);
        return 1;
    }
}

void printManagerData(){
    if(!managerDataIsNull()){
        printf("Hostname              Endereco_MAC             Endereco_IP\n");
        printf("%s           %s        %s\n", managerData->hostname, managerData->macAddress, managerData->ipAddress);
    }
}