# PRUPixels
In these turbulent times, it's sometimes necessary to let ourselves be distracted by the wonders of physics and electronics, and the magical phenomena they can produce. That’s the spirit behind this project.

This is certainly not a ready-made solution fit for production. Rather, the intention is to demonstrate how the microcontroller on the BeagleBone Black (BBB), using its PRU, can be used to generate serial LED data for WS2814 and SK6812RGBW-type RGBW LED drivers, based on a locally stored MP4 video. With few alternations, the code could be used to generate data for UCS1903, WS2811 and WS2812 RGB drivers. WS2812, SK6812, UCS1903 drivers are extensively used in addressable LED strips.

You might also be interested in checking the MBI5124Pixels branch for generating serial data for shift register based constant current LED drivers like MBI5124.

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
1. Starting at the label `NEXTLED`
2. Starting at the label `NEXTBIT`

Note: In the assembly code, it is important that we don't mess with the Save-on-entry registers (R3.w2-R13) to enable returning to the calling code (i.e. main.c).

#### 3.1.2. The C-code
Is used for configuring and calling the ASM-code.

An interesting macro might be this one: `#define nLEDs (*((volatile unsigned int *)0x00000110))`. This allows us to assign nLEDs as if it were a regular variable. Instead, it is actually a dereferenced pointer to a memory location (namely, 0x110) in the PRU0 DRAM. This way, parameters can be fed into the assembly code.

The while loop is executed endlessly. After execution, the bit-banging assembly code returns to the C-code. In the assembly code, it is important that we don't mess with the Save-on-entry registers (R3.w2-R13) to enable returning to the calling code (i.e. main.c).

There is a `__delay_cycles()' to allow for the mandatory gap between the pulse trains. A value of 500000 roughly corresponds to 2.5ms.

## 3.2. The content player code
Important files here are `player.c`, `pixelLUT.c` and `pixelLUT.h`.
#### 3.2.1. player.c
Using FFMPEG, this code opens a file called `video.mp4` and writes RGB pixel data into the PRU shared memory. The lookup table `pixelLUT.c` is used to assign the associated RGB values from the video-field to the pixels.
The video file content is scaled to a 150x150 pixel field, from which RGB data is picked from (using the LUT) to write into the PRU shared RAM.
The video content is played in an infinite loop.

## 4. Making it work
- clone this repo in the Beaglebone's home directory `git clone https://github.com/smeerpate/PRUPixels.git`
- `cd` into the project directory (normaly `/PRUPixels`)
- do `make`
  - This will create a new directory called `gen`
- Let's say we want to run this code on PRU0. **Note**: PRU0 is refered to as 'remoteproc1' and PRU1 is referred to as 'remoteproc2'. 
  - First we need to ensure that the PRU is running. Use this command `cat /sys/class/remoteproc/remoteproc1/state`.
    - `offline`: PRU is **not** running
    - `running`: PRU is running
  - If the prcessor is **not** running, start it using this command: `echo 'start' > /sys/class/remoteproc/remoteproc1/state`
  - Install the code on PRU0 using this command: `sudo make install_PRU0`
    - if you get this message ``
        Stopping current PRU0 application (/sys/class/remoteproc/remoteproc1)
        stop
        tee: /sys/class/remoteproc/remoteproc1/state: Invalid argument
        make: *** [Makefile:45: install_PRU0] Error 1
        ``, the PRU is probably not running. The PRU needs to be in the "running" state before using `make install_PRUx`
- Set the pin IO mux to connect the PRU output associated with `r30.t5` to pin P9_27: `config-pin P9_27 pruout`
- Since in main.c we have assigned values to the first 4 shared memory locations, we should see the first 4 pixels light up blue, white, red, green respectively.
- Compile the video player code using: `gcc -o player player.c  pixelLUT.c $(pkg-config --cflags --libs libavformat libavcodec libswscale libavutil)`
  - Make sure FFMPEG is installed
- run the player: `sudo ./player`



