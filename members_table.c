#include <string.h>
#include <stdio.h>
#include <stdlib.h> 
#include <unistd.h>
#include <time.h>
#include "members_table.h"

typedef struct table_lines{
    char *hostname;
    char *macAddress;
    char *ipAddress;
    char *status;
    unsigned int timestamp;
    struct table_lines* nextLine;
} table_line;

table_line *table;

int isTableEmpty(){
    return table == NULL;
}

int isTheLastLine(table_line* line){
    return line->nextLine == NULL;
}

unsigned int getTimestamp(){
    return (unsigned)time(NULL);;
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
        *ipAddress = (char*) malloc((strlen(line->ipAddress)+5)*sizeof(char));
        strcpy(*ipAddress,line->ipAddress);
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

void printLine(table_line* line){
    printf("%s        %s        %s        %s\n", line->hostname, line->macAddress, line->ipAddress, line->status);
}

void printTableHeader(){
    printf("Hostname           Endereco_MAC             Endereco_IP        Status\n");
}

void printTable(){
    table_line* line;
    line = table;
    printTableHeader();
    while(line != NULL){
        printLine(line);
        line = line->nextLine;
    }
}

int updateMembersStatus(){
    table_line* line;
    int changedMembersCounter;
    line = table;
    changedMembersCounter = 0;
    while(line != NULL){
        if((getTimestamp() - line->timestamp) < 3){
            if(strcmp(line->status,ASLEEP) == 0)
                changedMembersCounter++;
            strcpy(line->status,AWAKEN);
        }else{
            if(strcmp(line->status,AWAKEN) == 0)
                changedMembersCounter++;
            strcpy(line->status,ASLEEP);
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
