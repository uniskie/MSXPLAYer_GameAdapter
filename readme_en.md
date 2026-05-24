# MSXPLAYer Game Cartridge Adapter

[日本語版へ](./readme.md)

A tool for reading and writing MSX cassettes via USB CDC (virtual COM port). (MSXPLAYer support planned)

![title00](./IMAGE/title_fixed000.jpg)

![title01](./IMAGE/title_fixed001.jpg)


## What is this?

A cartridge adapter for reading and writing **MSX ROM cartridges** using a Raspberry Pi-based microcontroller (RP2350).
By sending commands from a PC via **USB CDC (serial)**, you can perform the following operations:

- Read/Write ROM cartridges
- IO Read/Write
- Bulk transfer to/from buffer (64KB) and PC
- Batch slot access via simple scripts on the buffer

## Features

- **USB CDC (virtual COM)** command operation (generally no driver required; available on Windows / Linux / macOS)
- **Slot power control and overcurrent detection** built-in
- Supports **Read/Write** to cartridge memory and IO space
- **Clock signal** support

- **5V IO voltage compatible**
- **Firmware updates** possible by users
- **Low-latency design** utilizing **dual-core configuration**
  - Core 0: USB reception, command parsing, transmission queue output
  - Core 1: Command execution (GPIO access, etc.)
- **Command Queue functionality** for continuous command execution
- **Simple script engine** included (simple VM)

(The following applies to distribution items)

- **Reliable connector parts from major manufacturers (AMP/Hirose)** used for slot connectors

## Differences from actual MSX cartridge slots

Unlike actual machines, the following features are NOT supported:

- +12V/-12V power supply
- Sound output
- DRAM Refresh signal support
- M1 signal support

## Device Description

![pcb001](./IMAGE/pcb001.jpg)

1. MSX SLOT: Insert MSX-standard cartridges here.
2. USB-C Port: Connection port to the PC.
3. GROVE Port: GROVE-standard communication port. Signal voltage level is 3.3V. Currently outputs debug UART signals.
4. ACCESS LED: LED indicating cartridge access status. Lights up when power is supplied and during cartridge access.
5. BOOT Switch: Not normally used.

## [Important] MSXPLAYer Support

MSXPLAYer support is currently in progress at the MSX Association.
Distribution versions from our company are planned to include MSXPLAYer, but details are still undetermined.

For now, this is an Early Access version and distribution is limited to Testers who already own MSXPLAYer and can cooperate with operation testing.

Therefore, the following book is required to obtain MSXPLAYer:

**"MSX-BASICでゲームを作ろう　懐かしくて新しいMSXで大人になった今ならわかる"**
(Create Games with MSX-BASIC: Understanding MSX Now That We're Grown Up)

<https://www.amazon.co.jp/dp/4297148900/>
<https://books.rakuten.co.jp/rb/18203653/>
<https://www.yodobashi.com/product/100000009004112958/>

We plan to support it by applying patches to the MSXPLAYer obtained here.
We are also preparing a bug report form for when issues occur with MSXPLAYer.

Specific information on how to obtain the compatible version will be added after distribution begins.

## Support for other platforms such as OpenMSX

We are looking for volunteers to help. We would be happy to discuss this via DM on X.

## Distribution

Currently offered as an Early Access version.
This is a limited distribution for Testers who can cooperate with operation testing and compatible software development.
Please purchase only if you understand that MSXPLAYer is required separately and that specifications may differ from future official versions.

Assembly requires simple work using a screwdriver.
The connector kit version requires simple soldering work on the slot section.

### Distribution Sites

Two versions will be available: a soldering kit version (where the slot requires soldering) and a completed PCB version.
Prices are likely to fluctuate significantly for a while depending on global manufacturing conditions. Please understand this.

#### Booth

2026/5/10 12:00(JPT) Start

<https://ifc.booth.pm/items/8175544>

Completed PCB version: 7,060JPY / Soldering kit version: 6,560JPY

## Included Items

1. Game Cartridge Adapter PCB
2. Dedicated aluminum panel
3. Rubber feet
4. Spacers x 4
5. Screws (3mm x 8mm) x 8
6. LED light pipe
7. 50-pin card edge connector (soldering kit version only)

![PARTS](./IMAGE/parts_image.jpg)

## Assembly Instructions

### 1. Solder the Card Edge Connector (Soldering kit version only)

Solder the card edge connector. The connector has no specific direction.
When attaching, be careful not to leave gaps between the connector and the PCB.
![assembly1](./IMAGE/assembly001.jpg)

It is best to first solder both end terminals to check for proper alignment, then solder the remaining terminals.
![assembly2](./IMAGE/assembly002.jpg)

If the included connector is from Hirose, the leads are slightly too long as-is, so trim the excess using wire cutters.
![assembly3](./IMAGE/assembly003.jpg)

### 2. Attach the Light Pipe

Press the light pipe into the panel. Insert the part with the flat side first into the panel surface and push until there are no gaps.
![assembly4](./IMAGE/assembly004.jpg)
![assembly5](./IMAGE/assembly005.jpg)

### 3. Attach the Spacers (Front side)

Attach spacers using screws on the front of the panel.
Since final adjustments will be made later, it is better to tighten only lightly at this stage.
![assembly6](./IMAGE/assembly006.jpg)

### 4. Attach the Spacers (Back side)

Place the PCB on top of the spacers and secure with screws. Pay attention to the orientation of the PCB.
![assembly7](./IMAGE/assembly007.jpg)
![assembly8](./IMAGE/assembly008.jpg)

After tightening the 4 back screws, fully tighten the 4 front screws as well.
![assembly9](./IMAGE/assembly009.jpg)

### 5. Attach the Rubber Feet

Attach rubber feet to 4 locations on the back of the PCB near the screws.
![assembly10](./IMAGE/assembly010.jpg)

All assembly work is now complete. Thank you for your effort.

## Operation Check (for Windows)  

Please download the file below, insert the MSX cartridge into this device, and run the program.  
If it is working correctly, the contents of the cartridge will be displayed starting from address 0x4000, as shown below.  

**Operation Check Program** [MSXPLAYer Game Cassette AdapterOperation Check Program](./SOFTWARE/TestProgram/)  

![Operation Check Screen](./IMAGE/testprg000.png)

---

## Technical Documentation

### USB CDC Command Specifications

[MSXPLAYer Game Cassette Adapter Command Specifications](./com_command_en.md)

### Sample Programs

For Windows: (Verified with VC++2019/2026)

- **Game Cartridge Dump Sample:** [ROM Cartridge Dump Program](./SOFTWARE/MSXCR_ROMDUMPER/)
- **Game Cartridge Dump Sample (requires FW 260520 or later):** [ROM Cartridge Dump Program](./SOFTWARE/MSXCR_ROMDUMPER_FW260519/)
- **Script Engine Sample:** [MSX_SimpleCartridge Write Program](./SOFTWARE/SimpleROM64KWriter/)
  Compatible cartridge: [https://github.com/v9938/MSX_SimpleCartridge](https://github.com/v9938/MSX_SimpleCartridge)

### Other Programs

- **MSX-PLAYer-GCA-Reader by Lithelia:** [https://github.com/Lithelia/MSX-PLAYer-GCA-Reader](https://github.com/Lithelia/MSX-PLAYer-GCA-Reader)
- **mgadump by madscient:** [https://github.com/madscient/mgadump](https://github.com/madscient/mgadump)
- **MSXPLAYer Game Cartidge Adapter GUI by t-bucchi:** [https://github.com/t-bucchi/msx-cartrigde-adapter-gui](https://github.com/t-bucchi/msx-cartrigde-adapter-gui)

### How to Use MSXCR_ROMDUMPER_FW260519

This Windows tool uses the `SMTH` command added in firmware version 260520 and later
to dump a ROM while also obtaining its HASH value during the read process.

#### Command Line

- Normal mode with an explicitly specified output file name

  ```bat
  MSXCR_ROMDUMPER.exe dump.rom
  ```

- Automatic file naming mode with an explicitly specified output directory

  ```bat
  MSXCR_ROMDUMPER.exe /AUTO
  ```

  or

  ```bat
  MSXCR_ROMDUMPER.exe /AUTO .\OUT
  ```

#### Description of Each Mode

**Normal Mode**

The ROM is saved using the file name specified on the command line.

**Automatic File Naming Mode (`/AUTO`)**

The save file name is automatically determined based on ROM information and the HASH value.  
If a second argument is specified, the file is saved into that folder.  
If the second argument is omitted, the file is saved into the current folder.  

#### About `msxromdb.xml` / `softwaredb.xml`

This software performs automatic matching using the ROM database from BlueMSX / OpenMSX.  
Place the file in the same folder as the executable using the name `softwaredb.xml` or `msxromdb.xml`.  
If this file exists, the ROM DB information is used for title identification and file name determination.  

The XML-format ROM database can be obtained from the BlueMSX installation directory or from the site below.  
[https://romdb.vampier.net/downloads.php](https://romdb.vampier.net/downloads.php)

#### About Output File Names

In **Normal Mode**, the ROM is saved exactly with the file name you specify.  
In **Automatic File Naming Mode**, the ROM is saved using a name based on the ROM identification result.  
If `softwaredb.xml` / `msxromdb.xml` is available, the file name is determined primarily using the information registered in the ROM DB.  
If the ROM DB is unavailable or no matching entry is found, an automatically generated name based on ROM size, HASH value, and similar information is used.  
If a file with the same name already exists in the destination, the existing file is read and compared.  
If the contents are identical, `[same_+hash]` is added to the beginning of the file name.  
If the contents are different, `[other_+hash]` is added to the beginning of the file name.  
If the dump is presumed to have failed, `[unsuccessful]` is added to the beginning of the file name.  

## Firmware

Compiled firmware is available in the following folder:

[Firmware Location](./FIRMWARE/UF2)

## Firmware Update

There are two methods available.

- Using the BOOT switch method
Press the BOOT switch while inserting the USB cable to enter BOOT mode.
A drive named "RP2350" will appear; copy the firmware file to it.

- Using the Firmware Update tool
We provide `msxcr_ffu.exe` as an FFU support tool.
Running the batch file `ffu.bat` in the FFU folder above will update the firmware.

### File Structure (Overview)

- `main.c`
  - USB CDC reception (line buffer → parsing)
  - Command queue (Core 0 → Core 1)
  - Slot Memory / IO Read/Write
  - BRCV reception (binary receive mode)
  - BSND transmission (binary transmission queue)
  - LED control (WS2812)
  - Factory Test (GPIO test)
  - Script Engine (executeCommands)
- `commands.c / commands.h`
  - Command name → Execution function (cmd_*) table (list of public commands)
- `ports.c / ports.h`
  - GPIO definitions (`board_pins[]`)
- `ws2812.pio`
  - PIO program for WS2812 control (from SDK pico-examples)
- `pwm_low_hiz.pio`
  - PIO for slot clock (LOW → Hi-Z)
- `usb_descriptors.c / tusb_config.h`
  - USB CDC descriptor/buffer configuration

### Firmware Operation Overview (Data Flow)

1. Text command sent from PC via USB CDC (example: `SMRD,1000\r\n`)
2. Core 0 accumulates up to line break in `lineBuf`, parses it, and enters it into `commandBufs[]`
3. Core 1 searches for the corresponding function in `cmd_table[]` and executes `cmd_*()`
4. Response is queued in `cdc_queue[]`, and Core 0's `cdc_task()` transmits to USB

### Build

Compiled in Visual Studio Code PIC-SDK2.20 environment.

## USB VID/PID

We use the VID and PID allocation from the former ASCII Corporation under the license of MSX License Corporation.
If you manufacture a modified version of this product, please use a different VID/PID.

## Circuit Diagram

[Circuit Diagram PDF](./PCB/MSXPLAYerCR_1SLOT_RevD.pdf)
![Circuit Diagram](./PCB/MSXPLAYerCR_1SLOT_RevD_SCR.png)

## PCB Data

[Rev D Gerber Data](./PCB/GARBER_DATA/)

![PCB Image](./PCB/MSXPLAYerCR_1SLOT_RevD_PCB.png)

## BOM List

[Rev D PCB Parts List](./PCB/partslist_RevD.md)

## Panel

[Aluminum Panel Data](./PCB/PANEL_DATA/)

![Panel Image](./PCB/PANEL_DATA/panel_revD.png)

The following additional parts are required for assembly:

| Part Name | Quantity | Source |
| --- | --- | --- |
| 3mm x 8mm Screw | 8 | <https://www.hirosugi-net.co.jp/shop/g/g104024/> |
| 3mm x 20mm Spacer | 4 | <https://www.hirosugi-net.co.jp/shop/g/g670/> |
| Light Pipe (VCC LFB075CTP) | 1 | <https://www.digikey.jp/ja/products/detail/visual-communications-company-vcc/LFB075CTP/5723594> |

## Regarding MSXPLAYer Name/Logo

MSX and MSXPLAYer are registered trademarks of MSX License Corporation.

The MSXPLAYer logo attached to our distribution products is used with permission from the MSX Association under cooperative development agreements.

## License

This project is published under the MIT License.

Note that some portions of the code are copied from Raspberry Pi's pico-sdk v2.20.
Those portions are licensed by Raspberry Pi (Trading) Ltd. under the requirements of the BSD 3-Clause "New" or "Revised" License.
