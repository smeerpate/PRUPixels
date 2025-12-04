#include "io.h"
#include <stdio.h>

void setGPIODirection(int gpioNr, int direction) // direction 0 is output, !=0 is input
{
	char path[64];
	
	sprintf(path, "/sys/class/gpio/gpio%d/direction", gpioNr);
	
	FILE *fp = fopen(path, "w");
    if (fp == NULL)
	{
        printf("[ERROR] kon file %s niet openen om driection te schrijven.\n", path);
		fflush(stdout);
        return;
    }
	
	if (direction == 0)
		fprintf(fp, "out");
	else
		fprintf(fp, "in");
	
    fclose(fp);
    return;
}

char readGPIO(int gpioNr)
{
	char path[64];
	char value;
	
	sprintf(path, "/sys/class/gpio/gpio%d/value", gpioNr);
	
	FILE *fp = fopen(path, "r");
	if (fp == NULL)
	{
        printf("[ERROR] kon file %s niet openen om input waarde te lezen.\n", path);
		fflush(stdout);
        return 0;
    }
	fread(&value, 1, 1, fp);
    fclose(fp);
	
	printf("[INFO] GPIO%d waarde is %d.\n", gpioNr, value);
	fflush(stdout);
	
	return value;
}