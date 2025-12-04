#ifndef PRU_H
#define PRU_H

#include <stdbool.h>

void* initPRUSharedMem(int *memFd);
void cleanupPRU(void *pruSharedMemPointer, int memFd);
bool isPRURunning();

#endif // PRU_H
