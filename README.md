# Ktool

**Ktool** is the **enhanced** version of [**kflash.py**](https://github.com/kendryte/kflash.py) utility.

It is compatible with **_kflash.py_** but introduces many new features and enhancement.

Full sources of the enhanced 2nd stage **ISP** are available in this repository's **src/isp** directory.<br>
More information and build instructions are available in the **build** directory.

<br>

### How it works

* On start, K210 board is **reset** into **bootloader ISP** mode by holding the K210 **IO16** low and asserting the **reset** pin.
* After reset, if `IO16` is low, K210 executes the **1st stage ISP** from **ROM**.<br>
1st stage ISP is only capable of loading K210 program to the requested SRAM address and execute it.
* After the communication with the _1st stage ISP_ is established, **ktool.py** uploads the **2nd stage ISP** to the K210 SRAM and executes it.
* All further communication is performed via **2nd stage ISP**

<br>

### Features:

* Compatible with standard _kflash.py_, the same command line options are supported
* Uses highly optimized 2nd stage **ISP** binary which loads in less than a second and supports many new features
* Write firmware or data to **any** SPI Flash **address**
* Write firmware/data using **4K** **chunks** and 4K **alignment*<br>(_kflash_ always writes 64K chunks and the firmware address must be 64KB aligned)
* **Read** Flash content to file is supported
* **Erase** the SPI Flash chip
* Reports SPI Flash chip manufacturer's **JEDEC ID**, **64-bit unique serial number** and **size**
* Optionally swap **endianess** on data write (useful when writing some kind of data, for example filesystem image)
* **Load** the firmware to **K210 SRAM** end execute it.<br>Loading is done by the 2nd stage ISP and can be performed **at any baudrate** supported by the board.<br>It is now possible to load and start large firmwares at high speed...

<br>

### Standard command line arguments:

| Argument | Description |
| :---: | :--- |
| **`-h`**<br>**`--help`** | Show help message and exit|
| **`-v`**<br>**`--version`** | Print version and exit|
| **`-p PORT`**<br>**`--port PORT`** | Communication port used to connect to the K210 board<br>If not given, automatic port detection is atempted |
| **`-b BAUDRATE`**<br>**`--baudrate BAUDRATE`** | UART baudrate used for for communication with 2nd stage ISP<br>_default:_ `115200` |
| **`-l BOOTLOADER`**<br>**`--bootloader BOOTLOADER`** | 2nd stage ISP binary<br>If not given, internal ISP binary is used |
| **`-k KEY`**<br>**`--key KEY`** | AES key in hex, if you need to encrypt the firmware.<br> If not given, no AES encription is used |
| **`-t`**<br>**`--terminal`** | Open terminal emulator (`miniterm`) after the operation is finished<br>_default:_ `False` |
| **`-n`**<br>**`--noansi`** | Do not use ANSI colors, recommended when ANSI colors are not supported (for example in Windows CMD)<br>_default:_ `False` |
| **`-B BOARD`**<br>**`--Board BOARD`** | Select the board to which you aree trying to connect<br>Different boards my have different reset to bootloader requirements.<br>If not given automatic board detection is atempted, which usually works good<br>Supported: `ḋan`, `bit`, `trainer`, `kd233`, `goE`, `goD`<br>_default:_ `None` |
| **`--verbose`** | Increase output verbosity; _default:_ `False`; **not used** |
| **`-S<br>--Slow`** | Slow download mode; _default:_ `False`; **not used** |

<br>

### New and changed command line arguments:

| Argument | Description |
| :---: | :--- |
| **`-a ADDRESS`**<br>**`--address ADDRESS`** | The addres in Flash to/from the firmware/data will be written/read<br>integer, can be given as decimal or hex number; _default:_ `0` |
| **`-E`**<br>**`--erase`** | Erase the whole flash chip |
| **`-L`**<br>**`--rdlen`** | The length of data to be **read** from Flash |
| **`-R`**<br>**`--read`** | Read data from Flash addres specified by `--address`<br>The data will be saved to the firmware name specified in command line (_default:_ `flash_dump.bin` |
| **`--swapendian`** | Swap endianess when writing the data.<br>It may be necessary when writing some kind of data, for example filesystem image |
| **`-s`**<br>**`--sram`** | Download firmware to K210 **SRAM** and boot<br>Loading is done by the 2nd stage ISP and can be performed **at any baudrate** supported by the board |
