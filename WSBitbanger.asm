; Schrijft naar pin P9_27. Gebruik config-pin P9_27 pruout
; vraag status van PRU0: cat /sys/class/remoteproc/remoteproc1/state
; Start PRU0: echo 'start' > /sys/class/remoteproc/remoteproc1/state
; kijk naar registers van PRU0: sudo cat /sys/kernel/debug/remoteproc/remoteproc1/regs
; pscp "D:\Users\Twenty Three BVBA\Documents\C_C++\PRUSS_WSdriver\*.*" debian@BeagleBone:"/home/debian/pruWSDriver"  
; compileren en linken: make
; laten lopen op PRU0: sudo make install_PRU0

; https://www.ti.com/lit/ug/spruhv7c/spruhv7c.pdf?ts=1757487139856&ref_url=https%253A%252F%252Fsoftware-dl.ti.com%252Fprocessor-sdk-linux-rt%252Fesd%252FAM64X%252F11_00_09_04%252Fexports%252Fdocs%252Fcommon%252FPRU-ICSS%252FPRU-Getting-Started-Labs_Lab2_mixedCandAssembly.html
; Sectie 6.6.1
; stack pointer zit op r2
; Save-on-entry registers (R3.w2-R13)
; 
    .cdecls "main.c"
    .clink
    .global bangBits
    .asg 60, T1H ; 1-bit fase 1
	.asg 60, T1L ; 1-bit fase 2
	.asg 30, T0H ; 0-bit fase 1
	.asg 89, T0L ; 0-bit fase 2

bangBits:
    LDI32 r0, 0x00010000 ; Initializeer de geheugen pointer
	LDI32 r4, 0x00000110 ; sla de inhoud van nLEDs op in r4
    LBBO &r1, r4, 0, 4 ; Initializeer de teller voor het aantal nog te vewerken LEDs
    
NEXTLED:
    SUB r1, r1, 1 ; r1 bevat het nog te verwerken bits
    LBBO &r2, r0, 0, 4 ; r2 bevat nu de kleurdata
	LDI32 r4, 0x00000114 ; sla het adres van nBitsPerLED op in r4
    LBBO &r5, r4, 0, 4; in r5, zet aantal te schuiven bits klaar
	;LDI32 r5, 32 ; sla het adres van NBITS op in r
    
SENDBIT:
    SUB r5, r5, 1; de index start van nul, dus trek 1 af
    QBBC ZEROBIT, r2, r5 ; test de huidige bit (bitnummer in r5) van de LED data in r2

ONEBIT:
    ; '1' bit: hoog voor ~600ns, laag voor ~600
    SET r30, r30.t5 ; zet uitgang P9_27 op 1
	LDI32 r4, T1H ; laad teller met T1H
DELAY_T1H:
    SUB r4, r4, 1
    QBNE DELAY_T1H, r4, 0 ; keep going until elapsed
    CLR r30, r30.t5 ; zet uitgang P9_27 op 0
	LDI32 r4, T1L ; laad teller met T1L
DELAY_T1L:
    SUB r4, r4, 1
    QBNE DELAY_T1L, r4, 0 ; keep going until elapsed
    QBA NEXTBIT
    
ZEROBIT:
    ; '0' bit: hoog voor ~300ns, laag voor ~900ns
    SET r30, r30.t5
	LDI32 r4, T0H ; laad teller met T0H
DELAY_T0H:
    SUB r4, r4, 1
	QBNE DELAY_T0H, r4, 0 ; keep going until elapsed
    CLR r30, r30.t5
	LDI32 r4, T0L ; laad teller met T0L
DELAY_T0L:
    SUB r4, r4, 1
    QBNE DELAY_T0L, r4, 0 ; keep going until elapsed
    QBA NEXTBIT

NEXTBIT:
    QBNE SENDBIT, r5, 0 ; als we nog niet door onze bits zijn, zend de volgende bit
    ADD r0, r0, 4 ; zet pointer volgende geheugenlocatie
    QBNE NEXTLED, r1, 0 ; zend data voor de volgende LED als we nog niet door onze leds zijn
	
    JMP r3.w2 ; ga terug naar C code
