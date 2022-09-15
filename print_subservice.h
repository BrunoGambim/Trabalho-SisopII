#include "custom_mutex.h"
#include <pthread.h>
#include <semaphore.h>

void runPrintSubservice(custom_mutex *mutex);
void stopPrintSubservice();