#include <string.h>
#include <stdio.h>
#include <stdlib.h> 
#include <unistd.h>
#include <time.h>
#include "members_table.h"

#define MAXIMUM_UPDATE_INTERVAL 3

typedef struct table_lines{
    char *hostname;
    char *macAddress;
    char *ipAddress;
    char *status;
    int isManager;
    unsigned int timestamp;
    struct table_lines* nextLine;
} table_line;

table_line *table;

table_line *markedLines;

int isTableEmpty(){
    return table == NULL;
}

int isTheLastLine(table_line* line){
    return line->nextLine == NULL;
}

unsigned int getTimestamp(){
    return (unsigned)time(NULL);
}

void getLastLine(table_line** lastLine){
    table_line* iteratorLine;
    if(!isTableEmpty()){
        iteratorLine = table;
        while (!isTheLastLine(iteratorLine))
        {
            iteratorLine = iteratorLine->nextLine;
        }
        *lastLine = iteratorLine;
    }else{
        *lastLine = table;
    }
}

void insertLine(table_line* newLine){
    table_line* lastLine;
    if(isTableEmpty()){
        table = newLine;
    }else{
        getLastLine(&lastLine);
        lastLine->nextLine = newLine;
    }
}

void addLine(char *hostname, char *macAddress, char *ipAddress, char *status){
    table_line* newLine = (table_line*) malloc(sizeof(table_line));

    newLine->hostname = strdup(hostname);
    newLine->macAddress = strdup(macAddress);
    newLine->ipAddress = strdup(ipAddress);
    newLine->status = strdup(status);

    newLine->timestamp = getTimestamp();
    newLine->isManager = 0;

    newLine->nextLine = NULL;
    insertLine(newLine);
}

int isHostnameInTheTable(char* hostname){
    table_line* iteratorLine;
    if(!isTableEmpty()){
        iteratorLine = table;
        while (iteratorLine != NULL){
            if(strcmp(hostname, iteratorLine->hostname) == 0){
                return 1;
            }
            iteratorLine = iteratorLine->nextLine;
        }
    }
    return 0;
}

int memberStatusIs(char* hostname,char* status){
    table_line* iteratorLine;
    if(!isTableEmpty()){
        iteratorLine = table;
        while (iteratorLine != NULL){
            if(strcmp(hostname, iteratorLine->hostname) == 0){
  	        if(strcmp(status, iteratorLine->status) == 0){
                    return 1;
                }
            }
            iteratorLine = iteratorLine->nextLine;
        }
    }
    return 0;
}

void findLineByHostname(char* hostname,table_line** line){
    table_line* iteratorLine;
    *line = NULL;
    if(!isTableEmpty()){
        iteratorLine = table;
        while (strcmp(hostname, iteratorLine->hostname) != 0 && !isTheLastLine(iteratorLine)){
            iteratorLine = iteratorLine->nextLine;
        }
        if(strcmp(hostname, iteratorLine->hostname) == 0){
            *line = iteratorLine;
        }
    }
}

void findLineByIPAddress(char* ipAddress,table_line** line){
    table_line* iteratorLine;
    *line = NULL;
    if(!isTableEmpty()){
        iteratorLine = table;
        while (strcmp(ipAddress, iteratorLine->ipAddress) != 0 && !isTheLastLine(iteratorLine)){
            iteratorLine = iteratorLine->nextLine;
        }
        if(strcmp(ipAddress, iteratorLine->ipAddress) == 0){
            *line = iteratorLine;
        }
    }
}

void findHostnameByIPAddress(char** hostname, char* ipAddress){
    table_line* line;
    findLineByIPAddress(ipAddress, &line);
    if(line != NULL){
        *hostname = strdup(line->hostname);
    }else{
        *hostname = NULL;
    }
}

void findManagerLine(table_line** line){
    table_line* iteratorLine;
    *line = NULL;
    if(!isTableEmpty()){
        iteratorLine = table;
        while (iteratorLine->isManager != 1 && !isTheLastLine(iteratorLine)){
            iteratorLine = iteratorLine->nextLine;
        }
        if(iteratorLine->isManager == 1){
            *line = iteratorLine;
        }
    }
}

int hasManager(){
    table_line* line;
    findManagerLine(&line);
    return line != NULL;
}

void getManagerIPAddress(char** ipAddress){
    table_line* line;
    if(hasManager()){
        findManagerLine(&line);
        *ipAddress = strdup(line->ipAddress);
    }else{
        *ipAddress = NULL;
    }
}

void getManagerHostname(char** hostname){
    table_line* line;
    if(hasManager){
        findManagerLine(&line);
        *hostname = strdup(line->hostname);
    }else {
        *hostname = NULL;
    }
}

void findMACAddressByHostname(char* hostname, char** macAddress){
    table_line* line;
    findLineByHostname(hostname, &line);
    if(line != NULL){
        *macAddress = (char*) malloc((strlen(line->macAddress)+5)*sizeof(char));
        strcpy(*macAddress,line->macAddress);
    }else{
        *macAddress = NULL;
    }
}

void findIPAddressByHostname(char* hostname, char** ipAddress){
    table_line* line;
    findLineByHostname(hostname, &line);
    if(line != NULL){
        *ipAddress = strdup(line->ipAddress);
    }else{
        *ipAddress = NULL;
    }
}

void findPrevLineByHostname(char* hostname,table_line** line){
    table_line* iteratorLine;
    *line = NULL;
    if(!isTableEmpty()){
        iteratorLine = table;
        while (!isTheLastLine(iteratorLine)){
            if(strcmp(hostname, iteratorLine->nextLine->hostname) == 0 ){
                *line = iteratorLine;
                break;
            }
            iteratorLine = iteratorLine->nextLine;
        }
    }
}

void freeLine(table_line* line){
    free(line->hostname);
    free(line->ipAddress);
    free(line->macAddress);
    free(line->status);
    free(line);
}

void removeLineByHostname(char* hostname){
    table_line* line;
    table_line* prevLine;
    findLineByHostname(hostname, &line);
    if(line != NULL){
        if(line == table){
	        table = line->nextLine;
            freeLine(line);
        }else{
	        findPrevLineByHostname(hostname, &prevLine);
            if(!isTheLastLine(line)){
                prevLine->nextLine = line->nextLine;
            }else{
                prevLine->nextLine = NULL;
	        }
            freeLine(line);
        }
    }
}

void addLineToMarkedLines(table_line *line){
    table_line* iteratorLine;
    if(markedLines == NULL){
        markedLines = iteratorLine;
    } else{
        iteratorLine = markedLines;
        while(iteratorLine->nextLine != NULL){
            iteratorLine = iteratorLine->nextLine;
        }
        iteratorLine->nextLine = line;
        line->nextLine = NULL;
    }
}

void markLineToRemove(char* hostname){
    table_line* line;
    table_line* prevLine;
    findLineByHostname(hostname, &line);
    if(line != NULL){
        if(line == table){
            addLineToMarkedLines(line);
	    table = line->nextLine;
        }else{
	    findPrevLineByHostname(hostname, &prevLine);
            if(!isTheLastLine(line)){
                prevLine->nextLine = line->nextLine;
                line->nextLine = NULL;
            }else{
                prevLine->nextLine = NULL;
	    }
            addLineToMarkedLines(line);
        }
    }
}

void setManagerByHostname(char *hostname){
    table_line* line;
    if(hasManager()){
        removeManager();
    }
    findLineByHostname(hostname,&line);
    line->isManager = 1;
    line->status = strdup(AWAKEN);
}

void removeManager(){
    table_line* line;
    findManagerLine(&line);
    if(line != NULL){
        line->isManager = 0;
    }
}

void printLine(table_line* line){
    printf("%s        %s        %s        %s\n", line->hostname, line->macAddress, line->ipAddress, line->status);
}

void printMemberHeader(){
    printf("Members:\n");
    printf("Hostname           Endereco_MAC             Endereco_IP        Status\n");
}

void printMembers(){
    table_line* line;
    line = table;
    printMemberHeader();
    while(line != NULL){
        if(!line->isManager){
            printLine(line);
        }
        line = line->nextLine;
    }
}

void printManagerHeader(){
    printf("Manager:\n");
    printf("Hostname           Endereco_MAC             Endereco_IP        Status\n");
}

void printManager(){
    table_line* line;
    line = table;
    findManagerLine(&line);
    if(line != NULL){
        printManagerHeader();
        printLine(line);
    }
}

int updateMembersStatus(){
    table_line* line;
    int changedMembersCounter;
    line = table;
    changedMembersCounter = 0;
    while(line != NULL){
        if(!line->isManager){
            if((getTimestamp() - line->timestamp) < MAXIMUM_UPDATE_INTERVAL){
                if(strcmp(line->status,ASLEEP) == 0)
                    changedMembersCounter++;
                strcpy(line->status, AWAKEN);
            }else{
                if(strcmp(line->status,AWAKEN) == 0)
                    changedMembersCounter++;
                strcpy(line->status, ASLEEP);
            }
        }
        line = line->nextLine;
    }
    return changedMembersCounter > 0;
}

void updateTimestamp(char* hostname){
    table_line* line;
    findLineByHostname(hostname,&line);
    if(line != NULL){
        line->timestamp = getTimestamp();
    }
}

void addBufferData(replication_buffer *buffer){
    table_line *iteratorLine , *auxLine;
    if(markedLines != NULL){
        iteratorLine = markedLines;
        while(iteratorLine != NULL){
            auxLine = iteratorLine;
            addMarkedDataNodeToBuffer(buffer,iteratorLine->hostname,iteratorLine->macAddress,iteratorLine->ipAddress,iteratorLine->status);
            iteratorLine = iteratorLine->nextLine;
            freeLine(auxLine);
            auxLine = NULL;
        }
        markedLines = NULL;
    }
    if(!isTableEmpty()){
        iteratorLine = table;
        while (iteratorLine != NULL){
            addDataNodeToBuffer(buffer,iteratorLine->hostname,iteratorLine->macAddress,iteratorLine->ipAddress,iteratorLine->status);
            if(strcmp(iteratorLine->status,AWAKEN) == 0 && !iteratorLine->isManager){
                addMember(buffer, iteratorLine->ipAddress);
            }
            iteratorLine = iteratorLine->nextLine;
        }
    }
}

void updateBuffer(replication_buffer *buffer){
    table_line* iteratorLine, *auxLine;
    int dataChanged;
    if(markedLines != NULL){
        iteratorLine = markedLines;
        while(iteratorLine != NULL){
            auxLine = iteratorLine;
            dataChanged = addMarkedDataNodeToBuffer(buffer,iteratorLine->hostname,iteratorLine->macAddress,iteratorLine->ipAddress,iteratorLine->status);
            if(dataChanged){
                removeAck(buffer,iteratorLine->hostname);
            }
	    if(hasMember(buffer,iteratorLine->ipAddress)){
		removeMember(buffer, iteratorLine->ipAddress);
	    }
            iteratorLine = iteratorLine->nextLine;
            freeLine(auxLine);
            auxLine = NULL;
        }
        markedLines = NULL;
    }
    if(!isTableEmpty()){
        iteratorLine = table;
        while (iteratorLine != NULL){
            dataChanged = addDataNodeToBuffer(buffer,iteratorLine->hostname,iteratorLine->macAddress,iteratorLine->ipAddress,iteratorLine->status);
            if(dataChanged){
                removeAck(buffer,iteratorLine->hostname);
            }
            if(strcmp(iteratorLine->status,AWAKEN) == 0 && !iteratorLine->isManager){
                if(!hasMember(buffer,iteratorLine->ipAddress)){
                    addMember(buffer, iteratorLine->ipAddress);
                }
            }else{
                if(hasMember(buffer,iteratorLine->ipAddress)){
                    removeMember(buffer, iteratorLine->ipAddress);
                }
            }
            iteratorLine = iteratorLine->nextLine;
        }
    }
}

void getMemberIpListOfLTIdMembers(member_ip_list** list, char *hostname){
    table_line* iteratorLine;
    createMemberIpList(list);
    if(!isTableEmpty()){
        iteratorLine = table;
        while (iteratorLine != NULL){
            if(strcmp(hostname,iteratorLine->hostname) > 0){
                addMemberIpToList(*list,iteratorLine->ipAddress);
            }
            iteratorLine = iteratorLine->nextLine;
        }
    }
}

void getMemberIpListOfGTIdMembers(member_ip_list** list, char *hostname){
    table_line* iteratorLine;
    createMemberIpList(list);
    if(!isTableEmpty()){
        iteratorLine = table;
        while (iteratorLine != NULL){
            if(strcmp(hostname,iteratorLine->hostname) < 0){
                addMemberIpToList(*list,iteratorLine->ipAddress);
            }
            iteratorLine = iteratorLine->nextLine;
        }
    }
}
