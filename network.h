#define DISCOVERY_BROADCAST_PORT     8080
#define DISCOVERY_PORT     8081
#define UPDATE_PORT     8082
#define EXIT_PORT     8083
#define REPLICATION_PORT     8084
#define ELECTION_PORT     8085
#define MAGIC_PACKAGE_PORT     7
#define PAYLOAD_SIZE 500
#define ELECTION "1"
#define ANSWER "2"
#define COORDINATOR "3"

typedef struct _package{
    char payload[PAYLOAD_SIZE]; 
} package;

void sendPackage(package* package, int port, char* ipAddress);
void serve(package** pack, char** ipAddress, int sockfd);
int  createSocket(int port, char* ipAddress);
void closeSocket(int socket);
void createDataPackage(package** pack, char *hostname, char *macAddress, char *ipAddress, char *status);
void createDiscoveryPackage(package** pack, char *hostname, char *macAddress);
void createHostnamePackage(package** pack, char *hostname);
void createEmptyPackage(package** pack);
void createMagicPackage(package** pack, char *macAddress);
void createElectionPackage(package** pack);
void createAnswerPackage(package** pack);
void createCoordinatorPackage(package** pack);
void unpackElectionPackage(package* pack, char **payload);
void unpackDataPackage(package* pack, char **hostname, char **macAddress, char **ipAddress, char **status);
void unpackDiscoveryPackage(package* pack, char **hostname, char **macAddress);
void unpackHostnamePackage(package* pack, char **hostname);
void freePackage(package* pack);
void getMACAddress(char** macAddress);
void getIPAddress(char** ipAddress);
void getHostname(char** hostname);
void getBroadcastIPAddress(char** ipAddress);
void initNetworkGlobalVariables();