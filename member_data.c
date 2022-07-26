#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include "member_data.h"

void createMemberData(char *hostname, char *macAddress, char *ipAddress, char *status, member_data** data){
    *data = (member_data*) malloc(sizeof(member_data));
    (*data)->hostname = strdup(hostname);
    (*data)->macAddress = strdup(macAddress);
    (*data)->ipAddress = strdup(ipAddress);
    (*data)->status = strdup(status);
}

void freeMemberData(member_data* data){
    free(data->hostname);
    free(data->macAddress);
    free(data->ipAddress);
    free(data->status);
    free(data);
}