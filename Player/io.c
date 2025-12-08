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
	
	printf("[INFO] GPIO%d waarde is %c.\n", gpioNr, value);
	fflush(stdout);
	
	// The value we get here is the assci char code. '0' is decimal 48, '1' is decimal 49
	if (value == '0')
		value = 0;
	else if (value == '1')
		value = 1;
	else
		printf("[WARNING] GPIO%d value %c is not recognized, assuming logic level 1.\n", gpioNr, value);
	
	return value;
}