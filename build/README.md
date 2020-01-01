# K210 2nd stage ISP


K210 can only execute the code from **SRAM** or from embeded **ROM**.

After **reset** K210 executes the code from embeded **ROM** at address **`0x88000000`**.

Based on the state of the **`IO16`** pin, one of two course of action can be taken:
* If the **`IO16`** is **high** (not connected), ROM code copies the application code from external SPI Flash at address **`0x00000000`** to SRAM at address **`0x80000000`** and starts its execution.
* If the **`IO16`** is **low** (pulled to GND), ROM code executes **1st stage ISP** which waits for **SLIP** frames on default UART (UART3, external pins 4&5).<br>
An external host must issue the commands to load some valid K210 code to SRAM and start the code execution.<br>
Usually, the **2nd stage ISP** is loaded into SRAM and executed.<br>
The basic purpose of the **2nd stage ISP** is to provide the method of loading the application code to external SPI Flash, which will be loaded and executed in case **`IO16`** is **high** at reset.<br>
This is how the **kflash.py** and similar utilities loads the firmware to SPI Flash.

The standard  **2nd stage ISP** used by **kflash.py** and similar utilities is the binary blob provided without sources and with minimal functionality: it only can load the firmware to SPI Flash.

The **2nd stage ISP** provided in this repository includes all the sources and provides some enhanced functionality:
* Highly optimized, binary size less than 10KB, loads into SRAM in **less than 1 second**
* Write data to Flash using **4K** or **64K** blocks, the Flash address can be aligned to 4K or 64K.<br>(standard ISP only provides 64K block writes and can only write to addresses aligned to 64K)
* Provides **read** from Flash function.
* In addition to **chip erase** function, provides also **block erase** function, a single sector (4KB) or 64KB block can be erased
* Provides the command to get the SPI Flash chip manufacturer's **JEDEC ID**, **64-bit unique serial number** and **size**
* Provides **write to SRAM** and **execute SRAM** functions to load the application code directly to K210 SRAM and execute it.<br>
Similar functionality is provided by the **1st stage ISP** (from ROM), but it can only load at baudrate of 115200 which makes loading large programs impractical.<br>
Functions from **2nd stage ISP** supports loading at **any baudrate** supported by the used board.

<br>

## How to build

Clone the repository or download and unpack the repository zip file.

**The repository contains all the tools and sources needed to build `2nd stage ISP`.**

The same build prerequisities must be satisfied as for building any other K210 application.<br>


### Build from source

* Change the working directory to the **`build`** directory.
* A simple build script **`BUILD.sh`** is provided which builds the `isp` binary.
* Simply execute `BUILD.sh`
* All files needed to use the **2nd stage ISP** with **kflash.py**, **ktool.py** and similar utilities are created:
  * `isp.bin` the ISP binary
  * `ISP_PROG.py` python variable containing `isp.bin` ISP binary, compressed and hexlified, ready to be inserted into **kflash.py**, **ktool.py** or similar Python based application.

<br>

```console
boris@UbuntuMate:/home/LoBo2_Razno/K210_razvoj/ktool/build$ ./BUILD.sh

 =======================================
 === Building 2ns stage ISP for K210 ===
 =======================================

=== Running 'cmake'
=== Running 'make'
Scanning dependencies of target kendryte
[  8%] Building C object lib/CMakeFiles/kendryte.dir/drivers/sysctl.c.obj
[ 33%] Building C object lib/CMakeFiles/kendryte.dir/drivers/fpioa.c.obj
[ 33%] Building C object lib/CMakeFiles/kendryte.dir/bsp/sleep.c.obj
[ 33%] Building C object lib/CMakeFiles/kendryte.dir/drivers/gpio.c.obj
[ 41%] Building C object lib/CMakeFiles/kendryte.dir/drivers/uart.c.obj
[ 50%] Building C object lib/CMakeFiles/kendryte.dir/drivers/utils.c.obj
[ 58%] Building C object lib/CMakeFiles/kendryte.dir/bsp/crt.S.obj
[ 66%] Linking C static library libkendryte.a
[ 66%] Built target kendryte
Scanning dependencies of target isp
[ 91%] Building C object CMakeFiles/isp.dir/src/isp/flash.c.obj
[ 91%] Building C object CMakeFiles/isp.dir/src/isp/main.c.obj
[ 91%] Building C object CMakeFiles/isp.dir/src/isp/slip.c.obj
[100%] Linking C executable isp
Generating .bin file ...
[100%] Built target isp
   text	   data	    bss	    dec	    hex	filename
   9907	     76	  65856	  75839	  1283f	isp
boris@UbuntuMate:/home/LoBo2_Razno/K210_razvoj/ktool/build$ 
```
