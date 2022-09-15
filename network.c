#include <stdio.h> 
#include <stdlib.h> 
#include <unistd.h> 
#include <string.h> 
#include <sys/types.h> 
#include <sys/socket.h> 
#include <arpa/inet.h> 
#include <netinet/in.h> 
#include <ifaddrs.h>
#include <netpacket/packet.h>
#include <netdb.h>
#include "network.h"

#define BUFFER_SIZE 50
#define EMPTY_PACKAGE 0

#define SEARCHING_HOSTNAME 0
#define SEARCHING_MAC_ADRESS 1
#define SEARCHING_IP_ADRESS 2
#define SEARCHING_STATUS 3
#define SEARCH_FINISHED 4
#define LOCAL "lo"

char* localMACAddress;
char* localIPAddress;
char* broadcastIPAddress;
char* localHostname;
    
void sendPackage(package* package, int port, char* ipAddress) { 
    int sockfd; 
    int broadcastPermission;
    struct sockaddr_in servaddr; 
    
    if ( (sockfd = socket(AF_INET, SOCK_DGRAM, 0)) < 0 ) { 
        perror("socket creation failed"); 
        exit(EXIT_FAILURE); 
    } 

    broadcastPermission = 1;
    if (setsockopt(sockfd, SOL_SOCKET, SO_BROADCAST, (void *) &broadcastPermission, 
          sizeof(broadcastPermission)) < 0){ 
        perror("socket creation failed"); 
        exit(EXIT_FAILURE); 
    }
    
    memset(&servaddr, 0, sizeof(servaddr)); 
         
    servaddr.sin_family = AF_INET; 
    servaddr.sin_port = htons(port); 
    servaddr.sin_addr.s_addr =  inet_addr(ipAddress);
        
    sendto(sockfd, package, sizeof(struct _package), 
        MSG_CONFIRM, (const struct sockaddr *) &servaddr,  
            sizeof(servaddr)); 
    
    close(sockfd); 
}

void serve(package** pack, char** ipAddress, int sockfd) { 
    int len;
    package* recvPackage;
    char *clientIpAddress;
    struct sockaddr_in cliaddr;

    memset(&cliaddr, 0, sizeof(cliaddr)); 
        
    len = sizeof(cliaddr);
    
    recvPackage = (package*) malloc(sizeof(package));
    recvfrom(sockfd, recvPackage, sizeof(struct _package),  
                MSG_WAITALL, ( struct sockaddr *) &cliaddr, 
                &len);

    if(ipAddress != NULL){
        clientIpAddress = inet_ntoa(cliaddr.sin_addr);
        *ipAddress = strdup(clientIpAddress);
    }

    *pack = recvPackage;
}

int createSocket(int port, char* ipAddress){
    int sockfd;
    struct sockaddr_in servaddr;

    if ( (sockfd = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP)) < 0 ) { 
        perror("socket creation failed"); 
        exit(EXIT_FAILURE); 
    } 
    
    memset(&servaddr, 0, sizeof(servaddr)); 
        
    servaddr.sin_family    = AF_INET;
    servaddr.sin_addr.s_addr =  inet_addr(ipAddress);
    servaddr.sin_port = htons(port); 

    if ( bind(sockfd, (const struct sockaddr *)&servaddr,  sizeof(servaddr)) < 0 ) { 
        perror("bind failed"); 
        exit(EXIT_FAILURE); 
    } 
    return sockfd;
}

void closeSocket(int socket){
    close(socket);
}

void freePackage(package* pack){
    free(pack);
}

void createDataPackage(package** pack, char *hostname, char *macAddress, char *ipAddress, char *status){
    package* newPackage;
    newPackage = (package*) malloc(sizeof(package));

    newPackage->payload[0] = 0;
    strcat(newPackage->payload,hostname);
    strcat(newPackage->payload,"\n");
    strcat(newPackage->payload,macAddress);
    strcat(newPackage->payload,"\n");
    strcat(newPackage->payload,ipAddress);
    strcat(newPackage->payload,"\n");
    strcat(newPackage->payload,status);
    strcat(newPackage->payload,"\n");

    *pack = newPackage;
}

void createDiscoveryPackage(package** pack, char *hostname, char *macAddress){
    package* newPackage;
    newPackage = (package*) malloc(sizeof(package));

    newPackage->payload[0] = 0;
    strcat(newPackage->payload,hostname);
    strcat(newPackage->payload,"\n");
    strcat(newPackage->payload,macAddress);
    strcat(newPackage->payload,"\n");

    *pack = newPackage;
}

void createHostnamePackage(package** pack, char *hostname){
    package* newPackage;
    newPackage = (package*) malloc(sizeof(package));

    newPackage->payload[0] = 0;
    strcat(newPackage->payload,hostname);

    *pack = newPackage;
}

void createEmptyPackage(package** pack){
    *pack = (package*) malloc(sizeof(package));
}



void createMagicPackage(package** pack, char *macAddress){
    package* newPackage;
    char mac[6], buffer[5];
    int i;
    newPackage = (package*) malloc(sizeof(package));
    for(i = 0; i < 6;i++){
        newPackage->payload[i] = 255;
        buffer[0] = *(macAddress+(i*3));
        buffer[1] = *(macAddress+(i*3)+1);
        buffer[2] = 0;
        mac[i] = (int)strtol(buffer, NULL, 16);
    }

    for(i = 1; i <= 16; i++){
		memcpy(&newPackage->payload[i * 6], &mac, 6 * sizeof(char));
	}

    *pack = newPackage;
}

void createElectionPackage(package** pack){
    package* newPackage;
    newPackage = (package*) malloc(sizeof(package));

    newPackage->payload[0] = 0;
    strcat(newPackage->payload,ELECTION);

    *pack = newPackage;
}

void createAnswerPackage(package** pack){
    package* newPackage;
    newPackage = (package*) malloc(sizeof(package));

    newPackage->payload[0] = 0;
    strcat(newPackage->payload,ANSWER);

    *pack = newPackage;
}

void createCoordinatorPackage(package** pack){
    package* newPackage;
    newPackage = (package*) malloc(sizeof(package));

    newPackage->payload[0] = 0;
    strcat(newPackage->payload,COORDINATOR);

    *pack = newPackage;
}

void unpackElectionPackage(package* pack, char **payload){
    *payload = strdup(pack->payload);
    freePackage(pack);
}

void unpackDiscoveryPackage(package* pack, char **hostname, char **macAddress){
    char buffer[BUFFER_SIZE];
    int i,bufferIndex,searching_flag;

    bufferIndex = 0;
    searching_flag = SEARCHING_HOSTNAME;
    for(i = 0; pack->payload[i] != 0; i++){
        if(pack->payload[i] != '\n'){
            buffer[bufferIndex] = pack->payload[i];
        }else{
            buffer[bufferIndex] = 0;
            if(searching_flag == SEARCHING_HOSTNAME){
                *hostname = strdup(buffer);
                searching_flag = SEARCHING_MAC_ADRESS;
            }else if(searching_flag == SEARCHING_MAC_ADRESS){
                *macAddress = strdup(buffer);
                searching_flag = SEARCH_FINISHED;
            }
            bufferIndex = -1;
        }
        bufferIndex++;
    }
    freePackage(pack);
}

void unpackDataPackage(package* pack, char **hostname, char **macAddress, char **ipAddress, char **status){
    char buffer[BUFFER_SIZE];
    int i,bufferIndex,searching_flag;
    bufferIndex = 0;
    searching_flag = SEARCHING_HOSTNAME;
    for(i = 0; pack->payload[i] != 0; i++){
        if(pack->payload[i] != '\n'){
            buffer[bufferIndex] = pack->payload[i];
        }else{
            buffer[bufferIndex] = 0;
            if(searching_flag == SEARCHING_HOSTNAME){
                *hostname = strdup(buffer);
                searching_flag = SEARCHING_MAC_ADRESS;
            }else if(searching_flag == SEARCHING_MAC_ADRESS){
                *macAddress = strdup(buffer);
                searching_flag = SEARCHING_IP_ADRESS;
            }else if(searching_flag == SEARCHING_IP_ADRESS){
                *ipAddress = strdup(buffer);
                searching_flag = SEARCHING_STATUS;
            }else if(searching_flag == SEARCHING_STATUS){
                *status = strdup(buffer);
                searching_flag = SEARCH_FINISHED;
            }
            bufferIndex = -1;
        }
        bufferIndex++;
    }
    freePackage(pack);
}

void unpackHostnamePackage(package* pack, char **hostname){
    *hostname = strdup(pack->payload);
    freePackage(pack);
}

void getIPAddressFromIfaddres(char** ipAddress){
    struct ifaddrs *ifaddr, *ifa;
    int family, s;
    char host[NI_MAXHOST];

    if (getifaddrs(&ifaddr) == -1) {
        perror("getifaddrs");
        exit(EXIT_FAILURE);
    }


    for (ifa = ifaddr; ifa != NULL; ifa = ifa->ifa_next) 
    {
        if (ifa->ifa_addr == NULL)
            continue;  

        if((strcmp(ifa->ifa_name,LOCAL) != 0)&&(ifa->ifa_addr->sa_family==AF_INET)){
            s=getnameinfo(ifa->ifa_addr,sizeof(struct sockaddr_in),host, NI_MAXHOST, NULL, 0, NI_NUMERICHOST);
            if (s != 0){
                printf("getnameinfo() failed: %s\n", gai_strerror(s));
                exit(EXIT_FAILURE);
            }
            *ipAddress = strdup(host);
            break;
        }
    }

    freeifaddrs(ifaddr);
}

void getNetmaskFromIfaddres(char** netmask){
    struct ifaddrs *ifaddr, *ifa;

    struct sockaddr_in *sa;
    char *addr;

    if (getifaddrs(&ifaddr) == -1) {
        perror("getifaddrs");
        exit(EXIT_FAILURE);
    }


    for (ifa = ifaddr; ifa != NULL; ifa = ifa->ifa_next) 
    {
        if (ifa->ifa_addr == NULL)
            continue;  

        if((strcmp(ifa->ifa_name,LOCAL) != 0)&&(ifa->ifa_addr->sa_family==AF_INET)){
            sa = (struct sockaddr_in *) ifa->ifa_netmask;
            addr = inet_ntoa(sa->sin_addr);

            *netmask = strdup(addr);
            break;
        }
    }

    freeifaddrs(ifaddr);
}

void getIPAddress(char** ipAddress){
    *ipAddress = localIPAddress;
}

void getHostnameFromLibs(char** hostname){
    char buffer[1024];
    buffer[1023] = '\0';
    gethostname(buffer, 1023);
    *hostname = strdup(buffer);
}

void getHostname(char** hostname){
    *hostname = localHostname;
}

void  getMACAddressFromIfaddrs(char** macAddress){
    struct ifaddrs *ifaddr=NULL;
    struct ifaddrs *ifa = NULL;
    struct sockaddr_ll *s;
    char buffer[25], buffer2[10];
    int i = 0;
    buffer[0] = 0;

    if (getifaddrs(&ifaddr) == -1){
         perror("getifaddrs");
    }
    else{
         for ( ifa = ifaddr; ifa != NULL; ifa = ifa->ifa_next)
         {
             if ( (ifa->ifa_addr) && (ifa->ifa_addr->sa_family == AF_PACKET) )
             {
                  s = (struct sockaddr_ll*)ifa->ifa_addr;
                  if(strcmp(ifa->ifa_name,LOCAL) != 0){
                    for (i=0; i <s->sll_halen; i++)
                    {
                        sprintf(buffer2,"%02x%c", (s->sll_addr[i]), (i+1!=s->sll_halen)?':':0);
                        strcat(buffer,buffer2);
                    }
                    break;
                  }
             }
         }
         freeifaddrs(ifaddr);
         *macAddress = strdup(buffer);
    }
}

void getMACAddress(char** macAddress){
    *macAddress = localMACAddress;
}

void calcBroadcastIP(char** ipAddress){
    char *host_ip;
    char *netmask;
    getIPAddress(&host_ip);
    getNetmaskFromIfaddres(&netmask);

    struct in_addr host, mask, broadcast;
    char broadcast_address[INET_ADDRSTRLEN];
    if (inet_pton(AF_INET, host_ip, &host) == 1 &&
        inet_pton(AF_INET, netmask, &mask) == 1)
        broadcast.s_addr = host.s_addr | ~mask.s_addr;
    else {
        fprintf(stderr, "Failed converting strings to numbers\n");
        exit(EXIT_FAILURE);
    }
    if (inet_ntop(AF_INET, &broadcast, broadcast_address, INET_ADDRSTRLEN) != NULL){
        *ipAddress = strdup(broadcast_address);
    }
    else {
        fprintf(stderr, "Failed converting number to string\n");
        exit(EXIT_FAILURE);
    }
}

void getBroadcastIPAddress(char** ipAddress){
    *ipAddress = broadcastIPAddress;
}

void initNetworkGlobalVariables(){
    getIPAddressFromIfaddres(&localIPAddress);
    getHostnameFromLibs(&localHostname);
    getMACAddressFromIfaddrs(&localMACAddress);
    calcBroadcastIP(&broadcastIPAddress);
}