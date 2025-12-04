#include "pru.h"
#include <stdio.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/mman.h>
#include <string.h>

#define PRU_SHARED_MEM_PHYS 0x4A310000
#define PRU_SHARED_MEM_SIZE 0x3000 // = 12kB
#define MAP_MASK (PRU_SHARED_MEM_SIZE - 1)

void* initPRUSharedMem(int *memFd)
{
    *memFd = open("/dev/mem", O_RDWR | O_SYNC);
    if (*memFd < 0)
	{
        printf("[ERROR] Kon /dev/mem niet openen.\n");
		fflush(stdout);
        return NULL;
    }
	
    void *pruSharedMemPointer = mmap(NULL, PRU_SHARED_MEM_SIZE, PROT_READ | PROT_WRITE, MAP_SHARED, *memFd, PRU_SHARED_MEM_PHYS & ~MAP_MASK);
    if (pruSharedMemPointer == MAP_FAILED)
	{
        printf("[ERROR] mmap is gefaald.\n");
		fflush(stdout);
        close(*memFd);
        return NULL;
    }
    return pruSharedMemPointer;
}

void cleanupPRU(void *pruSharedMemPointer, int memFd)
{
    munmap(pruSharedMemPointer, PRU_SHARED_MEM_SIZE);
    close(memFd);
}

bool isPRURunning()
{
    char state[64];
    FILE *pruStateFile = fopen("/sys/class/remoteproc/remoteproc1/state", "r");
	
    if (pruStateFile == NULL)
	{
		printf("[ERROR] Kan PRU state file niet openen.\n");
		fflush(stdout);
        return false;
    }
	
    if (fgets(state, sizeof(state), pruStateFile) == NULL)
	{
		printf("[ERROR] Kan PRU state niet lezen.\n");
		fflush(stdout);
        fclose(pruStateFile);
        return false;
    }
	
    fclose(pruStateFile);
	
    state[strcspn(state, "\r\n")] = '\0';
	
    return strcmp(state, "running") == 0;
}
