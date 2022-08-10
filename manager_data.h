typedef struct _manager_data{
    char *hostname;
    char *macAddress;
    char *ipAddress;
} manager_data;

int managerDataIsNull();
void getManagerIPAddress(char** ipAdress);
int updateManagerData(char *hostname, char *macAddress, char *ipAddress);
void printManagerData();