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
    .asg 0x00010000, PIXELBUFFERSTART
    .asg 255, NSLICES
    .asg 128, NPIXELS

;   r0: buffer start pointer
;   r1: huidige bufferwaarde
;   r4: pixel counter
;   r5: slice counter
;   r6: buffer pointer
;   r7: hulp bij compares

MBI5124BangBits:
    LDI32 r0, PIXELBUFFERSTART ; Initializeer r0, aka. de geheugen pointer
    LDI32 r4, 0x00000000 ; Initializeer r4, aka. de pixel counter
    LDI32 r5, 0x00000000 ; Initializeer r5, aka. de slice counter

PROCESSR:
    ; rode data verwerken
    ADD r6, r0, r5 ; r6=r0+r5, update de buffer pointer
    LBBO &r1, r6, 0, 1 ; haal de rode waarde op van de huidige buffer pointer (=r6) en steek in r1
    QBGE SDOR_HI, r5, r1 ; spring als r1(pixelwaarde) >= r5(slicecounter)
SDOR_LOW:
    CLR r30, r30.t0
    JMP PROCESSG
SDOR_HI:
    SET r30, r30.t0
    
PROCESSG:
    ; groene data verwerken
    LBBO &r1, r6, 1, 1 ; haal de groene waarde op van de huidige buffer pointer (=r6) en steek in r1
    QBGE SDOG_HI, r5, r1 ; spring als r1(pixelwaarde) >= r5(slicecounter)
SDOG_LOW:
    CLR r30, r30.t1
    JMP RISECLK
SDOG_HI:
    SET r30, r30.t1

RISECLK:
    SET r30, r30.t3 ; zet CLK op 1
    ADD r4, r4, 1 ; doe de pixelteller +1
    ; zijn er nog pixels?
    QBLE NEXTSLICE, r4, NPIXELS ; spring als NPIXELS <= r4(pixel counter)
    CLR r30, r30.t3 ; zet CLK terug op 0
    JMP PROCESSR
    
NEXTSLICE:
    LDI32 r4, 0x00000000 ; Initializeer r4, aka. de pixel counter
    CLR r30, r30.t3 ; zet CLK terug op 0
    LDI32 r7, NSLICES ; bereid voor om compare te doen
    SET r30, r30.t5 ; zet LE op 1
    QBLT INCSLICECOUNTER, r7, r5
    LDI32 r5, 0x00000000 ; Initializeer r5, aka. de slice counter
    JMP CLRLE
    
INCSLICECOUNTER:
    ADD r5, r5, 1 ; doe de slicecounter +1

CLRLE:
    CLR r30, r30.t5 ; zet LE op 0
    JMP PROCESSR