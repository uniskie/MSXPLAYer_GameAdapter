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
  - During `ERROFF`, OK/FAIL display is suppressed, and only the success/failure counters are updated

## Command List (Specification Format / Implementation Compliant)

### 1) HSET - Hardware Setting

- **Function**: Configures wait times and compatibility modes for hardware access
- **Format**: `HSET,[Address],[Data]`
- **Arguments**:
  - `Address` : Setting item number (hexadecimal)
  - `Data` : Setting value (hexadecimal; if omitted, the value is restored to its default)
- **Response**: `OK` / `FAIL`
- **Notes**:
  - The current firmware supports the following settings.

| Address | Setting Item | Description | Default |
|---|---|---|---|
| `0` | `MEMWAIT` | Wait time after memory read/write (`n x 10ns`) | `0` |
| `1` | `RDWAIT` | `/RD` signal width during memory read (`n x 10ns`) | `100` |
| `2` | `WRWAIT` | `/WR` signal width during memory write (`n x 10ns`) | `18` |
| `3` | `P6MODE` | PC-6001 16KB mode setting (`0`: OFF / `1` or higher: ON) | `0` |

- **Examples**:
  - `HSET,1,64` : Sets `RDWAIT` to `1000ns = 1us`
  - `HSET,1` : Restores `RDWAIT` to its default value

- **Supplement**:
  - The current values can be checked with the `HINF` command.
  - The output of `HINF` includes `MEMWAIT` / `RDWAIT` / `WRWAIT` / `P6MODE`.
  - Specifying an unsupported `Address` results in `FAIL`.

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
    Displays the type of line feed code in the input COMMAND string (1 = `"\n"` = treated as 0x0d,0x0a)
  - `COMDBG`: Debug output setting via UART
    When set to 1, debug output is enabled on UART
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
- **Notes**: The response is a string in SLOT3210 order. A slot with an inserted cartridge is shown as 1.

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

- **Function**: Powers off slot, returns signals to a safe state
- **Format**: `SPOFF`
- **Arguments**: None
- **Response**: `OK/FAIL`
- **Notes**:

---

### 13) SRST - Slot Reset

- **Function**: Toggles the slot RESET signal to reset the slot
- **Format**: `SRST`
- **Arguments**: None
- **Response**: `OK`

---

### 14) SSEL - Slot Select

- **Function**: Sets the default slot number
- **Format**: `SSEL,[Slot]`
- **Arguments**:
  - `Slot` : `1` or `2`
- **Response**: `OK/FAIL`
- **Notes**:
  - Omitting `Slot` results in `FAIL`
  - This value is used when the slot is omitted afterward

---

### 15) SMRD - Slot Memory Read (1 byte)

- **Function**: Reads 1 byte from slot memory
- **Format**: `SMRD,[Address](,[Slot])`
- **Arguments**:
  - `Address` : 0000〜FFFF Slot memory address
  - `Slot` : 1 or 2 (uses defaultSlot if omitted)
- **Response**:
  - `<addr> : <data>` (e.g. `1000 : 3F`) + `OK/FAIL`

---

### 16) SMWR - Slot Memory Write (1 byte)

- **Function**: Writes 1 byte to slot memory
- **Format**: `SMWR,[Address],[Data](,[Slot])`
- **Arguments**:
  - `Address` : 0000〜FFFF Slot memory address
  - `Data` : 00〜FF Write data
  - `Slot` : 1 or 2 (uses defaultSlot if omitted)
- **Response**: `OK/FAIL`

---

### 17) SMTR - Slot → Buffer Transfer Read (Bulk Read)

- **Function**: Continuous read from slot to buffer
- **Format**: `SMTR,[Address](,[Length],[BufferAddress],[Slot])`
- **Arguments**:
  - `Address` : 0000〜FFFF Slot-side start address
  - `Length` : 0000〜FFFF Read length (maximum if omitted)
  - `BufferAddress` : 0000〜FFFF Buffer storage start position (0 if omitted)
  - `Slot` : 1 or 2 (uses defaultSlot if omitted)
- **Response**: `OK/FAIL`

---

### 18) SMTW - Buffer → Slot Transfer Write (Bulk Write)

- **Function**: Continuous write from buffer to slot
- **Format**: `SMTW,[Address],[Length],[BufferAddress](,[Slot])`
- **Arguments**:
  - `Address` : 0000〜FFFF Slot-side start address
  - `Length` : 0000〜FFFF Write length
  - `BufferAddress` : Buffer read start position
  - `Slot` : 1 or 2 (uses defaultSlot if omitted)
- **Response**: `OK/FAIL`

---

### 19) IORD - IO Read (1 byte)

- **Function**: Reads 1 byte from IO port
- **Format**: `IORD,[IO]`
- **Arguments**:
  - `IO` : 0000〜FFFF IO address (16-bit)
- **Response**:
  - `<io> : <data>` + `OK/FAIL`

---

### 20) IOWR - IO Write (1 byte)

- **Function**: Writes 1 byte to IO port
- **Format**: `IOWR,[IO],[Data]`
- **Arguments**:
  - `IO` : 0000〜FFFF IO address
  - `Data` : 00〜FF Write data
- **Response**: `OK/FAIL`

---

### 21) IOTR - IO → Buffer Transfer Read (Implementation Compliant)

- **Function**: Continuous read from IO and stores it into the buffer
- **Format**: `IOTR,[IO],[Length],[BufferAddress]`
- **Arguments (Implementation Compliant)**:
  - `IO` : 0000〜FFFF Start IO address
  - `Length` : 0000〜FFFF Number of reads (bytes)
  - `BufferAddress` : Buffer storage start position
- **Response**: `OK/FAIL`

---

### 22) IOTW - Buffer → IO Transfer Write (Implementation Compliant)

- **Function**: Continuous write from buffer to IO
- **Format**: `IOTW,[IO],[Length],[BufferAddress]`
- **Arguments (Implementation Compliant)**:
  - `IO` : 0000〜FFFF Start IO address
  - `Length` : 0000〜FFFF Number of writes (bytes)
  - `BufferAddress` : 0000〜FFFF Buffer read start position
- **Response**: `OK/FAIL`

---

### 23) BDMP - Buffer Dump (Debug Use)

- **Function**: Displays buffer contents in HEX+ASCII
- **Format**: `BDMP,[BufferAddress](,[Length])`
- **Arguments**:
  - `BufferAddress` : 0000〜FFFF Start position (0 if omitted)
  - `Length` : 0000〜FFFF Display length (128 if omitted)
- **Response**: Dump lines + `OK`

---

### 24) SDMP - Slot Dump (Slot Memory Dump)

- **Function**: Reads directly from the slot while displaying a dump
- **Format**: `SDMP,[Address](,[Length],[Slot])`
- **Arguments**:
  - `Address` : 0000〜FFFF Start address (0 if omitted)
  - `Length` : 0000〜FFFF Display length (128 if omitted)
  - `Slot` : 1 or 2 (uses defaultSlot if omitted)
- **Response**: Dump lines + `OK/FAIL`

---

### 25) BSCR - Buffer Script Execute (Script Execution)

- **Function**: Executes a script on the buffer
- **Format**: `BSCR,[BufferAddress](,[Slot])`
- **Arguments**:
  - `BufferAddress` : 0000〜FFFF Script start (0 if omitted)
  - `Slot` : 1 or 2 (uses defaultSlot if omitted)
- **Response**: `OK/FAIL`
- **Notes**:
  - Instruction format is 4 bytes per instruction: `[cmd][addr_hi][addr_lo][data]`
  - See the separate section for script details

---

### 26) FTEST - Factory Test

- **Function**: Executes comprehensive factory tests
- **Format**: `FTEST`
- **Arguments**: None
- **Response**: Test logs + `OK/FAIL`

---

### 27) LEDRDY / LEDPON / LEDACC - LED Color Setting

- **Function**: Sets LED colors (READY / POWER ON / SLOT ACCESS) in RGB
- **Format**:
  - `LEDRDY(,[R],[G],[B])`
  - `LEDPON(,[R],[G],[B])`
  - `LEDACC(,[R],[G],[B])`
- **Arguments**:
  - `R` `G` `B` : 0x00〜0xFF (optional; default values are used if omitted)
- **Response**: `OK`

---

### 28) SDBGON - Serial Debug Log ON

- **Function**: Enables debug log output from the serial port
- **Format**: `SDBGON`
- **Arguments**: None
- **Response**: `OK`
- **Notes**:
  - Execution speed decreases because serial output increases.
  - Be careful of buffer overflows.
  - Serial output is provided through the GROVE connector.

---

### 29) _FFU - Bootloader Launch

- **Function**: Switches to FFU mode and enters USB boot mode
- **Format**: `_FFU`
- **Arguments**: None
- **Response**: Outputs a message and then reboots (no subsequent `OK/FAIL` is returned)

---

### 30) LSCR - Set Maximum LOOP Count for Script Mode

- **Function**: Sets the maximum number of loops for waiting on a condition in Script mode
- **Format**: `LSCR(,Maximum LOOP Count)`
- **Arguments**: 0-0xFFFF (default is 1000 when omitted)
- **Response**: `OK`

---

[Added in 260520_VER]

### 31) SMTH - Slot → Buffer Transfer Read (Bulk Read) + HASH

- **Function**: Continuously reads from the slot into the buffer and calculates a 32-bit hash code
- **Format**: `SMTH,[Address](,[Length],[BufferAddress],[Slot])`
- **Arguments**:
  - `Address` : 0000〜FFFF Slot-side start address
  - `Length` : 0000〜FFFF Read length (maximum if omitted)
  - `BufferAddress` : 0000〜FFFF Buffer storage start position (0 if omitted)
  - `Slot` : 1 or 2 (uses defaultSlot if omitted)
- **Response**: `<Length> : <32-bit HASH>` + `OK/FAIL`

---

## Serial Command Examples

### Example 1: Read 0x0000-0x3FFF from the slot and send it to the PC

```CMD_EX1
SPON
SMTR,0000,4000,0000,01
BSND,0000,4000
SPOFF
```

### Example 2: Receive binary data from the PC and write it to 0x8000

```CMD_EX2
SPON
BRCV,0000,2000
(Send 0x2000 bytes of binary data here)
SMTW,8000,2000,0000,01
SPOFF
```

## Script Mode

By using a script placed in the buffer, this mode allows data read/write and comparisons
to be executed without communicating with the PC.

## Script Format

- Script format: [Command],[Address(2Byte)],[DATA]...
- Each script is fixed-length at 4 bytes.

### Script Data Format

|Address|+0 Byte|+1 Byte|+2 Byte|+3 Byte|
|---|---|---|---|---|
|**DATA**|Instruction Code|Upper Address|Lower Address|Data|

### Instruction List

|Command|Instruction Name|Description|
|---|---|---|
|0x00|NOP|Do nothing|
|0x01|Read Memory|Executes a read from the slot|
|0x02|Write Memory|Writes DATA to the slot (LastData is overwritten with DATA)|
|0x03|Read IO|Executes an IO read from the slot (LastData becomes the read DATA)|
|0x04|Write IO|Executes an IO write to the slot (LastData is overwritten with DATA)|
|0x05|Wait|Waits for the time specified by address in milliseconds|
|0x06|Compare|Compares LastData with data; if they match, the next instruction is skipped|
|0x07|AND|Calculates LastData [AND] DATA; if the result is 0x00, the next instruction is skipped|
|0x08|OR|Calculates LastData [OR] DATA; if the result is 0x00, the next instruction is skipped|
|0x09|XOR|Calculates LastData [XOR] DATA; if the result is 0x00, the next instruction is skipped|
|0x0A|JMP|Skips instructions by the amount in DATA (`0x00-7F` = forward / `0x80-FF` = backward)|
|0x0B|PUSH|Writes lastData to the buffer memory location specified by address|
|0xFE|Abort|Ends script execution with failure|
|0xFF|End|Ends script execution with success|

*The JMP instruction causes script failure if it is executed at the same location a certain number of times (default: 1000).*  
*This value can be changed with the `LSCR` command.*

### Script Example

This can be executed with the following command:

```CMD_BRCV
BRCV,0,20
(Send the following binary)
BSCR,0
```

|Binary Value|Command|Description|
|---|---|---|
|0x02,0x55,0x55,0xaa|[Write Memory] Address:0x5555 Data:0xaa|Write 0xAA to 0x5555|
|0x02,0xaa,0xaa,0x55|[Write Memory] Address:0xaaaa Data:0x55|Write 0x55 to 0xAAAA|
|0x02,0x55,0x55,0xa0|[Write Memory] Address:0x5555 Data:0xa0|Write 0xA0 to 0x5555|
|0x02,0x40,0x00,0x41|[Write Memory] Address:0x4000 Data:0x41|Write 0x41 to 0x4000|
|0x01,0x40,0x00,0x00|[Read Memory] Address:0x4000|Read data from 0x4000|
|0x06,0x00,0x00,0x41|[Compare] Data:0x40|Compare with the previously read data; if it is 0x40, skip the next instruction|
|0x0a,0x00,0x00,0xfe|[JMP] -2|Go back two instructions (that is, loop until 0x4000 becomes 0x40)|
|0xff,0x00,0x00,0x00|End|End script|

## Debugging

By executing the `SDBGON` command, execution logs can be obtained from the GROVE connector (UART).  
However, enabling this feature slows execution, so it should normally remain disabled.

The serial port uses 3.3V levels, and the wiring is as shown below.  
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
