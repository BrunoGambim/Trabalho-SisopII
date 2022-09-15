#include "custom_mutex.h"

void runElectionSubservice(custom_mutex *mutex, void *(*runAsMana)(), void *(*runAsMem)());
void startElection();
void stopElectionSubservice();