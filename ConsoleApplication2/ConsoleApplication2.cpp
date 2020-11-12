// ConsoleApplication2.cpp : This file contains the 'main' function. Program execution begins and ends there.
//

#include <iostream>
#include <Windows.h>
#include <stdio.h>
#include <string.h>
#include <chrono>

// Arduino Serial Communication
HANDLE hComm;                          // Handle to the Serial port
char  ComPortName[] = "\\\\.\\COM3";  // Name of the Serial port(May Change) to be opened,
char serialWriteBuffer[20] = "$h\n";
char  SerialBuffer[256];               // Buffer Containing Rxed Data
int serialBufferIdx = 0;

void arduinoSerialInit(void);
void arduinoSerialWrite(const char writeBuffer[20]);
void arduinoSerialRead(void);
void arduinoCheckIdle(bool& idle);
int main()
{
	char gcode[20];
	arduinoSerialInit();
	Sleep(2000);
	arduinoSerialWrite("$h\n");
	Sleep(4000);
	sprintf_s(gcode, "g92x0y0z0\n"); //set all to zero
	arduinoSerialWrite(gcode);

	std::chrono::steady_clock::time_point begin = std::chrono::steady_clock::now();
	int xMax = 100;
	
	sprintf_s(gcode, "g0x%dy0\n", xMax);
	arduinoSerialWrite(gcode);
	bool idle = true;
	do {
		Sleep(40);
		arduinoCheckIdle(idle);
	} while (!idle);
	arduinoSerialWrite("g0x0y0\n");
	do {
		Sleep(40);
		arduinoCheckIdle(idle);
	} while (!idle);
	std::chrono::steady_clock::time_point end = std::chrono::steady_clock::now();
	int timeElapsed = std::chrono::duration_cast<std::chrono::milliseconds>(end - begin).count();
	printf("Done in %d ms\n", timeElapsed);
	printf("%lf mm/s", double(double(xMax) * 2) / (double(timeElapsed) / 1000));

	/*
	while (TRUE) {
		Sleep(2000);
		arduinoSerialWrite("g1x30y30z30f3000\n");
		Sleep(2000);
		arduinoSerialWrite("g1x0y0z0f3000\n");
		//arduinoSerialRead();
		//Sleep(1000);
		//for (int i = 0; i < 10; i++) {
		//	Sleep(200);
		//	strcpy_s(serialWriteBuffer, "?\n");
		//	arduinoSerialWrite();
		//	arduinoSerialRead();
		//}
		//strcpy_s(serialWriteBuffer, "g0x50y50\n");
		//arduinoSerialWrite();

		//for (int i = 0; i < 10; i++) {
		//	Sleep(200);
		//	strcpy_s(serialWriteBuffer, "?\n");
		//	arduinoSerialWrite();
		//	arduinoSerialRead();
		//}
		//strcpy_s(serialWriteBuffer, "g0x0y0\n");
		//arduinoSerialWrite();
	}
	*/
	
	CloseHandle(hComm);//Closing the Serial Port
	printf("\n +==========================================+\n");
	system("pause");
	return 0;
}
void arduinoSerialInit(void) {
	BOOL  Status;                          // Status of the various operations 
	printf("\n\n +==========================================+");
	printf("\n |    Serial Port  Reception (Win32 API)    |");
	printf("\n +==========================================+\n");
	/*---------------------------------- Opening the Serial Port -------------------------------------------*/

	hComm = CreateFile(L"\\\\.\\COM3",                  // Name of the Port to be Opened
		GENERIC_READ | GENERIC_WRITE, // Read/Write Access
		0,                            // No Sharing, ports cant be shared
		NULL,                         // No Security
		OPEN_EXISTING,                // Open existing port only
		0,                            // Non Overlapped I/O
		NULL);                        // Null for Comm Devices

	if (hComm == INVALID_HANDLE_VALUE)
		printf("\n    Error! - Port %s can't be opened\n", ComPortName);
	else
		printf("\n    Port %s Opened\n ", ComPortName);

	/*------------------------------- Setting the Parameters for the SerialPort ------------------------------*/

	DCB dcbSerialParams = { 0 };                         // Initializing DCB structure
	dcbSerialParams.DCBlength = sizeof(dcbSerialParams);

	Status = GetCommState(hComm, &dcbSerialParams);      //retreives  the current settings

	if (Status == FALSE)
		printf("\n    Error! in GetCommState()");

	dcbSerialParams.BaudRate = CBR_115200;      // Setting BaudRate = 9600
	//dcbSerialParams.BaudRate = CBR_9600;      // Setting BaudRate = 9600
	dcbSerialParams.ByteSize = 8;             // Setting ByteSize = 8
	dcbSerialParams.StopBits = ONESTOPBIT;    // Setting StopBits = 1
	dcbSerialParams.Parity = NOPARITY;        // Setting Parity = None 

	Status = SetCommState(hComm, &dcbSerialParams);  //Configuring the port according to settings in DCB 

	if (Status == FALSE)
	{
		printf("\n    Error! in Setting DCB Structure");
	}
	else //If Successfull display the contents of the DCB Structure
	{
		printf("\n\n    Setting DCB Structure Successfull\n");
		printf("\n       Baudrate = %d", dcbSerialParams.BaudRate);
		printf("\n       ByteSize = %d", dcbSerialParams.ByteSize);
		printf("\n       StopBits = %d", dcbSerialParams.StopBits);
		printf("\n       Parity   = %d", dcbSerialParams.Parity);
	}

	/*------------------------------------ Setting Timeouts --------------------------------------------------*/

	COMMTIMEOUTS timeouts = { 0 };
	timeouts.ReadIntervalTimeout = 50;
	timeouts.ReadTotalTimeoutConstant = 50;
	timeouts.ReadTotalTimeoutMultiplier = 10;
	timeouts.WriteTotalTimeoutConstant = 50;
	timeouts.WriteTotalTimeoutMultiplier = 10;

	if (SetCommTimeouts(hComm, &timeouts) == FALSE)
		printf("\n\n    Error! in Setting Time Outs");
	else
		printf("\n\n    Setting Serial Port Timeouts Successfull");

	/*------------------------------------ Setting Receive Mask ----------------------------------------------*/

	Status = SetCommMask(hComm, EV_RXCHAR); //Configure Windows to Monitor the serial device for Character Reception

	if (Status == FALSE)
		printf("\n\n    Error! in Setting CommMask");
	else
		printf("\n\n    Setting CommMask successfull");
}

void arduinoSerialWrite(const char writeBuffer[20]) { // lpBuffer should be  char or byte array, otherwise write wil fail
	
	BOOL  Status;                          // Status of the various operations 
	DWORD  dNoOFBytestoWrite;              // No of bytes to write into the port
	DWORD  dNoOfBytesWritten = 0;          // No of bytes written to the port
	char serialWriteBuffer[20];
	strcpy_s(serialWriteBuffer, writeBuffer);

	dNoOFBytestoWrite = sizeof(serialWriteBuffer); // Calculating the no of bytes to write into the port

	Status = WriteFile(hComm,               // Handle to the Serialport
		serialWriteBuffer,            // Data to be written to the port 
		dNoOFBytestoWrite,   // No of bytes to write into the port
		&dNoOfBytesWritten,  // No of bytes written to the port
		NULL);

	if (Status == TRUE)
		printf("\n    %s - Written to %s", serialWriteBuffer, ComPortName);
	else
		printf("\n\n   Error %d in Writing to Serial Port", GetLastError());

}

void arduinoSerialRead(void) {

	BOOL  Status;                          // Status of the various operations 
	DWORD NoBytesRead;                     // Bytes read by ReadFile()
	DWORD dwEventMask;                     // Event mask to trigger
	char  TempChar;                        // Temperory Character

	/*------------------------------------ Setting WaitComm() Event   ----------------------------------------*/
	Status = WaitCommEvent(hComm, &dwEventMask, NULL); //Wait for the character to be received

	/*-------------------------- Program will Wait here till a Character is received ------------------------*/

	if (Status == FALSE)
	{
		printf("\n    Error! in Setting WaitCommEvent()");
	}
	else //If  WaitCommEvent()==True Read the RXed data using ReadFile();
	{
		printf("\n\n    Characters Received");
		do
		{
			Status = ReadFile(hComm, &TempChar, sizeof(TempChar), &NoBytesRead, NULL);
			SerialBuffer[serialBufferIdx] = TempChar;
			if (NoBytesRead <= 0)
				break;
			if (serialBufferIdx == 255)
				serialBufferIdx = 0;
			else
				serialBufferIdx++;
		} while (NoBytesRead > 0);

		printf("done\n");
		/*------------Printing the RXed String to Console----------------------*/
		int k = serialBufferIdx;
		int newlineCount = 0;
		if (SerialBuffer[k] == '0')
			printf("0\n");
		else
			printf("1\n");

		//while (newlineCount<4) {
		//	if (SerialBuffer[k] == '\n')
		//		newlineCount++;
		//	if (newlineCount >= 4)
		//		break;
		//	if (k == 0)
		//		k = 255;
		//	else
		//		k--;
		//}
		//k += 1;
		//printf("\ni=%d ", serialBufferIdx);
		//printf("START");
		//for (int j = k; j < serialBufferIdx;) { // j < i-1 to remove the dupliated last character
		//	printf("%c", SerialBuffer[j]);
		//	if (j == 255)
		//		j = 0;
		//	else
		//		j++;
		//}
		//	
		//printf("END\n");
		//while (SerialBuffer[k] != '<') {
		//	k++;
		//}
		//printf("\nk=%d\n", k);
		//if(SerialBuffer[k+1] == 'I')
		//	printf("IDLE\n");
		//else
		//	printf("RUN\n");
	}
}

void arduinoCheckIdle(bool& idle) { // return idle=TRUE when IDLE, idle=FALSE when RUN

	arduinoSerialWrite("?\n"); //Send ? to check GRBL status

	BOOL  Status;                          // Status of the various operations 
	DWORD NoBytesRead;                     // Bytes read by ReadFile()
	DWORD dwEventMask;                     // Event mask to trigger
	char  TempChar;                        // Temperory Character

	/*------------------------------------ Setting WaitComm() Event   ----------------------------------------*/
	Status = WaitCommEvent(hComm, &dwEventMask, NULL); //Wait for the character to be received

	/*-------------------------- Program will Wait here till a Character is received ------------------------*/

	if (Status == FALSE)
	{
		printf("Error! in Setting WaitCommEvent()");
	}
	else //If  WaitCommEvent()==True Read the RXed data using ReadFile();
	{
		do
		{
			Status = ReadFile(hComm, &TempChar, sizeof(TempChar), &NoBytesRead, NULL);
			SerialBuffer[serialBufferIdx] = TempChar;
			if (NoBytesRead <= 0)
				break;
			if (serialBufferIdx == 255)
				serialBufferIdx = 0;
			else
				serialBufferIdx++;
		} while (NoBytesRead > 0);

		// Read latest status
		int k = serialBufferIdx;
		int newlineCount = 0;
		while (newlineCount < 4) {
			if (SerialBuffer[k] == '\n')
				newlineCount++;
			if (newlineCount >= 4)
				break;
			if (k == 0)
				k = 255;
			else
				k--;
		}
		k += 1;

		// Check Idle or Run
		while (SerialBuffer[k] != '<') {
			k++;
		}
		if (SerialBuffer[k + 1] == 'I')
			idle = TRUE;
		else
			idle = FALSE;

	}
}