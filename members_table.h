#define AWAKEN "awaken"
#define ASLEEP "ASLEEP"

void addLine(char *hostname, char *macAddress, char *ipAddress, char *status);
void removeLineByHostname(char* hostname);
int isHostnameInTheTable(char* hostname);
void findMACAddressByHostname(char* hostname, char** macAddress);
void findIPAddressByHostname(char* hostname, char** ipAddress);
int updateMembersStatus();
void updateTimestamp(char* hostname);
void printTable();