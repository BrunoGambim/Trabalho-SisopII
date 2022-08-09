typedef struct _manager_data{
    char *hostname;
    char *macAddress;
    char *ipAddress;
} manager_data;

int managerDataIsNull();
void getManagerIPAddress(char** ipAdress);
void createManagerData(char *hostname, char *macAddress, char *ipAddress, manager_data** data);
int updateManagerData(char *hostname, char *macAddress, char *ipAddress);
void printManagerData();
void freeManagerData(manager_data* data);