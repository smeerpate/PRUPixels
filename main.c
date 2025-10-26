// Schrijft naar pin P9_27. Gebruik config-pin P9_27 pruout
// vraag status van PRU0: cat /sys/class/remoteproc/remoteproc1/state
// Start PRU0: echo 'start' > /sys/class/remoteproc/remoteproc1/state
// Stop PRU0: echo 'stop' > /sys/class/remoteproc/remoteproc1/state
// kijk naar registers van PRU0: sudo cat /sys/kernel/debug/remoteproc/remoteproc1/regs
// pscp "D:\Users\Twenty Three BVBA\Documents\C_C++\PRUSS_WSdriver\*.*" debian@BeagleBone:"/home/debian/pruWSDriver"  
// compileren en linken: make
// laten lopen op PRU0: sudo make install_PRU0

// Zie ook https://git.ti.com/cgit/pru-software-support-package/pru-software-support-package/tree/examples/am243x/MyFirst_PRU_mixed_Program

#include <stdint.h>
#include <pru_cfg.h>
#include "resource_table_empty.h"

#define PRU0_DRAM	0x00000000
#define PRU1_DRAM	0x00002000
#define SHARE_MEM	0x00010000

#define nLEDs (*((volatile unsigned int *)0x00000110))
#define nBitsPerLED (*((volatile unsigned int *)0x00000114))

volatile uint32_t *pru0Mem = (unsigned int *) PRU0_DRAM;
volatile uint32_t *pru1Mem = (unsigned int *) PRU1_DRAM;
volatile uint32_t *shared = (unsigned int *) SHARE_MEM;

extern void MBI5124BangBits(void); // niet hier gedefinieerd maar wel in de asm

void main(void)
{
	nLEDs = 1200;
	nBitsPerLED = 32;
	uint32_t brightness = 8;

    // LED data
    shared[0] = 0x00000008;
    shared[1] = 0x00000800;
	
	// top 2 IMs
	uint32_t i;
	for (i = 0; i < 128; i++)
	{
		shared[i] = (brightness && 0xFF) << 8;
	}
/*	
	// Middle 2 IMs
	for (i = 128; i < 256; i++)
	{
		shared[i] = (brightness && 0xFF) << 8;
	}

	// Bottom 2 IMs
	for (i = 256; i < 384; i++)
	{
		shared[i] = 0x00000008;
	}
*/	
    while(1)
	{
		MBI5124BangBits(); // start de ASM code
		__delay_cycles(500000); // 5000000 is ongever 25ms
	}
}
