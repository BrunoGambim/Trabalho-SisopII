#include "custom_mutex.h"

void runExitSubservice(custom_mutex *mutex, void *(*func)());
void changeExitSubserviceToMember();
void changeExitSubserviceToManager();
void stopExitSubservice();
void finishProcess();