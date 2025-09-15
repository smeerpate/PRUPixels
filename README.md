# PRUPixels
In these turbulent times, it's sometimes necessary to let ourselves be distracted by the wonders of physics and electronics, and the magical phenomena they can produce. That’s the spirit behind this project.
This is certainly not a ready-made solution fit for production. Rather, the intention is to demonstrate how the microcontroller on the BeagleBone Black, using its PRU, can be used to generate serial LED data for WS and SK-type LED drivers, based on a locally stored MP4 video.
Perhaps this project could also serve as a starting point for those with the ambition to create controllers for use in events and architectural applications. Who knows...

## Resources
- *Exploring BeagleBone* — an excellent book by Dr. Derek Molloy  
- *PRU Guide* — a very helpful resource by Dr. Brian Fraser  
  PRU Guide PDF  
- *Processor SDK Linux for AM64X 3.6.2.3.3* — Section: Writing Mixed C and Assembly Code  
  © 1995–2025 Texas Instruments Incorporated  
- *Interfacing C and C++ With Assembly Language* — especially Section 6.6  
  © 2014–2018 Texas Instruments Incorporated

> These references are provided for educational and informational purposes only. No copyrighted content has been reproduced. All rights remain with the respective authors and publishers.

## BeagleBone Industrial Setup for PRU Usage
To enable the PRU (Programmable Real-time Unit) on the BeagleBone Black, you need to modify the boot configuration file.

### Step 1: Edit `uEnv.txt`
Open the file located at:
```bash
sudo nano /boot/uEnv.txt
````
### Step 2: Add the following lines to `UEnv.txt`
```bash
uboot_overlay_pru=/lib/firmware/AM335X-PRU-RPROC-4-19-TI-00A0.dtbo
disable_uboot_overlay_video=1
````
### Step 3: Save and exit `UEnv.txt`
