#include "custom_mutex.h"

void runDiscoverySubservice(custom_mutex *mutex, void *(*runAsMem)());
void changeDiscoverySubserviceToMember();
void changeDiscoverySubserviceToManager();
void stopDiscoverySubservice();