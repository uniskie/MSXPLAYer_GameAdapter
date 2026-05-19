# MSXPLAYer Game Cassette Adapter Command Specifications

## General Specifications

- 1 line = 1 command (comma-separated)
- Format (maximum 4 arguments):

  ```CMD
  CMD[,ARG1[,ARG2[,ARG3[,ARG4]]]]<CR|LF|CRLF>
  ```

- Command names are case-insensitive (converted to uppercase internally)
- Arguments are generally in **hexadecimal** (optional `0x` prefix)
- Omitted arguments will use default behavior
- Return values:
  - Normally returns `OK` / `FAIL` as the final result
  - During `ERROFF`, OK/FAIL display is suppressed, only success/failure counts are updated

## Command List (Specification Format / Implementation Compliant)

### 1) HSET - Hardware Setting (Dummy/Not Implemented)

- **Function**: Hardware settings (currently a dummy)
- **Format**: `HSET,[Address],[Data]`
- **Arguments**:
  - `Address` : Target configuration address (hexadecimal)
  - `Data` : Configuration value (hexadecimal)
- **Response**: `OK`
- **Notes**: Currently no actual operation. Reserved for future expansion.

---

### 2) ERROFF - Result Display OFF

- **Function**: Suppresses sequential display of command execution results (OK/FAIL display)
- **Format**: `ERROFF`
- **Arguments**: None
- **Response**: `OK`
- **Notes**:
  - When enabled, accumulates statistics in `passCount/errCount`
  - Used to suppress logs during bulk processing (transfers, scripts)

---

### 3) ERRON - Result Display ON + Summary

- **Function**: Cancels `ERROFF` and displays statistics for the suppression period
- **Format**: `ERRON`
- **Arguments**: None
- **Response**:
  - `PASS : <passCount>`
  - `FAIL : <errCount>`
  - Followed by `FAIL` if there were failures, otherwise `OK`
- **Notes**:
  - If there are `FAIL` results, `ERRON` itself returns `FAIL`

---

### 4) BCLR - Buffer Clear (slotMem Initialization)

- **Function**: Fills the internal buffer (64KB) with a specified value
- **Format**: `BCLR(,[BufferAddress],[Length],[Data])`
- **Arguments**:
  - `BufferAddress` : Start position (default 0 if omitted)
  - `Length` : Length (default 64KB equivalent if omitted)
  - `Data` : Fill value (default `0xFF` if omitted)
- **Response**: `OK` / `FAIL`
- **Notes**:
  - Out-of-range specifications result in `FAIL`
  - Length may be adjusted to match buffer boundaries

---

### 5) BSND - Buffer Send Host (Device → Host Binary Send)

- **Function**: Sends internal buffer data to PC (binary)
- **Format**: `BSND,[BufferAddress],[Length]`
- **Arguments**:
  - `BufferAddress` : Send start position
  - `Length` : Number of bytes to send
- **Response**:
  - Binary data is sent to the PC
  - Followed by `OK/FAIL` (when displayFlag=true)
- **Notes**:

---

### 6) BRCV - Buffer Receive Host (Host → Device Binary Receive)

- **Function**: Receives binary data of specified length from PC and stores it in `slotMem[]`
- **Format**: `BRCV,[BufferAddress],[Length]` + (followed by Length bytes of binary data)
- **Arguments**:
  - `BufferAddress` : Storage start position
  - `Length` : Number of bytes to receive
- **Response**:
  - `OK/FAIL` after reception is complete
- **Notes**:
  - Enters binary receive mode immediately after `BRCV` execution
  - Out-of-range (`BufferAddress + Length` exceeds 64KB) results in `FAIL`

---

### 7) HVER - Hardware Version

- **Function**: Displays hardware name, revision, and build date
- **Format**: `HVER`
- **Arguments**: None
- **Response**:
  - `HW_NAME`
  - `HW_VERSION`
  - `FIRMWARE DATE`
  - `OK`
- **Notes**:
  - `HW_NAME`: Name of the card reader
  - `HW_VERSION`: Hardware version
  - `FIRMWARE DATE`: Firmware release date

---

### 8) HINF - Hardware Information

- **Function**: Displays hardware configuration information
- **Format**: `HINF`
- **Arguments**: None
- **Response**: Each `KEY,VALUE` line + `OK`
- **Notes**:
  - `SLOTNUM`: Number of reader slots
    Indicates the number of physical slots on the reader (1-4)
  - `SLOTPHY`: Physical slot arrangement information on the reader
    Shows the arrangement of physical slots on the reader (1 = active)

    |bit7-4|bit3|bit2|bit1|bit0|
    |---|---|---|---|---|
    |0000|SLOT3|SLOT2|SLOT1|SLOT0|

    Example: SLOTNUM:1 / SLOTPHY:2 = Single slot on reader, treated as SLOT1

  - `POWERCTRL`: Presence of power control
    Indicates whether power control for cartridges is available (1 = yes)
  - `CURRENTSENSOR`: Presence of overcurrent detection
    Indicates whether overcurrent detection circuit for cartridges is available (1 = yes)
  - `PWR12V`: Presence of +12V/-12V power
    Indicates whether +12V/-12V power is available for cartridges (1 = yes)
  - `SLOTCLOCK`: Presence of external slot clock
    Indicates whether a precise 3.58MHz clock supply circuit for cartridges is available (1 = yes)
  - `LINEOUT`: Presence of audio line output
    Indicates whether a sound output circuit for the cartridge SOUND terminal is available (1 = yes)
  - `PSGUNIT`: Presence of PSG unit
    Indicates whether the reader has PSG sound source equivalent functionality (1 = yes)
  - `LFCR`: Line feed code setting value
    Displays the type of line feed code in the input COMMAND string (1 = "\n" = 0x0d, 0x0a treatment)
  - `COMDBG`: Debug output setting via UART
    When 1, debug output is configured to be output from UART
  - `SCRLOOP`: Maximum LOOP count setting for script mode
    Displays the LOOP count for script mode. Initial value is 1000

---

### 9) HSTS - Hardware Internal Status

- **Function**: Displays internal status (mainly queue remaining count and error presence)
- **Format**: `HSTS`
- **Arguments**: None
- **Response**:
  - Displays queue remaining count equivalent (`count-1`) in one line
  - `OK/FAIL`
- **Notes**:
  - Returns `FAIL` if an error occurred in the executed queue

---

### 10) SCHK - Slot Cassette Check

- **Function**: Displays slot connection status (power ON not required)
- **Format**: `SCHK`
- **Arguments**: None
- **Response**: One of `0000/0010/0100/0110` + `OK`
- **Notes**: Response is a string in SLOT3210 order. Slots with inserted cartridges show 1.

---

### 11) SPON - Slot Power ON

- **Function**: Powers on slot, enables clock, initializes RESET control, etc.
- **Format**: `SPON`
- **Arguments**: None
- **Response**: `OK/FAIL`
- **Notes**:
  - Returns `FAIL` when overcurrent occurs

---

### 12) SPOFF - Slot Power OFF

- **Function**: Powers off slot, returns signals to safe state
- **Format**: `SPOFF`
- **Arguments**: None
- **Response**: `OK/FAIL`
- **Notes**:

---

### 13) SRST - Slot Reset

- **Function**: Toggles slot RESET signal for reset
- **Format**: `SRST`
- **Arguments**: None
- **Response**: `OK`

---

### 14) SSEL - Slot Select

- **Function**: Sets default slot number
- **Format**: `SSEL,[Slot]`
- **Arguments**:
  - `Slot` : `1` or `2`
- **Response**: `OK/FAIL`
- **Notes**:
  - Omitting `Slot` results in `FAIL`
  - This value is used when slot is omitted thereafter

---

### 15) SMRD - Slot Memory Read (1 byte)

- **Function**: Reads 1 byte from slot memory
- **Format**: `SMRD,[Address](,[Slot])`
- **Arguments**:
  - `Address` : 0000〜FFFF　Slot memory address
  - `Slot` : 1 or 2 (uses defaultSlot if omitted)
- **Response**:
  - `<addr> : <data>` (e.g., `1000 : 3F`) + `OK/FAIL`

---

### 16) SMWR - Slot Memory Write (1 byte)

- **Function**: Writes 1 byte to slot memory
- **Format**: `SMWR,[Address],[Data](,[Slot])`
- **Arguments**:
  - `Address` : 0000〜FFFF　Slot memory address
  - `Data` : 00〜FF　Write data
  - `Slot` : 1 or 2 (uses defaultSlot if omitted)
- **Response**: `OK/FAIL`

---

### 17) SMTR - Slot → Buffer Transfer Read (Bulk Read)

- **Function**: Continuous read from slot to buffer
- **Format**: `SMTR,[Address](,[Length],[BufferAddress],[Slot])`
- **Arguments**:
  - `Address` : 0000〜FFFF　Slot-side start address
  - `Length` :  0000〜FFFF　Read length (maximum if omitted)
  - `BufferAddress` :  0000〜FFFF　Buffer storage start position (0 if omitted)
  - `Slot` : 1 or 2 (uses defaultSlot if omitted)
- **Response**: `OK/FAIL`

---

### 18) SMTW - Buffer → Slot Transfer Write (Bulk Write)

- **Function**: Continuous write from buffer to slot
- **Format**: `SMTW,[Address],[Length],[BufferAddress](,[Slot])`
- **Arguments**:
  - `Address` :  0000〜FFFF　Slot-side start address
  - `Length` :  0000〜FFFF　Write length
  - `BufferAddress` : Buffer read start position
  - `Slot` : 1 or 2 (uses defaultSlot if omitted)
- **Response**: `OK/FAIL`

---

### 19) IORD - IO Read (1 byte)

- **Function**: Reads 1 byte from IO port
- **Format**: `IORD,[IO]`
- **Arguments**:
  - `IO` :  0000〜FFFF　IO address (16-bit)
- **Response**:
  - `<io> : <data>` + `OK/FAIL`

---

### 20) IOWR - IO Write (1 byte)

- **Function**: Writes 1 byte to IO port
- **Format**: `IOWR,[IO],[Data]`
- **Arguments**:
  - `IO` :  0000〜FFFF　IO address
  - `Data` : 00〜FF　Write data
- **Response**: `OK/FAIL`

---

### 21) IOTR - IO → Buffer Transfer Read (Implementation Compliant)

- **Function**: Continuous read from IO and stores in buffer
- **Format**: `IOTR,[IO],[Length],[BufferAddress]`
- **Arguments (Implementation Compliant)**:
  - `IO` : 0000〜FFFF　Start IO address
  - `Length` : 0000〜FFFF　Number of reads (bytes)
  - `BufferAddress` : Buffer storage start position
- **Response**: `OK/FAIL`

---

### 22) IOTW - Buffer → IO Transfer Write (Implementation Compliant)

- **Function**: Continuous write from buffer to IO
- **Format**: `IOTW,[IO],[Length],[BufferAddress]`
- **Arguments (Implementation Compliant)**:
  - `IO` : 0000〜FFFF　Start IO address
  - `Length` : 0000〜FFFF　Number of writes (bytes)
  - `BufferAddress` : 0000〜FFFF　Buffer read start position
- **Response**: `OK/FAIL`

---

### 23) BDMP - Buffer Dump (Debug Use)

- **Function**: Displays buffer contents in HEX+ASCII
- **Format**: `BDMP,[BufferAddress](,[Length])`
- **Arguments**:
  - `BufferAddress` : 0000〜FFFF　Start position (0 if omitted)
  - `Length` : 0000〜FFFF　Display length (128 if omitted)
- **Response**: Dump lines + `OK`

---

### 24) SDMP - Slot Dump (Slot Memory Dump)

- **Function**: Reads directly from slot while displaying dump
- **Format**: `SDMP,[Address](,[Length],[Slot])`
- **Arguments**:
  - `Address` : 0000〜FFFF　Start address (0 if omitted)
  - `Length` : 0000〜FFFF　Display length (128 if omitted)
  - `Slot` : 1 or 2 (uses defaultSlot if omitted)
- **Response**: Dump lines + `OK/FAIL`

---

### 25) BSCR - Buffer Script Execute (Script Execution)

- **Function**: Executes script on buffer
- **Format**: `BSCR,[BufferAddress](,[Slot])`
- **Arguments**:
  - `BufferAddress` :  0000〜FFFF　Script start (0 if omitted)
  - `Slot` : 1 or 2 (uses defaultSlot if omitted)
- **Response**: `OK/FAIL`
- **Notes**:
  - Instruction format is 4 bytes/instruction: `[cmd][addr_hi][addr_lo][data]`
  - See separate section for script details

---

### 26) FTEST - Factory Test

- **Function**: Executes comprehensive factory test
- **Format**: `FTEST`
- **Arguments**: None
- **Response**: Test logs + `OK/FAIL`

---

### 27) LEDRDY / LEDPON / LEDACC - LED Color Setting

- **Function**: Set LED color (READY/POWER ON/SLOT ACCESS) in RGB
- **Format**:
  - `LEDRDY(,[R],[G],[B])`
  - `LEDPON(,[R],[G],[B])`
  - `LEDACC(,[R],[G],[B])`
- **Arguments**:
  - `R` `G` `B` : 0x00〜0xFF (optional; default values if omitted)
- **Response**: `OK`

---

### 28) SDBGON - Serial Debug Log ON

- **Function**: Enables debug log output from serial port
- **Format**: `SDBGON`
- **Arguments**: None
- **Response**: `OK`
- **Notes**:
  - Output is from the GROVE port (UART)
  - Enabling this feature will slow down execution speed
  - Watch out for buffer overflow

---

### 29) _FFU - Bootloader Launch

- **Function**: Transitions to FFU mode, transitions to USB boot mode
- **Format**: `_FFU`
- **Arguments**: None
- **Response**: Message output followed by reboot (no further OK/FAIL returned)

---

### 30) LSCR - Set Maximum LOOP Count for Script Mode

- **Function**: Sets the maximum loop count for waiting on conditions in script mode
- **Format**: `LSCR(,Maximum LOOP Count)`
- **Arguments**: 0-0xFFFF (default 1000 if omitted)
- **Response**: `OK`

---

### 17) SMTH - Slot → Buffer Transfer Read (Bulk Read) with HASH

- **Function**: Continuous read from slot to buffer + 32bit Hash
- **Format**: `SMTR,[Address](,[Length],[BufferAddress],[Slot])`
- **Arguments**:
  - `Address` : 0000〜FFFF　Slot-side start address
  - `Length` :  0000〜FFFF　Read length (maximum if omitted)
  - `BufferAddress` :  0000〜FFFF　Buffer storage start position (0 if omitted)
  - `Slot` : 1 or 2 (uses defaultSlot if omitted)
- **Response**: `<Length> : <HASH 32bit>`+`OK/FAIL`

---

## Serial Command Examples

### Example 1: Read slot 0x0000〜0x3FFF and send to PC

```CMD_EX1
SPON
SMTR,0000,4000,0000,01
BSND,0000,4000
SPOFF
```

### Example 2: Receive binary from PC and write to 0x8000

```CMD_EX2
SPON
BRCV,0000,2000
(Send 0x2000 bytes of binary data here)
SMTW,8000,2000,0000,01
SPOFF
```

## Script Mode

A mode that allows data read/write and comparison execution without communication with the PC
by using scripts placed in the buffer.

## Script Format

- Script format: [Command],[Address(2Byte)],[DATA]...
- Each script is fixed-length at 4 bytes

### Script Data Format

|Address|+0 Byte|+1 Byte|+2 Byte|+3 Byte|
|---|---|---|---|---|
|**DATA**|Instruction Code|Upper Address|Lower Address|Data|

### Instruction List

|Command|Instruction Name|Description|
|---|---|---|
|0x00|NOP|Do nothing|
|0x01|Read Memory|Executes Read against slot|
|0x02|Write Memory|Writes DATA to slot (LastData is overwritten with DATA)|
|0x03|Read IO|Executes Read using IO against slot (LastData becomes the read DATA)|
|0x04|Write IO|Executes Write using IO against slot (LastData is overwritten with DATA)|
|0x05|Wait|Waits for the duration specified in address (milliseconds)|
|0x06|Compare|Compares LastData with data. If matched, skips next instruction|
|0x07|AND|Calculates LastData [AND] DATA. If result is 0x00, skips next instruction|
|0x08|OR|Calculates LastData [OR] DATA. If result is 0x00, skips next instruction|
|0x09|XOR|Calculates LastData [XOR] DATA. If result is 0x00, skips next instruction|
|0x0A|JMP|Skips instructions by the amount of DATA (0x00-7F=forward/0x80-FF=backward)|
|0x0B|PUSH|Writes lastData to the Buffer memory location specified by the address|
|0xFE|Abort|Terminates script execution with failure|
|0xFF|End|Terminates script execution with success|

※ JMP instruction fails the script if executed at the same location over a certain number of times (default 1000).
This value can be changed with the "LSCR" command.

### Script Example

Executable with the following command:

```CMD_BRCV
BRCV,0,20
(Send the following binary)
BSCR,0
```

|Binary Value|Command|Description|
|---|---|---|
|0x02,0x55,0x55,0xaa|[Write Memory]  Address:0x5555 Data:0xaa|Write 0xAA to 0x5555|
|0x02,0xaa,0xaa,0x55|[Write Memory]  Address:0xaaaa Data:0x55|Write 0x55 to 0xAAAA|
|0x02,0x55,0x55,0xa0|[Write Memory]  Address:0x5555 Data:0xa0|Write 0xA0 to 0x5555|
|0x02,0x40,0x00,0x41|[Write Memory]  Address:0x4000 Data:0x41|Write 0x41 to 0x4000|
|0x01,0x40,0x00,0x00|[Read Memory]  Address:0x4000|Read data from 0x4000|
|0x06,0x00,0x00,0x41|[Compare]  Data:0x40|Compare with previously read data; skip next instruction if 0x40|
|0x0a,0x00,0x00,0xfe|[JMP] -2|Jump back 2 instructions (loop until 0x4000 becomes 0x40)|
|0xff,0x00,0x00,0x00|End|End script|

## Debugging

You can obtain execution logs from the GROVE port (UART) by executing the "SDBGON" command.
However, enabling this feature will slow down execution, so it is normally disabled.

The serial port uses 3.3V logic levels, and the wiring configuration is as described below.
![UART](./IMAGE/uart.jpg)

### Output Example

```CMD_DEBUGOUT
BRCV_OK20
SKIP : 0a
BIN End
BSCR,0
0: Write Memory 5555:aa
4: Write Memory 2aaa:55
8: Write Memory 5555:a0
12: Write Memory 4000:41
16: Read Memory 4000:ff
20: Compare ff=41
24: JMP
16: Read Memory 4000:ff
20: Compare ff=41
24: JMP
16: Read Memory 4000:ff
20: Compare ff=41
24: JMP
16: Read Memory 4000:ff
20: Compare ff=41
24: JMP
16: Read Memory 4000:41
20: Compare 41=41
28: End Script(PASS)
```
