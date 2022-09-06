#include "replication_buffer.h"

#define AWAKEN "awaken"
#define ASLEEP "ASLEEP"

void addLine(char *hostname, char *macAddress, char *ipAddress, char *status);
void removeLineByHostname(char* hostname);
int isHostnameInTheTable(char* hostname);
void findMACAddressByHostname(char* hostname, char** macAddress);
void findIPAddressByHostname(char* hostname, char** ipAddress);
void getManagerIPAddress(char** ipAddress);
int updateMembersStatus();
void updateTimestamp(char* hostname);
void setManagerByHostname(char *hostname);
int hasManager();
void updateBuffer(replication_buffer *buffer);
void removeManager();
void printMembers();
void printManager();
void addBufferData(replication_buffer *buffer);
void updateBufferMembers(replication_buffer *buffer);