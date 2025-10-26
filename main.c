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

    // LED data
    shared[0] = 0x00000008;
    shared[1] = 0x00000800;
    shared[2] = 0x00000030;
    shared[3] = 0x0000FF00;
    shared[4] = 0x000000FF;
    shared[5] = 0x00000100;
    shared[6] = 0x00000000;
    shared[7] = 0x00000000;
    shared[8] = 0x00000000;
    shared[9] = 0x00000000;
    shared[10] = 0x00000000;
    shared[11] = 0x00000000;
    shared[12] = 0x00000000;
    shared[13] = 0x00000000;
    shared[14] = 0x00000000;
    shared[15] = 0x00000000;
    shared[16] = 0x00000000;
    shared[17] = 0x00000000;
    shared[18] = 0x00000000;
    shared[19] = 0x00000000;
    shared[20] = 0x00000000;
    shared[21] = 0x00000000;
    shared[22] = 0x00000000;
    shared[23] = 0x00000000;
    shared[24] = 0x00000000;
    shared[25] = 0x00000000;
    shared[26] = 0x00000000;
    shared[27] = 0x00000000;
    shared[28] = 0x00000000;
    shared[29] = 0x00000000;
    shared[30] = 0x00000000;
    shared[31] = 0x00000000;
    shared[32] = 0x00000000;
    shared[33] = 0x00000000;
    shared[34] = 0x00000000;
    shared[35] = 0x00000000;
    shared[36] = 0x00000000;
    shared[37] = 0x00000000;
    shared[38] = 0x00000000;
    shared[39] = 0x00000000;
    shared[40] = 0x00000000;
	shared[41] = 0x00000000;
    shared[42] = 0x00000000;
    shared[43] = 0x00000000;
    shared[44] = 0x00000000;
    shared[45] = 0x00000000;
    shared[46] = 0x00000000;
    shared[47] = 0x00000000;
    shared[48] = 0x00000000;
    shared[49] = 0x00000000;
    shared[50] = 0x00000000;
    shared[51] = 0x00000000;
    shared[52] = 0x00000000;
    shared[53] = 0x00000000;
    shared[54] = 0x00000000;
    shared[55] = 0x00000000;
    shared[56] = 0x00000000;
    shared[57] = 0x00000000;
    shared[58] = 0x00000000;
    shared[59] = 0x00000000;
    shared[60] = 0x00000000;
    shared[61] = 0x00000000;
    shared[62] = 0x00000000;
    shared[63] = 0x00000F00;
    shared[64] = 0x00000008;
    shared[127] = 0x00000000;
	shared[383] = 0x00000000;
	
	void *memset((void *)shared, 0x00000008, 128*sizeof(uint32_t)); // top 2 IMS, 128 pixels

	
    while(1)
	{
		MBI5124BangBits(); // start de ASM code
		__delay_cycles(500000); // 5000000 is ongever 25ms
	}
}
