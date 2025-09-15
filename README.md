# PRUPixels
In these turbulent times, it's sometimes necessary to let ourselves be distracted by the wonders of physics and electronics, and the magical phenomena they can produce. That’s the spirit behind this project.
This is certainly not a ready-made solution fit for production. Rather, the intention is to demonstrate how the microcontroller on the BeagleBone Black, using its PRU, can be used to generate serial LED data for WS2814 and SK6812RGBW-type RGBW LED drivers, based on a locally stored MP4 video. With few alternations, the code could be used to generate data for UCS1903, WS2811 and WS2812 RGB drivers.
Perhaps this project could also serve as a starting point for those with the ambition to create controllers for use in events and architectural applications. Who knows...

## 1. Resources
- *Exploring BeagleBone* — an excellent book by Dr. Derek Molloy  
- *PRU Guide* — a very helpful resource by Dr. Brian Fraser  
  PRU Guide PDF  
- *Processor SDK Linux for AM64X 3.6.2.3.3* — Section: Writing Mixed C and Assembly Code  
  © 1995–2025 Texas Instruments Incorporated  
- *Interfacing C and C++ With Assembly Language* — especially Section 6.6  
  © 2014–2018 Texas Instruments Incorporated

> These references are provided for educational and informational purposes only. No copyrighted content has been reproduced. All rights remain with the respective authors and publishers.

## 2. BeagleBone Black (Industrial) Setup for PRU Usage
### 2.1. Flashing the BeagleBone Black eMMC

It is always a good idea to start off with the most recent available Linux kernel vesion. At the time of writing, I flashed my BeagleBone with the following image:
[https://www.beagleboard.org/distros/am335x-11-7-2023-09](https://www.beagleboard.org/distros/am335x-11-7-2023-09-02-4gb-emmc-iot-flasher)
1. Insert the microSD card into the BeagleBone **while it is powered off**.
2. Press and hold the **S2 push button** (located near the SD slot, on the opposite side of the board).
3. While holding the button, **apply power** to the board.
4. Release the button **once the user LEDs turn on**.
5. While flashing the eMMC, the user LEDs will display a Knight Rider-style pattern. 
   (for the younger readers: https://en.wikipedia.org/wiki/Knight_Rider)
6. Once the flashing process completes, perform a power cycle to reboot the board and load the newly installed image from eMMC.

To confirm that the correct image is running, use the following command:
```
cat /etc/dogtag
```
On my board, this returns:
```
BeagleBoard.org Debian Bullseye IoT Image 2023-09-02
```

### 2.2. Enabling the PRU Overlay
To enable the PRU (Programmable Real-time Unit) on the BeagleBone Black, you need to modify the boot configuration file.
#### Step 1: Edit `uEnv.txt`
Open the file located at:
```bash
sudo nano /boot/uEnv.txt
````
#### Step 2: Add the following lines to `UEnv.txt`
```bash
uboot_overlay_pru=/lib/firmware/AM335X-PRU-RPROC-4-19-TI-00A0.dtbo
disable_uboot_overlay_video=1
````
#### Step 3: Save and exit `UEnv.txt`

### 2.3. Configure a PRU out pin
To route a PRU output signal to a physical pin on the BeagleBone, you need to configure the pin multiplexing (pinmux). This can be done using the `config-pin` utility from the Linux command line.
In this project, we use the PRU output associated with register 30, bit 5 to bit-bang serial data for WS2812/SK6812-type LED drivers.
This specific PRU output can be routed to pin 27 on header P9 (i.e., P9_27) using the following command:
```
`config-pin P9_27 pruout`
```

## 3. The code in this repo
### 3.1. PRU code
#### 3.1.1. The assembly code: `WSBitbanger.asm`
The assembly code is responsible for pushing out the serial data for the WS/SK LED drivers. It reads data from the 12kB PRU0/PRU1 shared RAM.
There two loops:
1.) Starting at the label `NEXTLED`
2.) Starting at the label `NEXTBIT`

