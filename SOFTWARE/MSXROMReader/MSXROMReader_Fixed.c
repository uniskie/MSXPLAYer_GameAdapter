// MSXROMReader.c - MSX ROM Reader Program (FIXED VERSION)
// Purpose: Read ROM data from MSX cartridges via serial communication
// Supports: MegaROM mappers detection and ROM dump

#include <windows.h>
#include <setupapi.h>
#include <stdio.h>
#include <tchar.h>
#include <winioctl.h>
#include <string.h>
#include <conio.h>
#include <stdlib.h>

// ============================================================================
// Constants and Defines
// ============================================================================

#define BANK_SIZE           0x2000  // 8K bank size
#define SLOT_ADDR_BASE      0x4000  // Slot base address
#define HASH_SIZE           7936    // Hash calculation size
#define MAX_RESPONSE_LEN    256
#define TIMEOUT_MS          5000
#define SRAM_THRESHOLD      8       // Number of identical banks to detect SRAM

// MegaROM Mapper Types
typedef enum {
    MAPPER_UNKNOWN = 0,
    MAPPER_ASCII_16K,               // ASCII 16K
    MAPPER_ASCII_8K,                // ASCII 8K
    MAPPER_KONAMI_8K,               // Konami 8K
    MAPPER_KONAMI_SCC,              // Konami SCC
    MAPPER_GENERIC_16K,             // Generic 16K
    MAPPER_GENERIC_8K,              // Generic 8K
    MAPPER_HAL_NOTE,                // Hal Note
    MAPPER_NO_MAPPER_16K,           // 16KB no mapper
    MAPPER_NO_MAPPER_32K,           // 32KB no mapper
    MAPPER_NO_MAPPER_48K            // 48KB no mapper
} MAPPER_TYPE;

// ROM Information Structure
typedef struct {
    MAPPER_TYPE mapperType;
    const char* mapperName;
    DWORD romSize;                  // in bytes
    DWORD bankCount;                // number of banks
    BOOL hasSRAM;                   // has SRAM
    DWORD validDataStart;           // start of valid data
    DWORD validDataSize;            // size of valid data
    DWORD readBankSize;             // size per bank read
    DWORD readAreaStart;            // start address for reading
    DWORD readAreaSize;             // size of read area
} ROM_INFO;

// ============================================================================
// Serial Communication Functions
// ============================================================================

static BOOL ConfigureSerialPort(HANDLE hSerial)
{
    if (!SetupComm(hSerial, 65536, 65536))
    {
        puts("SetupComm error");
        return FALSE;
    }

    DCB dcbSerialParam = { sizeof(DCB) };
    if (!GetCommState(hSerial, &dcbSerialParam))
    {
        puts("GetCommState error");
        return FALSE;
    }

    PurgeComm(hSerial, PURGE_TXABORT | PURGE_RXABORT | PURGE_TXCLEAR | PURGE_RXCLEAR);

    dcbSerialParam.BaudRate = CBR_115200;
    dcbSerialParam.ByteSize = 8;
    dcbSerialParam.Parity = NOPARITY;
    dcbSerialParam.StopBits = ONESTOPBIT;
    dcbSerialParam.fBinary = TRUE;
    dcbSerialParam.fOutxCtsFlow = FALSE;
    dcbSerialParam.fOutxDsrFlow = FALSE;
    dcbSerialParam.fDtrControl = DTR_CONTROL_DISABLE;
    dcbSerialParam.fRtsControl = RTS_CONTROL_DISABLE;
    dcbSerialParam.fOutX = FALSE;
    dcbSerialParam.fInX = FALSE;
    dcbSerialParam.fNull = FALSE;
    dcbSerialParam.fErrorChar = FALSE;

    if (!SetCommState(hSerial, &dcbSerialParam))
    {
        puts("SetCommState error");
        return FALSE;
    }

    COMMTIMEOUTS timeout;
    timeout.ReadIntervalTimeout = 100;
    timeout.ReadTotalTimeoutMultiplier = 0;
    timeout.ReadTotalTimeoutConstant = 500;
    timeout.WriteTotalTimeoutMultiplier = 0;
    timeout.WriteTotalTimeoutConstant = 500;

    if (!SetCommTimeouts(hSerial, &timeout))
    {
        puts("SetCommTimeouts error");
        return FALSE;
    }

    return TRUE;
}

static BOOL SendCommand(HANDLE hSerial, const char* command)
{
    DWORD sendsize;
    char tmpstr[512];
    sprintf_s(tmpstr, sizeof(tmpstr), "%s\r\n", command);
    
    if (!WriteFile(hSerial, tmpstr, (DWORD)strlen(tmpstr), &sendsize, NULL))
        return FALSE;
    
    FlushFileBuffers(hSerial);
    return TRUE;
}

static BOOL RecvResponse(HANDLE hSerial, char* response, size_t maxlen, DWORD timeoutMs)
{
    DWORD bytesRead;
    size_t idx = 0;
    COMMTIMEOUTS timeout;
    DWORD startTime, currentTime;

    timeout.ReadIntervalTimeout = 50;
    timeout.ReadTotalTimeoutMultiplier = 0;
    timeout.ReadTotalTimeoutConstant = 50;
    timeout.WriteTotalTimeoutMultiplier = 0;
    timeout.WriteTotalTimeoutConstant = 1000;
    SetCommTimeouts(hSerial, &timeout);

    memset(response, 0, maxlen);
    startTime = GetTickCount();

    while (1)
    {
        char ch;
        if (!ReadFile(hSerial, &ch, 1, &bytesRead, NULL))
            return FALSE;

        if (bytesRead == 0)
        {
            currentTime = GetTickCount();
            if (timeoutMs > 0 && currentTime - startTime >= timeoutMs)
            {
                printf("Response timeout\n");
                return FALSE;
            }
            continue;
        }

        startTime = GetTickCount();

        if (idx < maxlen - 1)
        {
            response[idx++] = ch;
            response[idx] = '\0';
        }
        else
        {
            for (size_t i = 0; i < maxlen - 2; i++)
            {
                response[i] = response[i + 1];
            }
            response[maxlen - 2] = ch;
            response[maxlen - 1] = '\0';
        }

        printf("%c", ch);
        fflush(stdout);

        if (idx >= 2 && strstr(response, "OK") != NULL)
            return TRUE;

        if (idx >= 4 && strstr(response, "FAIL") != NULL)
            return FALSE;
    }

    return FALSE;
}

static BOOL RecvBinaryBlock(HANDLE hSerial, BYTE* recvbuf, DWORD recvsize)
{
    DWORD bytesRead = 0;
    DWORD totalRead = 0;
    COMMTIMEOUTS timeout;

    timeout.ReadIntervalTimeout = 50;
    timeout.ReadTotalTimeoutMultiplier = 0;
    timeout.ReadTotalTimeoutConstant = 1000;
    timeout.WriteTotalTimeoutMultiplier = 0;
    timeout.WriteTotalTimeoutConstant = 1000;
    SetCommTimeouts(hSerial, &timeout);

    memset(recvbuf, 0, recvsize);

    while (totalRead < recvsize)
    {
        if (!ReadFile(hSerial, recvbuf + totalRead, recvsize - totalRead, &bytesRead, NULL))
        {
            if (bytesRead == 0)
                break;
            return FALSE;
        }
        if (bytesRead == 0)
            break;
        totalRead += bytesRead;
    }

    return (totalRead == recvsize);
}

static BOOL SendBinary(HANDLE hSerial, const BYTE* data, DWORD size)
{
    DWORD sentSize;
    if (!WriteFile(hSerial, data, size, &sentSize, NULL))
        return FALSE;
    FlushFileBuffers(hSerial);
    return (sentSize == size);
}

// ============================================================================
// Cartridge Control Functions
// ============================================================================

/**
 * Check if cartridge is properly inserted
 * Command: SCHK
 * Response: 0x0 format (e.g., "0010") + OK
 * Returns: TRUE if response contains "0010", FALSE otherwise
 */
static BOOL SlotCheck(HANDLE hSerial)
{
    char response[MAX_RESPONSE_LEN];

    printf("Checking cartridge insertion...\n");
    
    if (!SendCommand(hSerial, "SCHK"))
    {
        printf("Failed to send SCHK command\n");
        return FALSE;
    }

    if (!RecvResponse(hSerial, response, sizeof(response), TIMEOUT_MS))
    {
        printf("Failed to receive SCHK response\n");
        return FALSE;
    }

    printf("\n");

    // Check if response contains "0010"
    if (strstr(response, "0010") != NULL)
    {
        printf("Cartridge is properly inserted\n");
        return TRUE;
    }

    printf("ERROR: Cartridge is not properly inserted\n");
    return FALSE;
}

/**
 * Power ON the slot
 * Command: SPON
 * Response: OK or FAIL
 * Returns: TRUE if response is "OK", FALSE otherwise
 */
static BOOL SlotPowerOn(HANDLE hSerial)
{
    char response[MAX_RESPONSE_LEN];

    printf("Turning on slot power...\n");
    
    if (!SendCommand(hSerial, "SPON"))
    {
        printf("Failed to send SPON command\n");
        return FALSE;
    }

    if (!RecvResponse(hSerial, response, sizeof(response), TIMEOUT_MS))
    {
        printf("Failed to receive SPON response\n");
        return FALSE;
    }

    printf("\n");

    // Check if response contains "OK"
    if (strstr(response, "OK") != NULL)
    {
        printf("Slot power turned on successfully\n");
        return TRUE;
    }

    printf("ERROR: Failed to turn on slot power\n");
    return FALSE;
}

/**
 * Power OFF the slot
 * Command: SPOFF
 * Response: OK or FAIL
 * Returns: TRUE if response is "OK", FALSE otherwise
 */
static BOOL SlotPowerOff(HANDLE hSerial)
{
    char response[MAX_RESPONSE_LEN];

    printf("Turning off slot power...\n");
    
    if (!SendCommand(hSerial, "SPOFF"))
    {
        printf("Failed to send SPOFF command\n");
        return FALSE;
    }

    if (!RecvResponse(hSerial, response, sizeof(response), TIMEOUT_MS))
    {
        printf("Failed to receive SPOFF response\n");
        return FALSE;
    }

    printf("\n");

    // Check if response contains "OK"
    if (strstr(response, "OK") != NULL)
    {
        printf("Slot power turned off successfully\n");
        return TRUE;
    }

    printf("ERROR: Failed to turn off slot power\n");
    return FALSE;
}

// ============================================================================
// Slot Access Functions
// ============================================================================

static BOOL slotWrite(HANDLE hSerial, DWORD address, BYTE data)
{
    char command[128];
    char response[MAX_RESPONSE_LEN];

    sprintf_s(command, sizeof(command), "SMWR,%04lX,%02X", address, data);

    if (!SendCommand(hSerial, command))
        return FALSE;

    if (!RecvResponse(hSerial, response, sizeof(response), TIMEOUT_MS))
        return FALSE;

    printf("\n");
    return TRUE;
}

static BOOL slotRead(HANDLE hSerial, DWORD address, BYTE* data)
{
    char command[128];
    char response[MAX_RESPONSE_LEN];

    sprintf_s(command, sizeof(command), "SMRD,%04lX", address);

    if (!SendCommand(hSerial, command))
        return FALSE;

    if (!RecvResponse(hSerial, response, sizeof(response), TIMEOUT_MS))
        return FALSE;

    printf("\n");

    if (sscanf_s(response, "%02hhX", data) == 1)
        return TRUE;

    return FALSE;
}

static BOOL slotDump(HANDLE hSerial, DWORD address, DWORD length, BYTE* data)
{
    char command[128];
    char response[MAX_RESPONSE_LEN];

    sprintf_s(command, sizeof(command), "SMTR,%04lX,%04lX", address, length);

    if (!SendCommand(hSerial, command))
        return FALSE;

    if (!RecvResponse(hSerial, response, sizeof(response), TIMEOUT_MS))
        return FALSE;

    printf("\n");

    sprintf_s(command, sizeof(command), "BSND,0,%04lX", length);

    if (!SendCommand(hSerial, command))
        return FALSE;

    if (!RecvBinaryBlock(hSerial, data, length))
        return FALSE;

    printf("\nReceived %lu bytes\n", length);

    if (!RecvResponse(hSerial, response, sizeof(response), TIMEOUT_MS))
        return FALSE;

    printf("\n");
    return TRUE;
}

// ============================================================================
// Hash Calculation
// ============================================================================

static DWORD Hash7936(const BYTE* data, DWORD address)
{
    DWORD hash = 0x5381;
    DWORD i;
    
    if (address + HASH_SIZE > 0xC000)
        return 0;

    for (i = 0; i < HASH_SIZE; i++)
    {
        hash = ((hash << 5) + hash) ^ data[address + i];
    }

    return hash;
}

// ============================================================================
// ASCII 16K Mapper Detection
// ============================================================================

static BOOL DetectASCII16K(HANDLE hSerial, ROM_INFO* romInfo)
{
    printf("\n--- Testing ASCII 16K ---\n");

    BYTE dataBuffer[0x8000];
    DWORD hashA[2], hashB[2];
    DWORD prevHashA[2];
    DWORD bankNum;
    DWORD maxBank = 0;
    int identicalCount = 0;
    DWORD sramStartBank = 0;
    BOOL foundPattern = FALSE;

    printf("Setting bank 0 (0x6000=0, 0x6800=0, 0x7000=1, 0x7800=1)\n");
    if (!slotWrite(hSerial, 0x6000, 0)) return FALSE;
    if (!slotWrite(hSerial, 0x6800, 0)) return FALSE;
    if (!slotWrite(hSerial, 0x7000, 1)) return FALSE;
    if (!slotWrite(hSerial, 0x7800, 1)) return FALSE;

    Sleep(10);

    if (!slotDump(hSerial, 0x4000, 0x8000, dataBuffer))
        return FALSE;

    hashA[0] = Hash7936(dataBuffer, 0x4000);
    hashA[1] = Hash7936(dataBuffer, 0x6000);

    printf("Bank 0: Hash[0x8000]=%08lX, Hash[0xA000]=%08lX\n", hashA[0], hashA[1]);
    memcpy(prevHashA, hashA, sizeof(hashA));
    identicalCount = 0;

    for (bankNum = 1; bankNum < 256; bankNum++)
    {
        if (!slotWrite(hSerial, 0x6000, (BYTE)bankNum)) return FALSE;
        if (!slotWrite(hSerial, 0x6800, 0)) return FALSE;
        if (!slotWrite(hSerial, 0x7000, (BYTE)(bankNum + 1))) return FALSE;
        if (!slotWrite(hSerial, 0x7800, 1)) return FALSE;

        Sleep(10);

        if (!slotDump(hSerial, 0x4000, 0x8000, dataBuffer))
            return FALSE;

        hashB[0] = Hash7936(dataBuffer, 0x0000);
        hashB[1] = Hash7936(dataBuffer, 0x2000);

        printf("Bank %lu: Hash[0x4000]=%08lX, Hash[0x6000]=%08lX", bankNum, hashB[0], hashB[1]);

        if (hashA[0] == hashB[0] && hashA[1] == hashB[1])
        {
            printf(" (pattern match - cycle detected)\n");
            maxBank = bankNum - 1;
            foundPattern = TRUE;
            break;
        }

        if (hashA[0] == prevHashA[0] && hashA[1] == prevHashA[1])
        {
            if (identicalCount == 0)
                sramStartBank = bankNum;
            identicalCount++;

            if (identicalCount >= SRAM_THRESHOLD)
            {
                printf(" (SRAM detected)\n");
                maxBank = sramStartBank - 1;
                romInfo->hasSRAM = TRUE;
                foundPattern = TRUE;
                break;
            }
        }
        else
        {
            identicalCount = 0;
        }

        printf("\n");
        memcpy(prevHashA, hashA, sizeof(hashA));
        memcpy(hashA, hashB, sizeof(hashB));
    }

    // ✅ 修正: パターンが見つかった場合のみ TRUE を返す
    if (foundPattern && maxBank > 0)
    {
        printf("\n=== ASCII 16K Detected ===\n");
        romInfo->mapperType = MAPPER_ASCII_16K;
        romInfo->mapperName = "ASCII 16K";
        romInfo->bankCount = maxBank + 1;
        romInfo->romSize = romInfo->bankCount * 0x4000;
        romInfo->readBankSize = 0x4000;
        romInfo->readAreaStart = 0x8000;
        romInfo->readAreaSize = 0x4000;
        printf("Bank count: %lu, ROM size: %lu (0x%lX)\n", romInfo->bankCount, romInfo->romSize, romInfo->romSize);
        return TRUE;
    }

    return FALSE;
}

// ============================================================================
// ASCII 8K Mapper Detection
// ============================================================================

static BOOL DetectASCII8K(HANDLE hSerial, ROM_INFO* romInfo)
{
    printf("\n--- Testing ASCII 8K ---\n");

    BYTE dataBuffer[0x8000];
    DWORD hashA[3], hashB[3];
    DWORD prevHashA[3];
    DWORD bankNum;
    DWORD maxBank = 0;
    int identicalCount = 0;
    DWORD sramStartBank = 0;
    BOOL foundPattern = FALSE;

    printf("Setting bank 0 (0x6000=0, 0x6800=1, 0x7000=2, 0x7800=3)\n");
    if (!slotWrite(hSerial, 0x6000, 0)) return FALSE;
    if (!slotWrite(hSerial, 0x6800, 1)) return FALSE;
    if (!slotWrite(hSerial, 0x7000, 2)) return FALSE;
    if (!slotWrite(hSerial, 0x7800, 3)) return FALSE;

    Sleep(10);

    if (!slotDump(hSerial, 0x4000, 0x8000, dataBuffer))
        return FALSE;

    hashA[0] = Hash7936(dataBuffer, 0x2000);
    hashA[1] = Hash7936(dataBuffer, 0x4000);
    hashA[2] = Hash7936(dataBuffer, 0x6000);

    printf("Bank 0: Hash[0x6000]=%08lX, Hash[0x8000]=%08lX, Hash[0xA000]=%08lX\n", 
           hashA[0], hashA[1], hashA[2]);
    memcpy(prevHashA, hashA, sizeof(hashA));
    identicalCount = 0;

    for (bankNum = 1; bankNum < 256; bankNum++)
    {
        if (!slotWrite(hSerial, 0x6000, (BYTE)bankNum)) return FALSE;
        if (!slotWrite(hSerial, 0x6800, (BYTE)(bankNum + 1))) return FALSE;
        if (!slotWrite(hSerial, 0x7000, (BYTE)(bankNum + 2))) return FALSE;
        if (!slotWrite(hSerial, 0x7800, (BYTE)(bankNum + 3))) return FALSE;

        Sleep(10);

        if (!slotDump(hSerial, 0x4000, 0x8000, dataBuffer))
            return FALSE;

        hashB[0] = Hash7936(dataBuffer, 0x0000);
        hashB[1] = Hash7936(dataBuffer, 0x2000);
        hashB[2] = Hash7936(dataBuffer, 0x4000);

        printf("Bank %lu: Hash[0x4000]=%08lX, Hash[0x6000]=%08lX, Hash[0x8000]=%08lX", 
               bankNum, hashB[0], hashB[1], hashB[2]);

        if (hashA[0] == hashB[0] && hashA[1] == hashB[1] && hashA[2] == hashB[2])
        {
            printf(" (pattern match - cycle detected)\n");
            maxBank = bankNum - 1;
            foundPattern = TRUE;
            break;
        }

        if (hashA[0] == prevHashA[0] && hashA[1] == prevHashA[1] && hashA[2] == prevHashA[2])
        {
            if (identicalCount == 0)
                sramStartBank = bankNum;
            identicalCount++;

            if (identicalCount >= SRAM_THRESHOLD)
            {
                printf(" (SRAM detected)\n");
                maxBank = sramStartBank - 1;
                romInfo->hasSRAM = TRUE;
                foundPattern = TRUE;
                break;
            }
        }
        else
        {
            identicalCount = 0;
        }

        printf("\n");
        memcpy(prevHashA, hashA, sizeof(hashA));
        memcpy(hashA, hashB, sizeof(hashB));
    }

    // ✅ 修正: パターンが見つかった場合のみ TRUE を返す
    if (foundPattern && maxBank > 0)
    {
        printf("\n=== ASCII 8K Detected ===\n");
        romInfo->mapperType = MAPPER_ASCII_8K;
        romInfo->mapperName = "ASCII 8K";
        romInfo->bankCount = maxBank + 1;
        romInfo->romSize = romInfo->bankCount * 0x2000;
        romInfo->readBankSize = 0x2000;
        romInfo->readAreaStart = 0x8000;
        romInfo->readAreaSize = 0x2000;
        printf("Bank count: %lu, ROM size: %lu (0x%lX)\n", romInfo->bankCount, romInfo->romSize, romInfo->romSize);
        return TRUE;
    }

    return FALSE;
}

// ============================================================================
// KONAMI 8K Mapper Detection
// ============================================================================

static BOOL DetectKONAMI8K(HANDLE hSerial, ROM_INFO* romInfo)
{
    printf("\n--- Testing KONAMI 8K ---\n");

    BYTE dataBuffer[0x8000];
    DWORD hashA[3], hashB[3];
    DWORD prevHashA[3];
    DWORD bankNum;
    DWORD maxBank = 0;
    int identicalCount = 0;
    DWORD sramStartBank = 0;
    BOOL foundPattern = FALSE;

    printf("Setting bank 0\n");
    if (!slotWrite(hSerial, 0x4000, 0)) return FALSE;
    if (!slotWrite(hSerial, 0x6000, 1)) return FALSE;
    if (!slotWrite(hSerial, 0x8000, 2)) return FALSE;
    if (!slotWrite(hSerial, 0xA000, 3)) return FALSE;
    if (!slotWrite(hSerial, 0x5000, 0)) return FALSE;
    if (!slotWrite(hSerial, 0x7000, 1)) return FALSE;
    if (!slotWrite(hSerial, 0x9000, 2)) return FALSE;
    if (!slotWrite(hSerial, 0xB000, 3)) return FALSE;

    Sleep(10);

    if (!slotDump(hSerial, 0x4000, 0x8000, dataBuffer))
        return FALSE;

    hashA[0] = Hash7936(dataBuffer, 0x0000);
    hashA[1] = Hash7936(dataBuffer, 0x4000);
    hashA[2] = Hash7936(dataBuffer, 0x6000);

    printf("Bank 0: Hash[0x4000]=%08lX, Hash[0x8000]=%08lX, Hash[0xA000]=%08lX\n", 
           hashA[0], hashA[1], hashA[2]);
    memcpy(prevHashA, hashA, sizeof(hashA));
    identicalCount = 0;

    for (bankNum = 1; bankNum <= 0x1F; bankNum++)
    {
        if (!slotWrite(hSerial, 0x4000, (BYTE)bankNum)) return FALSE;
        if (!slotWrite(hSerial, 0x6000, (BYTE)(bankNum + 1))) return FALSE;
        if (!slotWrite(hSerial, 0x8000, (BYTE)(bankNum + 2))) return FALSE;
        if (!slotWrite(hSerial, 0xA000, (BYTE)(bankNum + 3))) return FALSE;
        if (!slotWrite(hSerial, 0x5000, 0)) return FALSE;
        if (!slotWrite(hSerial, 0x7000, 1)) return FALSE;
        if (!slotWrite(hSerial, 0x9000, 2)) return FALSE;
        if (!slotWrite(hSerial, 0xB000, 3)) return FALSE;

        Sleep(10);

        if (!slotDump(hSerial, 0x4000, 0x8000, dataBuffer))
            return FALSE;

        hashB[0] = Hash7936(dataBuffer, 0x0000);
        hashB[1] = Hash7936(dataBuffer, 0x2000);
        hashB[2] = Hash7936(dataBuffer, 0x4000);

        printf("Bank %lu: Hash[0x4000]=%08lX, Hash[0x6000]=%08lX, Hash[0x8000]=%08lX", 
               bankNum, hashB[0], hashB[1], hashB[2]);

        if (hashA[0] == hashB[0] && hashA[1] == hashB[1] && hashA[2] == hashB[2])
        {
            printf(" (pattern match - cycle detected)\n");
            maxBank = bankNum - 1;
            foundPattern = TRUE;
            break;
        }

        if (hashA[0] == prevHashA[0] && hashA[1] == prevHashA[1] && hashA[2] == prevHashA[2])
        {
            if (identicalCount == 0)
                sramStartBank = bankNum;
            identicalCount++;

            if (identicalCount >= SRAM_THRESHOLD)
            {
                printf(" (SRAM detected)\n");
                maxBank = sramStartBank - 1;
                romInfo->hasSRAM = TRUE;
                foundPattern = TRUE;
                break;
            }
        }
        else
        {
            identicalCount = 0;
        }

        printf("\n");
        memcpy(prevHashA, hashA, sizeof(hashA));
        memcpy(hashA, hashB, sizeof(hashB));
    }

    // ✅ 修正: パターンが見つかった場合のみ TRUE を返す
    if (foundPattern && maxBank > 0)
    {
        printf("\n=== KONAMI 8K Detected ===\n");
        if (romInfo->hasSRAM)
            printf("(with SRAM)\n");
        romInfo->mapperType = MAPPER_KONAMI_8K;
        romInfo->mapperName = romInfo->hasSRAM ? "KONAMI 8K (with SRAM)" : "KONAMI 8K";
        romInfo->bankCount = maxBank + 1;
        romInfo->romSize = romInfo->bankCount * 0x2000;
        romInfo->readBankSize = 0x2000;
        romInfo->readAreaStart = 0x8000;
        romInfo->readAreaSize = 0x2000;
        printf("Bank count: %lu, ROM size: %lu (0x%lX)\n", romInfo->bankCount, romInfo->romSize, romInfo->romSize);
        return TRUE;
    }

    return FALSE;
}

// ============================================================================
// KONAMI SCC Mapper Detection
// ============================================================================

static BOOL DetectKONAMI_SCC(HANDLE hSerial, ROM_INFO* romInfo)
{
    printf("\n--- Testing KONAMI SCC ---\n");

    BYTE dataBuffer[0x8000];
    DWORD hashA[3], hashB[3];
    DWORD prevHashA[3];
    DWORD bankNum;
    DWORD maxBank = 0;
    BOOL foundPattern = FALSE;

    printf("Setting bank 0\n");
    if (!slotWrite(hSerial, 0x5000, 0)) return FALSE;
    if (!slotWrite(hSerial, 0x7000, 1)) return FALSE;
    if (!slotWrite(hSerial, 0x9000, 2)) return FALSE;
    if (!slotWrite(hSerial, 0xB000, 3)) return FALSE;
    if (!slotWrite(hSerial, 0x5800, 0)) return FALSE;
    if (!slotWrite(hSerial, 0x7800, 1)) return FALSE;
    if (!slotWrite(hSerial, 0x9800, 2)) return FALSE;
    if (!slotWrite(hSerial, 0xB800, 3)) return FALSE;

    Sleep(10);

    if (!slotDump(hSerial, 0x4000, 0x8000, dataBuffer))
        return FALSE;

    hashA[0] = Hash7936(dataBuffer, 0x2000);
    hashA[1] = Hash7936(dataBuffer, 0x4000);
    hashA[2] = Hash7936(dataBuffer, 0x6000);

    printf("Bank 0: Hash[0x6000]=%08lX, Hash[0x8000]=%08lX, Hash[0xA000]=%08lX\n", 
           hashA[0], hashA[1], hashA[2]);
    memcpy(prevHashA, hashA, sizeof(hashA));

    for (bankNum = 1; bankNum <= 0x3F; bankNum++)
    {
        if (!slotWrite(hSerial, 0x5000, (BYTE)bankNum)) return FALSE;
        if (!slotWrite(hSerial, 0x7000, (BYTE)(bankNum + 1))) return FALSE;
        if (!slotWrite(hSerial, 0x9000, (BYTE)(bankNum + 2))) return FALSE;
        if (!slotWrite(hSerial, 0xB000, (BYTE)(bankNum + 3))) return FALSE;
        if (!slotWrite(hSerial, 0x5800, 0)) return FALSE;
        if (!slotWrite(hSerial, 0x7800, 1)) return FALSE;
        if (!slotWrite(hSerial, 0x9800, 2)) return FALSE;
        if (!slotWrite(hSerial, 0xB800, 3)) return FALSE;

        Sleep(10);

        if (!slotDump(hSerial, 0x4000, 0x8000, dataBuffer))
            return FALSE;

        hashB[0] = Hash7936(dataBuffer, 0x0000);
        hashB[1] = Hash7936(dataBuffer, 0x2000);
        hashB[2] = Hash7936(dataBuffer, 0x4000);

        printf("Bank %lu: Hash[0x4000]=%08lX, Hash[0x6000]=%08lX, Hash[0x8000]=%08lX", 
               bankNum, hashB[0], hashB[1], hashB[2]);

        if (hashA[0] == hashB[0] && hashA[1] == hashB[1] && hashA[2] == hashB[2])
        {
            printf(" (pattern match - cycle detected)\n");
            maxBank = bankNum - 1;
            foundPattern = TRUE;
            break;
        }

        printf("\n");
        memcpy(prevHashA, hashA, sizeof(hashA));
        memcpy(hashA, hashB, sizeof(hashB));
    }

    // ✅ 修正: パターンが見つかった場合のみ TRUE を返す
    if (foundPattern && maxBank > 0)
    {
        printf("\n=== KONAMI SCC Detected ===\n");
        romInfo->mapperType = MAPPER_KONAMI_SCC;
        romInfo->mapperName = "KONAMI SCC";
        romInfo->bankCount = maxBank + 1;
        romInfo->romSize = romInfo->bankCount * 0x2000;
        romInfo->readBankSize = 0x2000;
        romInfo->readAreaStart = 0x8000;
        romInfo->readAreaSize = 0x2000;
        printf("Bank count: %lu, ROM size: %lu (0x%lX)\n", romInfo->bankCount, romInfo->romSize, romInfo->romSize);
        return TRUE;
    }

    return FALSE;
}

// ============================================================================
// Generic 16K Mapper Detection
// ============================================================================

static BOOL DetectGeneric16K(HANDLE hSerial, ROM_INFO* romInfo)
{
    printf("\n--- Testing Generic 16K ---\n");

    BYTE dataBuffer[0x8000];
    DWORD hashA[2], hashB[2];
    DWORD bankNum;
    DWORD maxBank = 0;
    BOOL foundPattern = FALSE;

    printf("Setting bank 0 (0x4000=0, 0x8000=1)\n");
    if (!slotWrite(hSerial, 0x4000, 0)) return FALSE;
    if (!slotWrite(hSerial, 0x8000, 1)) return FALSE;

    Sleep(10);

    if (!slotDump(hSerial, 0x4000, 0x8000, dataBuffer))
        return FALSE;

    hashA[0] = Hash7936(dataBuffer, 0x4000);
    hashA[1] = Hash7936(dataBuffer, 0x6000);

    printf("Bank 0: Hash[0x8000]=%08lX, Hash[0xA000]=%08lX\n", hashA[0], hashA[1]);

    for (bankNum = 1; bankNum < 256; bankNum++)
    {
        if (!slotWrite(hSerial, 0x4000, (BYTE)bankNum)) return FALSE;
        if (!slotWrite(hSerial, 0x8000, (BYTE)(bankNum + 1))) return FALSE;

        Sleep(10);

        if (!slotDump(hSerial, 0x4000, 0x8000, dataBuffer))
            return FALSE;

        hashB[0] = Hash7936(dataBuffer, 0x0000);
        hashB[1] = Hash7936(dataBuffer, 0x2000);

        printf("Bank %lu: Hash[0x4000]=%08lX, Hash[0x6000]=%08lX", bankNum, hashB[0], hashB[1]);

        if (hashA[0] == hashB[0] && hashA[1] == hashB[1])
        {
            printf(" (pattern match - cycle detected)\n");
            maxBank = bankNum - 1;
            foundPattern = TRUE;
            break;
        }

        printf("\n");
        memcpy(hashA, hashB, sizeof(hashA));
    }

    // ✅ 修正: パターンが見つかった場合のみ TRUE を返す
    if (foundPattern && maxBank > 0)
    {
        printf("\n=== Generic 16K Detected ===\n");
        romInfo->mapperType = MAPPER_GENERIC_16K;
        romInfo->mapperName = "Generic 16K";
        romInfo->bankCount = maxBank + 1;
        romInfo->romSize = romInfo->bankCount * 0x4000;
        romInfo->readBankSize = 0x4000;
        romInfo->readAreaStart = 0x8000;
        romInfo->readAreaSize = 0x4000;
        printf("Bank count: %lu, ROM size: %lu (0x%lX)\n", romInfo->bankCount, romInfo->romSize, romInfo->romSize);
        return TRUE;
    }

    return FALSE;
}

// ============================================================================
// Generic 8K Mapper Detection
// ============================================================================

static BOOL DetectGeneric8K(HANDLE hSerial, ROM_INFO* romInfo)
{
    printf("\n--- Testing Generic 8K ---\n");

    BYTE dataBuffer[0x8000];
    DWORD hashA[3], hashB[3];
    DWORD bankNum;
    DWORD maxBank = 0;
    BOOL foundPattern = FALSE;

    printf("Setting bank 0 (0x4000=0, 0x6000=1, 0x8000=2, 0xA000=3)\n");
    if (!slotWrite(hSerial, 0x4000, 0)) return FALSE;
    if (!slotWrite(hSerial, 0x6000, 1)) return FALSE;
    if (!slotWrite(hSerial, 0x8000, 2)) return FALSE;
    if (!slotWrite(hSerial, 0xA000, 3)) return FALSE;

    Sleep(10);

    if (!slotDump(hSerial, 0x4000, 0x8000, dataBuffer))
        return FALSE;

    hashA[0] = Hash7936(dataBuffer, 0x2000);
    hashA[1] = Hash7936(dataBuffer, 0x4000);
    hashA[2] = Hash7936(dataBuffer, 0x6000);

    printf("Bank 0: Hash[0x6000]=%08lX, Hash[0x8000]=%08lX, Hash[0xA000]=%08lX\n", 
           hashA[0], hashA[1], hashA[2]);

    for (bankNum = 1; bankNum <= 0x3F; bankNum++)
    {
        if (!slotWrite(hSerial, 0x4000, (BYTE)bankNum)) return FALSE;
        if (!slotWrite(hSerial, 0x6000, (BYTE)(bankNum + 1))) return FALSE;
        if (!slotWrite(hSerial, 0x8000, (BYTE)(bankNum + 2))) return FALSE;
        if (!slotWrite(hSerial, 0xA000, (BYTE)(bankNum + 3))) return FALSE;

        Sleep(10);

        if (!slotDump(hSerial, 0x4000, 0x8000, dataBuffer))
            return FALSE;

        hashB[0] = Hash7936(dataBuffer, 0x0000);
        hashB[1] = Hash7936(dataBuffer, 0x2000);
        hashB[2] = Hash7936(dataBuffer, 0x4000);

        printf("Bank %lu: Hash[0x4000]=%08lX, Hash[0x6000]=%08lX, Hash[0x8000]=%08lX", 
               bankNum, hashB[0], hashB[1], hashB[2]);

        if (hashA[0] == hashB[0] && hashA[1] == hashB[1] && hashA[2] == hashB[2])
        {
            printf(" (pattern match - cycle detected)\n");
            maxBank = bankNum - 1;
            foundPattern = TRUE;
            break;
        }

        printf("\n");
        memcpy(hashA, hashB, sizeof(hashA));
    }

    // ✅ 修正: パターンが見つかった場合のみ TRUE を返す
    if (foundPattern && maxBank > 0)
    {
        printf("\n=== Generic 8K Detected ===\n");
        romInfo->mapperType = MAPPER_GENERIC_8K;
        romInfo->mapperName = "Generic 8K";
        romInfo->bankCount = maxBank + 1;
        romInfo->romSize = romInfo->bankCount * 0x2000;
        romInfo->readBankSize = 0x2000;
        romInfo->readAreaStart = 0x8000;
        romInfo->readAreaSize = 0x2000;
        printf("Bank count: %lu, ROM size: %lu (0x%lX)\n", romInfo->bankCount, romInfo->romSize, romInfo->romSize);
        return TRUE;
    }

    return FALSE;
}

// ============================================================================
// HAL NOTE Mapper Detection
// ============================================================================

static BOOL DetectHAL_NOTE(HANDLE hSerial, ROM_INFO* romInfo)
{
    printf("\n--- Testing HAL NOTE ---\n");

    BYTE dataBuffer[0x8000];
    DWORD hashA[3], hashB[3];
    DWORD bankNum;
    BOOL foundPattern = FALSE;

    printf("Setting bank 0 (0x4FFF=0, 0x6FFF=1, 0x8FFF=2, 0xAFFF=3)\n");
    if (!slotWrite(hSerial, 0x4FFF, 0)) return FALSE;
    if (!slotWrite(hSerial, 0x6FFF, 1)) return FALSE;
    if (!slotWrite(hSerial, 0x8FFF, 2)) return FALSE;
    if (!slotWrite(hSerial, 0xAFFF, 3)) return FALSE;

    Sleep(10);

    if (!slotDump(hSerial, 0x4000, 0x8000, dataBuffer))
        return FALSE;

    hashA[0] = Hash7936(dataBuffer, 0x2000);
    hashA[1] = Hash7936(dataBuffer, 0x4000);
    hashA[2] = Hash7936(dataBuffer, 0x6000);

    printf("Bank 0: Hash[0x6000]=%08lX, Hash[0x8000]=%08lX, Hash[0xA000]=%08lX\n", 
           hashA[0], hashA[1], hashA[2]);

    for (bankNum = 1; bankNum <= 0x7F; bankNum++)
    {
        if (!slotWrite(hSerial, 0x4FFF, (BYTE)bankNum)) return FALSE;
        if (!slotWrite(hSerial, 0x6FFF, (BYTE)(bankNum + 1))) return FALSE;
        if (!slotWrite(hSerial, 0x8FFF, (BYTE)(bankNum + 2))) return FALSE;
        if (!slotWrite(hSerial, 0xAFFF, (BYTE)(bankNum + 3))) return FALSE;

        Sleep(10);

        if (!slotDump(hSerial, 0x4000, 0x8000, dataBuffer))
            return FALSE;

        hashB[0] = Hash7936(dataBuffer, 0x0000);
        hashB[1] = Hash7936(dataBuffer, 0x2000);
        hashB[2] = Hash7936(dataBuffer, 0x4000);

        printf("Bank %lu: Hash[0x4000]=%08lX, Hash[0x6000]=%08lX, Hash[0x8000]=%08lX", 
               bankNum, hashB[0], hashB[1], hashB[2]);

        if (hashA[0] == hashB[0] && hashA[1] == hashB[1] && hashA[2] == hashB[2])
        {
            printf(" (pattern match - cycle detected)\n");
            foundPattern = TRUE;
            break;
        }

        printf("\n");
        memcpy(hashA, hashB, sizeof(hashB));
    }

    // ✅ 修正: HAL NOTE はループが 0x7F に達したことで検出
    if (foundPattern || bankNum > 0x7F)
    {
        printf("\n=== HAL NOTE Detected ===\n");
        romInfo->mapperType = MAPPER_HAL_NOTE;
        romInfo->mapperName = "HAL NOTE";
        romInfo->bankCount = 0x80;
        romInfo->romSize = romInfo->bankCount * 0x2000;
        romInfo->readBankSize = 0x2000;
        romInfo->readAreaStart = 0x8000;
        romInfo->readAreaSize = 0x2000;
        printf("Bank count: %lu, ROM size: %lu (0x%lX)\n", romInfo->bankCount, romInfo->romSize, romInfo->romSize);
        return TRUE;
    }

    return FALSE;
}

// ============================================================================
// Standard ROM Detection
// ============================================================================

static BOOL DetectStandardROM(HANDLE hSerial, ROM_INFO* romInfo)
{
    BYTE fullDataBuffer[0xC000];

    printf("\n=== Detecting Standard ROM Type ===\n\n");

    printf("Reading 0x0000-0xBFFF...\n");
    if (!slotDump(hSerial, 0x0000, 0xC000, fullDataBuffer))
    {
        printf("Failed to read full ROM\n");
        return FALSE;
    }

    DWORD hash0 = Hash7936(fullDataBuffer, 0x0000);
    DWORD hash2 = Hash7936(fullDataBuffer, 0x2000);
    DWORD hash4 = Hash7936(fullDataBuffer, 0x4000);
    DWORD hash6 = Hash7936(fullDataBuffer, 0x6000);
    DWORD hash8 = Hash7936(fullDataBuffer, 0x8000);
    DWORD hashA = Hash7936(fullDataBuffer, 0xA000);

    printf("\nFull ROM hash analysis:\n");
    printf("  0x0000: %08lX\n", hash0);
    printf("  0x2000: %08lX\n", hash2);
    printf("  0x4000: %08lX\n", hash4);
    printf("  0x6000: %08lX\n", hash6);
    printf("  0x8000: %08lX\n", hash8);
    printf("  0xA000: %08lX\n", hashA);

    if ((hash0 == hash4 && hash4 == hash8) && (hash2 == hash6 && hash6 == hashA))
    {
        printf("\n=== 16KB Standard ROM Detected ===\n");
        printf("Pattern: 0x0000=0x4000=0x8000, 0x2000=0x6000=0xA000\n");
        romInfo->mapperType = MAPPER_NO_MAPPER_16K;
        romInfo->mapperName = "16KB ROM";
        romInfo->romSize = 0x4000;
        romInfo->validDataStart = 0x4000;
        romInfo->validDataSize = 0x4000;
        return TRUE;
    }

    if ((hash4 == hash8) && (hash6 == hashA))
    {
        BOOL allFF = TRUE;
        for (DWORD i = 0; i < 0x4000; i++)
        {
            if (fullDataBuffer[i] != 0xFF)
            {
                allFF = FALSE;
                break;
            }
        }

        if (allFF)
        {
            printf("\n=== 16KB Standard ROM Detected ===\n");
            printf("Pattern: 0x4000=0x8000, 0x6000=0xA000, 0x0000-0x3FFF all 0xFF\n");
            romInfo->mapperType = MAPPER_NO_MAPPER_16K;
            romInfo->mapperName = "16KB ROM";
            romInfo->romSize = 0x4000;
            romInfo->validDataStart = 0x4000;
            romInfo->validDataSize = 0x4000;
            return TRUE;
        }
    }

    if (((hash0 == hash4) || (hash0 == hash8)) &&
        ((hash2 == hash6) || (hash2 == hashA)) &&
        (hash4 == hash8) && (hash6 == hashA))
    {
        printf("\n=== 32KB Standard ROM Detected ===\n");
        printf("Pattern: Mirror pattern with 0x4000=0x8000, 0x6000=0xA000\n");
        romInfo->mapperType = MAPPER_NO_MAPPER_32K;
        romInfo->mapperName = "32KB ROM";
        romInfo->romSize = 0x8000;
        romInfo->validDataStart = 0x4000;
        romInfo->validDataSize = 0x8000;
        return TRUE;
    }

    if ((hash4 == hash8) && (hash6 == hashA))
    {
        BOOL allFF = TRUE;
        for (DWORD i = 0; i < 0x4000; i++)
        {
            if (fullDataBuffer[i] != 0xFF)
            {
                allFF = FALSE;
                break;
            }
        }

        if (allFF)
        {
            printf("\n=== 32KB Standard ROM Detected ===\n");
            printf("Pattern: 0x0000-0x3FFF all 0xFF, 0x4000=0x8000, 0x6000=0xA000\n");
            romInfo->mapperType = MAPPER_NO_MAPPER_32K;
            romInfo->mapperName = "32KB ROM";
            romInfo->romSize = 0x8000;
            romInfo->validDataStart = 0x4000;
            romInfo->validDataSize = 0x8000;
            return TRUE;
        }
    }

    printf("\n=== 48KB Standard ROM Detected ===\n");
    printf("Pattern: No specific pattern matched (default)\n");
    romInfo->mapperType = MAPPER_NO_MAPPER_48K;
    romInfo->mapperName = "48KB ROM";
    romInfo->romSize = 0xC000;
    romInfo->validDataStart = 0x0000;
    romInfo->validDataSize = 0xC000;
    return TRUE;
}

// ============================================================================
// ROM Reading and Saving Functions
// ============================================================================

static BOOL ReadASCII16K(HANDLE hSerial, ROM_INFO* romInfo, BYTE* outData)
{
    printf("\n=== Reading ASCII 16K ROM ===\n\n");

    BYTE buffer[0x4000];
    DWORD bytesWritten = 0;

    for (DWORD bank = 0; bank < romInfo->bankCount; bank++)
    {
        if (!slotWrite(hSerial, 0x7000, (BYTE)bank)) return FALSE;

        Sleep(10);

        if (!slotDump(hSerial, romInfo->readAreaStart, romInfo->readAreaSize, buffer))
        {
            printf("Failed to read bank %lu\n", bank);
            return FALSE;
        }

        memcpy(outData + bytesWritten, buffer, romInfo->readBankSize);
        bytesWritten += romInfo->readBankSize;

        printf("Saved bank %lu (0x%04lX - 0x%04lX)\n", bank, bytesWritten - romInfo->readBankSize, bytesWritten - 1);
    }

    printf("\nTotal bytes read: %lu (0x%lX)\n", bytesWritten, bytesWritten);
    return TRUE;
}

static BOOL ReadASCII8K(HANDLE hSerial, ROM_INFO* romInfo, BYTE* outData)
{
    printf("\n=== Reading ASCII 8K ROM ===\n\n");

    BYTE buffer[0x2000];
    DWORD bytesWritten = 0;

    for (DWORD bank = 0; bank < romInfo->bankCount; bank++)
    {
        if (!slotWrite(hSerial, 0x7000, (BYTE)bank)) return FALSE;

        Sleep(10);

        if (!slotDump(hSerial, romInfo->readAreaStart, romInfo->readAreaSize, buffer))
        {
            printf("Failed to read bank %lu\n", bank);
            return FALSE;
        }

        memcpy(outData + bytesWritten, buffer, romInfo->readBankSize);
        bytesWritten += romInfo->readBankSize;

        printf("Saved bank %lu (0x%04lX - 0x%04lX)\n", bank, bytesWritten - romInfo->readBankSize, bytesWritten - 1);
    }

    printf("\nTotal bytes read: %lu (0x%lX)\n", bytesWritten, bytesWritten);
    return TRUE;
}

static BOOL ReadKONAMI8K(HANDLE hSerial, ROM_INFO* romInfo, BYTE* outData)
{
    printf("\n=== Reading KONAMI 8K ROM ===\n\n");

    BYTE buffer[0x2000];
    DWORD bytesWritten = 0;

    for (DWORD bank = 0; bank < romInfo->bankCount; bank++)
    {
        if (!slotWrite(hSerial, 0x8000, (BYTE)bank)) return FALSE;

        Sleep(10);

        if (!slotDump(hSerial, romInfo->readAreaStart, romInfo->readAreaSize, buffer))
        {
            printf("Failed to read bank %lu\n", bank);
            return FALSE;
        }

        memcpy(outData + bytesWritten, buffer, romInfo->readBankSize);
        bytesWritten += romInfo->readBankSize;

        printf("Saved bank %lu (0x%04lX - 0x%04lX)\n", bank, bytesWritten - romInfo->readBankSize, bytesWritten - 1);
    }

    printf("\nTotal bytes read: %lu (0x%lX)\n", bytesWritten, bytesWritten);
    return TRUE;
}

static BOOL ReadKONAMI_SCC(HANDLE hSerial, ROM_INFO* romInfo, BYTE* outData)
{
    printf("\n=== Reading KONAMI SCC ROM ===\n\n");

    BYTE buffer[0x2000];
    DWORD bytesWritten = 0;

    for (DWORD bank = 0; bank < romInfo->bankCount; bank++)
    {
        if (!slotWrite(hSerial, 0x8000, (BYTE)bank)) return FALSE;

        Sleep(10);

        if (!slotDump(hSerial, romInfo->readAreaStart, romInfo->readAreaSize, buffer))
        {
            printf("Failed to read bank %lu\n", bank);
            return FALSE;
        }

        memcpy(outData + bytesWritten, buffer, romInfo->readBankSize);
        bytesWritten += romInfo->readBankSize;

        printf("Saved bank %lu (0x%04lX - 0x%04lX)\n", bank, bytesWritten - romInfo->readBankSize, bytesWritten - 1);
    }

    printf("\nTotal bytes read: %lu (0x%lX)\n", bytesWritten, bytesWritten);
    return TRUE;
}

static BOOL ReadGeneric16K(HANDLE hSerial, ROM_INFO* romInfo, BYTE* outData)
{
    printf("\n=== Reading Generic 16K ROM ===\n\n");

    BYTE buffer[0x4000];
    DWORD bytesWritten = 0;

    for (DWORD bank = 0; bank < romInfo->bankCount; bank++)
    {
        if (!slotWrite(hSerial, 0x4000, (BYTE)bank)) return FALSE;
        if (!slotWrite(hSerial, 0x8000, (BYTE)(bank + 1))) return FALSE;

        Sleep(10);

        if (!slotDump(hSerial, romInfo->readAreaStart, romInfo->readAreaSize, buffer))
        {
            printf("Failed to read bank %lu\n", bank);
            return FALSE;
        }

        memcpy(outData + bytesWritten, buffer, romInfo->readBankSize);
        bytesWritten += romInfo->readBankSize;

        printf("Saved bank %lu (0x%04lX - 0x%04lX)\n", bank, bytesWritten - romInfo->readBankSize, bytesWritten - 1);
    }

    printf("\nTotal bytes read: %lu (0x%lX)\n", bytesWritten, bytesWritten);
    return TRUE;
}

static BOOL ReadGeneric8K(HANDLE hSerial, ROM_INFO* romInfo, BYTE* outData)
{
    printf("\n=== Reading Generic 8K ROM ===\n\n");

    BYTE buffer[0x2000];
    DWORD bytesWritten = 0;

    for (DWORD bank = 0; bank < romInfo->bankCount; bank++)
    {
        if (!slotWrite(hSerial, 0x4000, (BYTE)bank)) return FALSE;
        if (!slotWrite(hSerial, 0x6000, (BYTE)(bank + 1))) return FALSE;
        if (!slotWrite(hSerial, 0x8000, (BYTE)(bank + 2))) return FALSE;
        if (!slotWrite(hSerial, 0xA000, (BYTE)(bank + 3))) return FALSE;

        Sleep(10);

        if (!slotDump(hSerial, romInfo->readAreaStart, romInfo->readAreaSize, buffer))
        {
            printf("Failed to read bank %lu\n", bank);
            return FALSE;
        }

        memcpy(outData + bytesWritten, buffer, romInfo->readBankSize);
        bytesWritten += romInfo->readBankSize;

        printf("Saved bank %lu (0x%04lX - 0x%04lX)\n", bank, bytesWritten - romInfo->readBankSize, bytesWritten - 1);
    }

    printf("\nTotal bytes read: %lu (0x%lX)\n", bytesWritten, bytesWritten);
    return TRUE;
}

static BOOL ReadHAL_NOTE(HANDLE hSerial, ROM_INFO* romInfo, BYTE* outData)
{
    printf("\n=== Reading HAL NOTE ROM ===\n\n");

    BYTE buffer[0x2000];
    DWORD bytesWritten = 0;

    for (DWORD bank = 0; bank < romInfo->bankCount; bank++)
    {
        if (!slotWrite(hSerial, 0x4FFF, (BYTE)bank)) return FALSE;
        if (!slotWrite(hSerial, 0x6FFF, (BYTE)(bank + 1))) return FALSE;
        if (!slotWrite(hSerial, 0x8FFF, (BYTE)(bank + 2))) return FALSE;
        if (!slotWrite(hSerial, 0xAFFF, (BYTE)(bank + 3))) return FALSE;

        Sleep(10);

        if (!slotDump(hSerial, romInfo->readAreaStart, romInfo->readAreaSize, buffer))
        {
            printf("Failed to read bank %lu\n", bank);
            return FALSE;
        }

        memcpy(outData + bytesWritten, buffer, romInfo->readBankSize);
        bytesWritten += romInfo->readBankSize;

        printf("Saved bank %lu (0x%04lX - 0x%04lX)\n", bank, bytesWritten - romInfo->readBankSize, bytesWritten - 1);
    }

    printf("\nTotal bytes read: %lu (0x%lX)\n", bytesWritten, bytesWritten);
    return TRUE;
}

static BOOL ReadStandardROM(HANDLE hSerial, ROM_INFO* romInfo, BYTE* outData)
{
    printf("\n=== Reading Standard ROM ===\n\n");

    printf("Reading standard ROM: 0x%04lX - 0x%04lX (%lu bytes)\n",
           romInfo->validDataStart, romInfo->validDataStart + romInfo->validDataSize - 1,
           romInfo->validDataSize);

    if (!slotDump(hSerial, romInfo->validDataStart, romInfo->validDataSize, outData))
    {
        printf("Failed to read ROM\n");
        return FALSE;
    }

    printf("\nTotal bytes read: %lu (0x%lX)\n", romInfo->validDataSize, romInfo->validDataSize);
    return TRUE;
}

static BOOL ReadCompleteROM(HANDLE hSerial, ROM_INFO* romInfo, BYTE* outData)
{
    printf("\n========== READING ROM DATA ==========\n");

    switch (romInfo->mapperType)
    {
        case MAPPER_ASCII_16K:
            return ReadASCII16K(hSerial, romInfo, outData);
        case MAPPER_ASCII_8K:
            return ReadASCII8K(hSerial, romInfo, outData);
        case MAPPER_KONAMI_8K:
            return ReadKONAMI8K(hSerial, romInfo, outData);
        case MAPPER_KONAMI_SCC:
            return ReadKONAMI_SCC(hSerial, romInfo, outData);
        case MAPPER_GENERIC_16K:
            return ReadGeneric16K(hSerial, romInfo, outData);
        case MAPPER_GENERIC_8K:
            return ReadGeneric8K(hSerial, romInfo, outData);
        case MAPPER_HAL_NOTE:
            return ReadHAL_NOTE(hSerial, romInfo, outData);
        case MAPPER_NO_MAPPER_16K:
        case MAPPER_NO_MAPPER_32K:
        case MAPPER_NO_MAPPER_48K:
            return ReadStandardROM(hSerial, romInfo, outData);
        default:
            printf("Unknown mapper type\n");
            return FALSE;
    }
}

static BOOL SaveROMToFile(const wchar_t* filename, const BYTE* data, DWORD size)
{
    printf("\n=== Saving ROM to File ===\n\n");

    HANDLE hFile = CreateFileW(filename, GENERIC_WRITE, 0, NULL, CREATE_ALWAYS,
                               FILE_ATTRIBUTE_NORMAL, NULL);
    if (hFile == INVALID_HANDLE_VALUE)
    {
        wprintf(L"Failed to create output file: %s\n", filename);
        return FALSE;
    }

    DWORD bytesWritten;
    if (!WriteFile(hFile, data, size, &bytesWritten, NULL))
    {
        printf("Failed to write file\n");
        CloseHandle(hFile);
        return FALSE;
    }

    CloseHandle(hFile);

    wprintf(L"ROM saved successfully: %s (%lu bytes)\n", filename, bytesWritten);
    return TRUE;
}

// ============================================================================
// Main Mapper Detection
// ============================================================================

static BOOL DetectMapper(HANDLE hSerial, ROM_INFO* romInfo)
{
    printf("\n========== ROM DETECTION START ==========\n");

    printf("\n=== Testing MegaROM Mappers ===\n");

    if (DetectASCII16K(hSerial, romInfo))
        return TRUE;

    if (DetectASCII8K(hSerial, romInfo))
        return TRUE;

    if (DetectKONAMI8K(hSerial, romInfo))
        return TRUE;

    if (DetectKONAMI_SCC(hSerial, romInfo))
        return TRUE;

    if (DetectGeneric16K(hSerial, romInfo))
        return TRUE;

    if (DetectGeneric8K(hSerial, romInfo))
        return TRUE;

    if (DetectHAL_NOTE(hSerial, romInfo))
        return TRUE;

    if (DetectStandardROM(hSerial, romInfo))
        return TRUE;

    printf("\nROM detection failed\n");
    return FALSE;
}

// ============================================================================
// Main Processing
// ============================================================================

int ProcessROMRead(const wchar_t* outputFile)
{
    HDEVINFO hDevInfo = SetupDiGetClassDevs(&GUID_DEVINTERFACE_COMPORT, 0, 0,
                                            DIGCF_PRESENT | DIGCF_DEVICEINTERFACE);
    if (!hDevInfo)
    {
        printf("Failed to get device info\n");
        return 1;
    }

    SP_DEVINFO_DATA data = { sizeof(data) };
    BOOL foundDevice = FALSE;

    for (int count = 0; SetupDiEnumDeviceInfo(hDevInfo, count, &data); count++)
    {
        HKEY key = SetupDiOpenDevRegKey(hDevInfo, &data, DICS_FLAG_GLOBAL, 0,
                                        DIREG_DEV, KEY_QUERY_VALUE);
        if (!key)
            continue;

        WCHAR devdesc[256], portname[256];
        DWORD size = sizeof(portname);

        if (!SetupDiGetDeviceRegistryProperty(hDevInfo, &data, SPDRP_HARDWAREID, NULL,
                                             (LPBYTE)devdesc, sizeof(devdesc), NULL))
        {
            RegCloseKey(key);
            continue;
        }

        if (wcsncmp(L"USB\\", devdesc, 4) != 0)
        {
            RegCloseKey(key);
            continue;
        }

        if (RegQueryValueExW(key, L"PortName", NULL, NULL, (LPBYTE)portname, &size) != ERROR_SUCCESS)
        {
            RegCloseKey(key);
            continue;
        }

        RegCloseKey(key);

        wprintf(L"Found USB COM port: %s\n\n", portname);

        wchar_t fullPortName[32];
        swprintf_s(fullPortName, sizeof(fullPortName) / sizeof(wchar_t), L"\\\\.\\%s", portname);

        HANDLE hSerial = CreateFileW(fullPortName, GENERIC_READ | GENERIC_WRITE, 0, 0,
                                    OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, 0);
        if (hSerial == INVALID_HANDLE_VALUE)
        {
            printf("Failed to open COM port\n");
            continue;
        }

        if (!ConfigureSerialPort(hSerial))
        {
            CloseHandle(hSerial);
            continue;
        }

        wprintf(L"Connected to %s\n\n", portname);

        // ✅ 追加: カセット挿入確認
        if (!SlotCheck(hSerial))
        {
            printf("ERROR: Cartridge is not properly inserted\n\n");
            CloseHandle(hSerial);
            continue;
        }

        // ✅ 追加: 電源ON
        if (!SlotPowerOn(hSerial))
        {
            printf("ERROR: Failed to power on slot\n\n");
            CloseHandle(hSerial);
            continue;
        }

        // ROM検出
        ROM_INFO romInfo = { 0 };
        if (!DetectMapper(hSerial, &romInfo))
        {
            printf("Mapper detection failed\n\n");
            // ✅ Error時も電源OFF
            SlotPowerOff(hSerial);
            CloseHandle(hSerial);
            continue;
        }

        printf("\n========== DETECTION RESULT ==========\n");
        printf("Detected: %s\n", romInfo.mapperName);
        printf("Bank Count: %lu\n", romInfo.bankCount);
        printf("ROM Size: %lu bytes (0x%lX)\n", romInfo.romSize, romInfo.romSize);

        BYTE* romData = (BYTE*)malloc(romInfo.romSize);
        if (!romData)
        {
            printf("Memory allocation failed\n");
            SlotPowerOff(hSerial);
            CloseHandle(hSerial);
            continue;
        }

        if (!ReadCompleteROM(hSerial, &romInfo, romData))
        {
            printf("ROM reading failed\n");
            free(romData);
            SlotPowerOff(hSerial);
            CloseHandle(hSerial);
            continue;
        }

        if (!SaveROMToFile(outputFile, romData, romInfo.romSize))
        {
            printf("File save failed\n");
            free(romData);
            SlotPowerOff(hSerial);
            CloseHandle(hSerial);
            continue;
        }

        printf("\nROM read and save completed successfully!\n\n");

        // ✅ 追加: 正常終了時も電源OFF
        SlotPowerOff(hSerial);

        free(romData);
        CloseHandle(hSerial);
        foundDevice = TRUE;
        break;
    }

    SetupDiDestroyDeviceInfoList(hDevInfo);

    if (!foundDevice)
    {
        printf("No suitable device found\n");
        return 1;
    }

    return 0;
}

// ============================================================================
// Entry Point
// ============================================================================

int wmain(int argc, wchar_t* argv[])
{
    // ✅ バナーメッセージ表示
    printf("MSX Game Adapter ROM Dumper\n");
    printf("Copyright @v9938\n");
    printf("Build: %s %s\n", __DATE__, __TIME__);
    printf("\n");

    if (argc < 2)
    {
        wprintf(L"Usage: %s <output_file_path>\n", argv[0]);
        wprintf(L"  <output_file_path>  Path where ROM will be saved\n");
        return 1;
    }

    int result = ProcessROMRead(argv[1]);

    printf("Done.\n");
    return result;
}
