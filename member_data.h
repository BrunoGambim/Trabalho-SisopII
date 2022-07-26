typedef struct _member_data{
    char *hostname;
    char *macAddress;
    char *ipAddress;
    char *status;
} member_data;

void createMemberData(char *hostname, char *macAddress, char *ipAddress, char *status, member_data** data);
void freeMemberData(member_data* data);