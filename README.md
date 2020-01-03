# Ktool

**Ktool** is the **enhanced** version of Kendryte [**kflash.py**](https://github.com/kendryte/kflash.py) utility.

It is compatible with **_kflash.py_** but introduces many new features and enhancement.

Full sources of the enhanced 2nd stage **ISP** are available in this repository's [**src/isp**](https://github.com/loboris/ktool/tree/master/src/isp) directory.<br>
More information about **2nd stage ISP** and build instructions are available in the [**build**](https://github.com/loboris/ktool/tree/master/build) directory.

<br>

## How it works

* On start, K210 board is **reset** into **bootloader ISP** mode (running from K210 **ROM**) by holding the K210 **`IO16`** low and asserting the **reset** pin.
* After reset, if **`IO16`** is low, K210 executes the **1st stage ISP** from **ROM**.<br>
_1st stage ISP is only capable of loading K210 program to the requested SRAM address and execute it._
* After the communication with the _1st stage ISP_ is established, **ktool.py** uploads the **2nd stage ISP** to the K210 SRAM (at address `0x805E0000`) and executes it.
* All further communication is performed via **2nd stage ISP**

<br>

## Features:

* Compatible with standard _`kflash.py`_, the same command line options are supported (but more are added)
* Uses highly optimized **2nd stage ISP** binary which **loads in less than a second** and supports many new features
* Write firmware or data to **any** SPI Flash **address**
* Write firmware/data using also **4K** **chunks** and 4K **alignment**<br>(_`kflash.py` always writes 64K chunks and the firmware address must be 64KB aligned_)
* **Read** Flash content to file is supported
* **Erase** the SPI Flash chip
* Reports SPI Flash chip manufacturer's **JEDEC ID**, **64-bit unique serial number** and **size**
* Optionally swap **endianess** on data write (useful when writing some kind of data, for example file system images)
* **Load** the firmware to **K210 SRAM** end execute it.<br>Loading is done by the **2nd stage ISP** and can be performed **at any baudrate** supported by the board.<br>It is now possible to load and start large firmwares at high speed...<br>(_`kflash.py` supports this function, but loading to SRAM is done at `115200` baud, making it impractical for larger firmwares_)

<br>

## Standard command line arguments:

| Argument&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp; | Description |
| :--- | :--- |
| **`-h`**<br>**`--help`** | Show help message and exit|
| **`-v`**<br>**`--version`** | Print version and exit|
| **`-p PORT`**<br>**`--port PORT`** | Communication port used to connect to the K210 board<br>If not given, automatic port detection is atempted |
| **`-b BAUDRATE`**<br>**`--baudrate BAUDRATE`** | UART baudrate used for for communication with **2nd stage ISP**. Communication with _1st stage ISP_ is always performed at `115200` Bd.<br>_default:_ `115200` |
| **`-l ISP_BIN`**<br>**`--bootloader ISP_BIN`** | **2nd stage ISP** binary file name.<br>External ISP binary is usually used only for testing.<br>If not given, internal ISP binary is used (recommended) |
| **`-k KEY`**<br>**`--key KEY`** | AES key in hex, if you need to encrypt the firmware.<br>_There is no documentation about how it works, it probably requires some programming of K210 **OTP** memory, firmware flashed with this option **will not start**_<br>If not given, no AES encription is used |
| **`-t`**<br>**`--terminal`** | Open terminal emulator (default: `miniterm`) after the operation is finished<br>_default:_ `False` |
| **`-n`**<br>**`--noansi`** | Do not use ANSI colors, recommended when ANSI colors are not supported<br>(for example in Windows _Command prompt_)<br>_default:_ `False` |
| **`-B BOARD`**<br>**`--Board BOARD`** | Select the board to which you are trying to connect<br>Different boards my have different _reset to bootloader_ requirements.<br>If not given automatic board detection is atempted, which usually works good<br>Supported: `ḋan`, `bit`, `trainer`, `kd233`, `goE`, `goD`<br>_default:_ `None` |
| **`--verbose`** | Increase output verbosity; _default:_ `False`; **not used** |
| **`-S`**<br>**`--Slow`** | Slow download mode; _default:_ `False`; **not used** |
| **`firmware`** | The only **positional argument**, usually given after all other arguments<br>Name of the file containing firmware or other data to be flashed to K210 SPI Flash<br> Name of the firmware file to be loaded to K210 SRAM and executed<br>**Not needed** for erase operation, **optional** for read operation<br>The file may contain **binary** firmware/data, firmware **ELF** file or **kfpkg** package (see below) |

<br>

## New and changed command line arguments:

| Argument | Description |
| :--- | :--- |
| **`-a ADDRESS`**<br>**`--address ADDRESS`** | Flash address at which the firmware/data will be written for Flash **write** operations<br>Flash address from which the data will be read for Flash **read** operations<br>integer, can be given as decimal or hex number<br>_default:_ `0` |
| **`-E`**<br>**`--erase`** | Erase the whole flash chip<br>**Be carefull!** |
| **`-L`**<br>**`--rdlen`** | The length of data to be **read** from Flash<br>integer, can be given as decimal or hex number<br>_default:_ `0` |
| **`-R`**<br>**`--read`** | **Read** data from Flash address specified by `--address`<br>The data will be saved to the **`firmware`** name specified in command line<br>_default:_ `"flash_dump.bin"` |
| **`--swapendian`** | Swap endianess when writing the data.<br>It may be necessary when writing some kind of data, for example file system image<br>_default:_ `False` |
| **`-s`**<br>**`--sram`** | Download firmware to K210 **SRAM** and boot<br>Loading is done by the 2nd stage ISP and can be performed **at any baudrate** supported by the board<br>_default:_ `False`<br>_`kflash.py` supports this function, but loading to SRAM is done at `115200` baud, making it impractical for larger firmwares_ |
| **`--termbdr BAUDRATE`** | UART baudrate used for for terminal emulator<br>_default:_ `115200` |
| **`--nosha`** | Flash without firmware prefix and SHA suffix<br>Used when flashing data which is not executable firmware<br>_default:_ `False` |
| **`-T`**<br>**`--onlyterm`** | Do not perform any operation, only start the terminal emulator<br>_default:_ `False` |
| **`--reset`** | Reset the board before running terminal emulator, used only with `--onlyterm` option<br>_default:_ `False` |

<br>

## kfpkg package

**kfpgk** package is a **ZIP archive** containing several **firmware** or **data** binary files and a **json** file describing its content.<br>
It is convenient to use it when multiple files must be flashed at different Flash addresses and with different options.

**`ktool.py`** can proces this file, unpack the content and perform the necessary operations.

The package must contain the **JSON** file named **`flash-list.json`** which describes which files are to be flashed and how.

The following objects must be present to describe each file under `files` object:

| Key | Value description |
| :--- | :--- |
| **`address`** | **integer**; Flash address at which the file content will be written |
| **`bin`** | **string**; Name of the file containing firmware/data to be flashed |
| **`sha256Prefix`** | **boolean**<br>if `true` flash with firmware prefix and SHA suffix (flash the executable firmware)<br>if `ḟalse` flash without firmware prefix and SHA suffix (data) |
| **`swap`** | **boolean**, **may be omitted**, if not present defaults to `false`<br>If `true`, swap endianess when writing the data.<br>It may be necessary when writing some kind of data, for example file system image |

<br>

### Examples of `flash-list.json`

```
{
    "version": "0.1.0",
    "files": [
        {
            "address": 0,
            "bin": "MicroPython.bin",
            "sha256Prefix": true,
            "swap": false
        },
        {
            "address": 5242880,
            "bin": "MicroPython_lfs.img",
            "sha256Prefix": false,
            "swap": true
        }
    ]
}
```

```
{
    "version": "0.1.1",
    "files": [
        {
            "address": 0,
            "bin": "bootloader_lo.bin",
            "sha256Prefix": true
        },
        {
            "address": 4096,
            "bin": "bootloader_hi.bin",
            "sha256Prefix": true
        },
        {
            "address": 16384,
            "bin": "config.bin",
            "sha256Prefix": false
        },
        {
            "address": 20480,
            "bin": "config.bin",
            "sha256Prefix": false
        }
    ]
}
```
