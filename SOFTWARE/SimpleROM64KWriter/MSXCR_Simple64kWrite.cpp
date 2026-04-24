// MSXCR_Simple64kWrite.cpp : MSXシステムへのバイナリファイル転送・検証プログラム
//

#include <windows.h>
#include <setupapi.h>
#include <stdio.h>
#include <tchar.h>
#include <winioctl.h>
#include <string.h>
#include <conio.h>

// プロトタイプ宣言
int comList(const wchar_t* binFile);

// シリアル通信関数群

static BOOL SendCom(HANDLE hSerial, const char* sendstr)
{
	DWORD sendsize;
	char tmpstr[256];
	sprintf_s(tmpstr, sizeof(tmpstr), "%s\r\n", sendstr);
	if (!WriteFile(hSerial, tmpstr, (DWORD)strlen(tmpstr), &sendsize, NULL))
		return FALSE;
	FlushFileBuffers(hSerial);
	return TRUE;
}

// 1文字ずつ受信してOKまたはFAILを待つ（指定秒数のタイムアウト付き、データなしの場合のみタイムアウト）
static BOOL RecvResponse(HANDLE hSerial, char* response, size_t maxlen, DWORD timeoutMs)
{
	DWORD bytesRead;
	size_t idx = 0;
	COMMTIMEOUTS timeout;
	DWORD startTime, currentTime;

	// タイムアウト設定（データなしの場合のみ）
	timeout.ReadIntervalTimeout = 50;
	timeout.ReadTotalTimeoutMultiplier = 0;
	timeout.ReadTotalTimeoutConstant = 50;  // 短めの設定で何もない場合を検知
	timeout.WriteTotalTimeoutMultiplier = 0;
	timeout.WriteTotalTimeoutConstant = 1000;
	SetCommTimeouts(hSerial, &timeout);

	memset(response, 0, maxlen);
	startTime = GetTickCount();

	// OKまたはFAILを受信するまでループ
	while (1)
	{
		char ch;
		if (!ReadFile(hSerial, &ch, 1, &bytesRead, NULL))
			return FALSE;

		if (bytesRead == 0)
		{
			// データなし - 指定秒数のタイムアウトチェック
			currentTime = GetTickCount();
			if (timeoutMs > 0 && currentTime - startTime >= timeoutMs)
			{
				printf("Response timeout (no data received for %lu ms)\n", timeoutMs);
				return FALSE;
			}
			continue;
		}

		// データ受信時は開始時間をリセット
		startTime = GetTickCount();

		// バッファサイズを超えないようにするが、OKまたはFAILの判定は保持
		if (idx < maxlen - 1)
		{
			response[idx++] = ch;
			response[idx] = '\0';
		}
		else
		{
			// バッファフルの場合は古いデータを削除して新しいデータを入れる
			for (size_t i = 0; i < maxlen - 2; i++)
			{
				response[i] = response[i + 1];
			}
			response[maxlen - 2] = ch;
			response[maxlen - 1] = '\0';
		}

		// 受信データを表示
		printf("%c", ch);
		fflush(stdout);

		if (idx >= 2 && strstr(response, "OK") != NULL)
			return TRUE;

		if (idx >= 4 && strstr(response, "FAIL") != NULL)
			return FALSE;
	}

	return FALSE;
}


static BOOL SendComRecvAck(HANDLE hSerial, char* sendstr, char* recvbuf, DWORD* recvsize)
{
	DWORD sendsize;
	char tmpstr[256];
	sprintf_s(tmpstr, sizeof(tmpstr), "%s\r\n", sendstr);
	if (!WriteFile(hSerial, tmpstr, (DWORD)strlen(tmpstr), &sendsize, NULL))
		return FALSE;
	FlushFileBuffers(hSerial);

	memset(recvbuf, 0, *recvsize);
	return ReadFile(hSerial, recvbuf, *recvsize, recvsize, NULL);
}

// コマンド送信と応答受信（タイムアウト指定版）
static BOOL SendCommandAndRecv(HANDLE hSerial, const char* command, char* response, size_t responseLen, DWORD timeoutMs)
{
	printf("Sending: %s\n", command);
	if (!SendCom(hSerial, command))
	{
		printf("COM access error\n");
		return FALSE;
	}

	if (!RecvResponse(hSerial, response, responseLen, timeoutMs))
	{
		printf("Failed to receive response\n");
		return FALSE;
	}


	// 受信結果がFAILなら FALSE を返す
	if (strstr(response, "FAIL") != NULL)
	{
		return FALSE;
	}

	return TRUE;
}


static BOOL SendCom(HANDLE hSerial, char* sendstr)
{
	DWORD sendsize;
	char tmpstr[256];
	sprintf_s(tmpstr, sizeof(tmpstr), "%s\r\n", sendstr);
	if (!WriteFile(hSerial, tmpstr, (DWORD)strlen(tmpstr), &sendsize, NULL))
		return FALSE;
	FlushFileBuffers(hSerial);
	return TRUE;
}

static BOOL SendBinary(HANDLE hSerial, const BYTE* data, DWORD size)
{
	DWORD sentSize;
	if (!WriteFile(hSerial, data, size, &sentSize, NULL))
		return FALSE;
	FlushFileBuffers(hSerial);
	return TRUE;
}

static BOOL RecvBlock(HANDLE hSerial, char* recvbuf, DWORD recvsize)
{
	DWORD tmp;
	memset(recvbuf, 0, recvsize);
	return ReadFile(hSerial, recvbuf, recvsize, &tmp, NULL);
}

// 1文字ずつ受信してOKまたはFAILを待つ
static BOOL RecvResponse(HANDLE hSerial, char* response, size_t maxlen)
{
	DWORD bytesRead;
	size_t idx = 0;
	COMMTIMEOUTS timeout;

	// タイムアウト設定（応答受信用・短め）
	timeout.ReadIntervalTimeout = 50;
	timeout.ReadTotalTimeoutMultiplier = 0;
	timeout.ReadTotalTimeoutConstant = 500;
	timeout.WriteTotalTimeoutMultiplier = 0;
	timeout.WriteTotalTimeoutConstant = 500;
	SetCommTimeouts(hSerial, &timeout);

	memset(response, 0, maxlen);

	// OKまたはFAILを受信するまでループ
	while (idx < maxlen - 1)
	{
		char ch;
		if (!ReadFile(hSerial, &ch, 1, &bytesRead, NULL))
			return FALSE;

		if (bytesRead == 0)
			continue;  // タイムアウト

		response[idx++] = ch;
		response[idx] = '\0';

		if (idx >= 2 && strstr(response, "OK") != NULL)
			return TRUE;

		if (idx >= 4 && strstr(response, "FAIL") != NULL)
			return FALSE;
	}

	return FALSE;
}

static BOOL dispResponse(BOOL ret, char* sendstr, char* recvbuf)
{
	printf("%s:\n", sendstr);
	if (ret)
		printf("%s\n\n", recvbuf);
	else
		puts("COM access error\n");
	return ret;
}

// シリアルポート設定
static BOOL ConfigureSerialPort(HANDLE hSerial)
{
	// バッファ設定
	if (!SetupComm(hSerial, 65536, 65536))
	{
		puts("SetupComm error");
		return FALSE;
	}

	// DCB設定取得
	DCB dcbSerialParam = { sizeof(DCB) };
	if (!GetCommState(hSerial, &dcbSerialParam))
	{
		puts("GetCommState error");
		return FALSE;
	}

	// バッファクリア
	PurgeComm(hSerial, PURGE_TXABORT | PURGE_RXABORT | PURGE_TXCLEAR | PURGE_RXCLEAR);

	// シリアルパラメータ設定（115200, 8-N-1）
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

	// タイムアウト設定（通常のコマンド用）
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

// フラッシュメモリ初期化・データ転送
static BOOL TransferBinaryData(HANDLE hSerial, BYTE* fileData, DWORD fileSize)
{
	char sendstr[256], recvbuf[65536];
	DWORD recvsize;
	BOOL commandSuccess = TRUE;

	// デバイス情報取得
	strcpy_s(sendstr, sizeof(sendstr), "HVER");
	recvsize = sizeof(recvbuf);
	if (!dispResponse(SendComRecvAck(hSerial, sendstr, recvbuf, &recvsize), sendstr, recvbuf))
		return FALSE;

	strcpy_s(sendstr, sizeof(sendstr), "HINF");
	recvsize = sizeof(recvbuf);
	if (!dispResponse(SendComRecvAck(hSerial, sendstr, recvbuf, &recvsize), sendstr, recvbuf))
		return FALSE;

	// デバイス接続チェック
	// SCHK - 最大3回まで0010が返ってきたらキー入力待ち（1秒）
	if (commandSuccess && !SendCommandAndRecv(hSerial, "SCHK", recvbuf, recvsize, 1000))
	{
		printf("SCHK failed\n");
		commandSuccess = FALSE;
	}
	else if (commandSuccess)
	{
		// SCHK応答を確認して、0010の場合は治具挿入待ちを行う
		int waitCount = 0;
		while (waitCount < 3 && strstr(recvbuf, "0000") != NULL)
		{
			waitCount++;
			printf("Please insert Simple ROM Cartridge (attempt %d/3)\n", waitCount);
			printf("Press any key to continue...\n");
			_getch();

			// 再度SCHKを実行（1秒）
			if (!SendCommandAndRecv(hSerial, "SCHK", recvbuf, recvsize, 1000))
			{
				printf("SCHK retry failed\n");
				commandSuccess = FALSE;
				break;
			}
		}

		// 3回待ったが0010のままの場合
		if (waitCount >= 3 && strstr(recvbuf, "0000") != NULL)
		{
			printf("Failed to insert Simple ROM Cartridge\n");
			commandSuccess = FALSE;
		}
		else if (strstr(recvbuf, "0010") == NULL && strstr(recvbuf, "FAIL") == NULL)
		{
			printf("Unexpected SCHK response\n");
			commandSuccess = FALSE;
		}
	}


	// commandSuccess が FALSE の場合はエラー
	if (!commandSuccess)
	{
		printf("Test failed on this port\n");
		return FALSE;
	}
	// DEBUG用
//	strcpy_s(sendstr, sizeof(sendstr), "SDBGON");
//	recvsize = sizeof(recvbuf);
//	if (!dispResponse(SendComRecvAck(hSerial, sendstr, recvbuf, &recvsize), sendstr, recvbuf))
//		return FALSE;

	// POWER　ON
	strcpy_s(sendstr, sizeof(sendstr), "SPON");
	recvsize = sizeof(recvbuf);
	if (!dispResponse(SendComRecvAck(hSerial, sendstr, recvbuf, &recvsize), sendstr, recvbuf))
		return FALSE;

	// フラッシュメモリ消去（AMDコマンドシーケンス）
	printf("Erasing Flash...\n");
	const char* eraseSequence[] = {
		"SMWR,5555,AA",
		"SMWR,2AAA,55",
		"SMWR,5555,80",
		"SMWR,5555,AA",
		"SMWR,2AAA,55",
		"SMWR,5555,10"
	};

	for (int i = 0; i < sizeof(eraseSequence) / sizeof(eraseSequence[0]); i++)
	{
		strcpy_s(sendstr, sizeof(sendstr), eraseSequence[i]);
		recvsize = sizeof(recvbuf);
		if (!dispResponse(SendComRecvAck(hSerial, sendstr, recvbuf, &recvsize), sendstr, recvbuf))
			return FALSE;
	}

	// フラッシュ消去完了確認（ダミー読み込み）
	strcpy_s(sendstr, sizeof(sendstr), "SMRD,4000");
	recvsize = sizeof(recvbuf);
	if (!dispResponse(SendComRecvAck(hSerial, sendstr, recvbuf, &recvsize), sendstr, recvbuf))
		return FALSE;

	// バイナリデータ送信
	printf("Sending binary data (%lu bytes)...\n", fileSize);
	DWORD baseAddress = 0x4000;
	const DWORD BUFFER_SIZE = 256;

	for (DWORD bufferOffset = 0; bufferOffset < fileSize; bufferOffset += BUFFER_SIZE)
	{
		DWORD bytesToSend = (fileSize - bufferOffset > BUFFER_SIZE) ? BUFFER_SIZE : (fileSize - bufferOffset);

		// コマンドデータサイズ計算
		DWORD cmdDataSize = 7 * 4 * bytesToSend + 4;
		sprintf_s(sendstr, sizeof(sendstr), "BRCV,0,%X", cmdDataSize);
		if (!SendCom(hSerial, sendstr))
		{
			printf("Failed to send BRCV command\n");
			return FALSE;
		}

		// コマンドシーケンス生成
		BYTE cmdSequence[8096];
		DWORD idx = 0;

		for (DWORD i = 0; i < bytesToSend; i++)
		{
			DWORD address = baseAddress + bufferOffset + i;
			BYTE dataValue = fileData[bufferOffset + i];

			// AMDフラッシュ書き込みシーケンス
			cmdSequence[idx++] = 0x02;
			cmdSequence[idx++] = 0x55;
			cmdSequence[idx++] = 0x55;
			cmdSequence[idx++] = 0xaa;

			cmdSequence[idx++] = 0x02;
			cmdSequence[idx++] = 0x2a;
			cmdSequence[idx++] = 0xaa;
			cmdSequence[idx++] = 0x55;

			cmdSequence[idx++] = 0x02;
			cmdSequence[idx++] = 0x55;
			cmdSequence[idx++] = 0x55;
			cmdSequence[idx++] = 0xa0;

			cmdSequence[idx++] = 0x02;
			cmdSequence[idx++] = (BYTE)((address >> 8) & 0xFF);
			cmdSequence[idx++] = (BYTE)(address & 0xFF);
			cmdSequence[idx++] = dataValue;

			cmdSequence[idx++] = 0x01;
			cmdSequence[idx++] = (BYTE)((address >> 8) & 0xFF);
			cmdSequence[idx++] = (BYTE)(address & 0xFF);
			cmdSequence[idx++] = 0x00;

			cmdSequence[idx++] = 0x06;
			cmdSequence[idx++] = (BYTE)((address >> 8) & 0xFF);
			cmdSequence[idx++] = (BYTE)(address & 0xFF);
			cmdSequence[idx++] = dataValue;

			cmdSequence[idx++] = 0x0a;
			cmdSequence[idx++] = 0x00;
			cmdSequence[idx++] = 0x00;
			cmdSequence[idx++] = 0xfe;
		}

		// シーケンス終了マーカー
		cmdSequence[idx++] = 0xff;
		cmdSequence[idx++] = 0x00;
		cmdSequence[idx++] = 0x00;
		cmdSequence[idx++] = 0x00;

		// コマンドシーケンス送信
		if (!SendBinary(hSerial, cmdSequence, idx))
		{
			printf("Failed to send command sequence.\n");
			return FALSE;
		}

		// 応答確認
		char dummy[256];
		if (!RecvResponse(hSerial, dummy, sizeof(dummy)))
		{
			printf("Failed to receive response at offset 0x%04lX\n", bufferOffset);
			return FALSE;
		}

		// BSCR実行
		strcpy_s(sendstr, sizeof(sendstr), "BSCR,0");
		if (!SendCom(hSerial, sendstr))
		{
			printf("Failed to send BSCR command\n");
			return FALSE;
		}

		// BSCR応答確認
		if (!RecvResponse(hSerial, dummy, sizeof(dummy)))
		{
			printf("Failed to receive BSCR response at offset 0x%04lX\n", bufferOffset);
			return FALSE;
		}

		printf("Transferred offset 0x%04lX (%lu bytes)\n", bufferOffset, bytesToSend);
	}

	return TRUE;
}

// データ検証
static BOOL VerifyData(HANDLE hSerial, BYTE* fileData, DWORD fileSize)
{
	char sendstr[256], recvbuf[65536];
	DWORD recvsize;

	// チェックサム検証コマンド実行
	strcpy_s(sendstr, sizeof(sendstr), "BCLR");
	recvsize = sizeof(recvbuf);
	if (!dispResponse(SendComRecvAck(hSerial, sendstr, recvbuf, &recvsize), sendstr, recvbuf))
		return FALSE;

	// チェックサム検証コマンド実行
	strcpy_s(sendstr, sizeof(sendstr), "SMTR,4000,8000,0,1");
	recvsize = sizeof(recvbuf);
	if (!dispResponse(SendComRecvAck(hSerial, sendstr, recvbuf, &recvsize), sendstr, recvbuf))
		return FALSE;

	// タイムアウト調整（長めの受信待ち）
	COMMTIMEOUTS timeout;
	timeout.ReadIntervalTimeout = 500;
	timeout.ReadTotalTimeoutMultiplier = 0;
	timeout.ReadTotalTimeoutConstant = 500;
	timeout.WriteTotalTimeoutMultiplier = 0;
	timeout.WriteTotalTimeoutConstant = 500;
	SetCommTimeouts(hSerial, &timeout);

	// デバイスからデータ受信
	strcpy_s(sendstr, sizeof(sendstr), "BSND,0000,8000");
	SendCom(hSerial, sendstr);
	recvsize = 32768;
	if (!RecvBlock(hSerial, recvbuf, recvsize))
	{
		puts("COM access error\n");
		return FALSE;
	}

	// fileDataとrecvbufを比較
	printf("Data Compare started...\n");
	DWORD compareSize = (fileSize < recvsize) ? fileSize : recvsize;

	for (DWORD i = 0; i < compareSize; i++)
	{
		// 0x1000単位で進捗表示
		if ((i % 0x1000) == 0)
			printf("Comparing at offset 0x%04lX...\n", i);

		if (fileData[i] != (BYTE)recvbuf[i])
		{
			printf("Data mismatch at offset 0x%04lX: expected 0x%02X, got 0x%02X\n",
				i, fileData[i], (BYTE)recvbuf[i]);
			return FALSE;
		}
	}

	printf("Data Compare completed successfully (%lu bytes verified)\n\n", compareSize);
	return TRUE;
}

// メイン処理
int comList(const wchar_t* binFile)
{
	// バイナリファイルを開く
	HANDLE hBinFile = CreateFileW(binFile, GENERIC_READ, FILE_SHARE_READ, NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);
	if (hBinFile == INVALID_HANDLE_VALUE)
	{
		wprintf(L"Failed to open binary file: %s\n", binFile);
		return 1;
	}

	// ファイルサイズ取得
	DWORD fileSize = GetFileSize(hBinFile, NULL);
	if (fileSize == 0 || fileSize == INVALID_FILE_SIZE)
	{
		wprintf(L"Invalid file size: %u\n", fileSize);
		CloseHandle(hBinFile);
		return 1;
	}

	// ファイルデータをメモリに読み込む
	BYTE* fileData = (BYTE*)malloc(fileSize);
	if (fileData == NULL)
	{
		printf("Memory allocation failed\n");
		CloseHandle(hBinFile);
		return 1;
	}

	DWORD readSize;
	if (!ReadFile(hBinFile, fileData, fileSize, &readSize, NULL) || readSize != fileSize)
	{
		printf("Failed to read binary file\n");
		free(fileData);
		CloseHandle(hBinFile);
		return 1;
	}
	CloseHandle(hBinFile);

	printf("Binary file loaded: %u bytes\n\n", fileSize);

	// COM ポート列挙
	HDEVINFO hDevInfo = SetupDiGetClassDevs(&GUID_DEVINTERFACE_COMPORT, 0, 0, DIGCF_PRESENT | DIGCF_DEVICEINTERFACE);
	if (!hDevInfo)
	{
		printf("Failed to get device info\n");
		free(fileData);
		return 1;
	}

	SP_DEVINFO_DATA data = { sizeof(data) };
	for (int count = 0; SetupDiEnumDeviceInfo(hDevInfo, count, &data); count++)
	{
		HKEY key = SetupDiOpenDevRegKey(hDevInfo, &data, DICS_FLAG_GLOBAL, 0, DIREG_DEV, KEY_QUERY_VALUE);
		if (!key)
			continue;

		WCHAR devdesc[256], portname[256];
		DWORD size = sizeof(portname);

		// デバイス情報取得
		if (!SetupDiGetDeviceRegistryProperty(hDevInfo, &data, SPDRP_HARDWAREID, NULL, (LPBYTE)devdesc, sizeof(devdesc), NULL))
		{
			RegCloseKey(key);
			continue;
		}

		// USB デバイスかチェック
		if (wcsncmp(L"USB\\", devdesc, 4) != 0)
		{
			RegCloseKey(key);
			continue;
		}

		// ポート名取得
		if (RegQueryValueExW(key, L"PortName", NULL, NULL, (LPBYTE)portname, &size) != ERROR_SUCCESS)
		{
			RegCloseKey(key);
			continue;
		}

		RegCloseKey(key);

		wprintf(L"Found USB COM port: %s\n", portname);

		// ポート名を適切な形式に変換（COM10以降対応）
		wchar_t fullPortName[32];
		swprintf_s(fullPortName, sizeof(fullPortName) / sizeof(wchar_t), L"\\\\.\\%s", portname);

		// COMポートを開く
		HANDLE hSerial = CreateFileW(fullPortName, GENERIC_READ | GENERIC_WRITE, 0, 0, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, 0);
		if (hSerial == INVALID_HANDLE_VALUE)
		{
			puts("COM open error\n");
			continue;
		}

		// シリアルポート設定
		if (!ConfigureSerialPort(hSerial))
		{
			CloseHandle(hSerial);
			continue;
		}

		wprintf(L"Connected to %s\n\n", portname);

		// バイナリデータ転送
		BOOL transferSuccess = TransferBinaryData(hSerial, fileData, fileSize);
		if (!transferSuccess)
		{
			printf("Binary data transfer failed\n\n");
			CloseHandle(hSerial);
			continue;
		}

		// データ検証
		BOOL verifySuccess = VerifyData(hSerial, fileData, fileSize);
		if (!verifySuccess)
		{
			printf("Data verification failed\n\n");
			CloseHandle(hSerial);
			continue;
		}

		// デバイス終了処理
		char sendstr[256], recvbuf[65536];
		DWORD recvsize;

		strcpy_s(sendstr, sizeof(sendstr), "ERRON");
		recvsize = sizeof(recvbuf);
		SendComRecvAck(hSerial, sendstr, recvbuf, &recvsize);

		strcpy_s(sendstr, sizeof(sendstr), "SPOFF");
		recvsize = sizeof(recvbuf);
		SendComRecvAck(hSerial, sendstr, recvbuf, &recvsize);

		printf("Transfer and verification completed successfully!\n\n");
		CloseHandle(hSerial);
	}

	SetupDiDestroyDeviceInfoList(hDevInfo);
	free(fileData);
	return 0;
}

int wmain(int argc, wchar_t* argv[])
{
	// コマンドライン引数チェック
	if (argc < 2)
	{
		wprintf(L"Usage: %s <binary_file_path>\n", argv[0]);
		return 1;
	}

	// メイン処理実行
	comList(argv[1]);

	printf("Done...\n");
	return 0;
}
