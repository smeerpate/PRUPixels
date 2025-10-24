; vraag status van PRU0: cat /sys/class/remoteproc/remoteproc1/state
; Start PRU0: echo 'start' > /sys/class/remoteproc/remoteproc1/state
; kijk naar registers van PRU0: sudo cat /sys/kernel/debug/remoteproc/remoteproc1/regs
; pscp "D:\Users\Twenty Three BVBA\Documents\C_C++\PRUSS_WSdriver\*.*" debian@BeagleBone:"/home/debian/pruWSDriver"
; compileren en linken: make
; laten lopen op PRU0: sudo make install_PRU0
; https://www.ti.com/lit/ug/spruhv7c/spruhv7c.pdf?ts=1757487139856&ref_url=https%253A%252F%252Fsoftware-dl.ti.com%252Fprocessor-sdk-linux-rt%252Fesd%252FAM64X%252F11_00_09_04%252Fexports%252Fdocs%252Fcommon%252FPRU-ICSS%252FPRU-Getting-Started-Labs_Lab2_mixedCandAssembly.html
; Sectie 6.6.1 -- somige registers best niet gebruikenÒ
; stack pointer zit op r2
; Save-on-entry registers (R3.w2-R13)

    .cdecls "main.c"
    .clink
    .global MBI5124BangBits
    .asg 32, NSLICES

    sliceCounter .equ r5

MBI5124BangBits:
    LDI32 r0, 0x00010000 ; Initializeer r0, aka. de geheugen pointer
    LDI32 r4, 0x00000110 ; sla de inhoud van nLEDs op in r4
    LBBO &r1, r4, 0, 4 ; Initializeer r1, aka. de teller voor het aantal nog te vewerken LEDs
    LDI32 sliceCounter, 0x00000000 ; Initializeer r5, aka. de slice counter

NEXTLED:

SETRDATA:

CLRRDATA:

SETGDATA:

CLRGDATA:
