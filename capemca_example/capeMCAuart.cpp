///////////////////////////////////////////////////////////////////////////////////////////
//  Command line version to support testing of the uart serial interface                 //
//                                                                                       //
//    Copyright (C) 2020-2023  CapeSym, Inc.                                             //
//                                                                                       //
///////////////////////////////////////////////////////////////////////////////////////////

#include <windows.h>								// for windows types
#include <stdio.h>									// for printf to console
#include "packet0type.h"
#include "version.h"

/* #define SPECTRUM_SIZE 4096							// max. number of channels in spectrum */
#define SPECTRUM_SIZE 512							// max. number of channels in spectrum


static char help[] = "CapeMCA Uart Interface\n\n\
Usage: CapeMCAuart [flags]\n\n\
Flags:\n\
  -b=115200 : use baud rate 115200 bit/s (default)\n\
  -p=COM1 : use COM1 for serial port (default)\n\
  -q=8 : request type {0,1,2,4,8,16,32+1,32+2,32+4,32+8,32+16}\n\
  -h : display this help message\n\
  -v : print version info\n\
  -z : zero spectrum before request\n\
\nRead energy spectrum from 1 macropixels via COM port.\
\nSpectral output is streamed to the console.\n";
													// Print the version number
void printversion()
{
	INT_PTR p;										// use to determine 32 vs. 64-bit OS
	char version[64];

	if ( sizeof(p) == 4 ) sprintf(version, "32-bit Version %d.%d.%d\n",VERSION_MAJOR,VERSION_MINOR,VERSION_RELEASE);
	else sprintf(version,"64-bit Version %d.%d.%d\n",VERSION_MAJOR,VERSION_MINOR,VERSION_RELEASE);
	
	printf("\nCapeMCA Uart Test %s\n",version);
	printf("Trademark (TM) 2021 CapeSym, Inc.\nCopyright (c) 2020-2023 CapeSym, Inc.\n\n");
}

////// Non-threaded, windowless, blocking COM port communications ////////////////////////////////////////////////////////

#define SERIAL_TIMEOUT_MS			1000			// 1 second timeout

HANDLE OpenComPort( char *port, DWORD baudRate, BYTE dataBits, BYTE parity, BYTE stopBits )
{
	DCB dcb;
	DWORD dwSize;
	COMMPROP commProp;
	COMMTIMEOUTS cto;
	COMMCONFIG commConfig;
	HANDLE commFile = NULL;
	TCHAR *pcCommPort = TEXT(port);
	
	SecureZeroMemory(&dcb, sizeof(DCB));		//  Initialize the DCB structure.
	dcb.DCBlength = sizeof(DCB);
												//  Open a handle to the specified com port.
	commFile = CreateFile("\\\\.\\COM3",
							GENERIC_READ | GENERIC_WRITE,
							0,							//  must be opened with exclusive-access
							NULL,						//  default security attributes
							OPEN_EXISTING,				//  must use OPEN_EXISTING
							0,							//  not overlapped I/O; use FILE_FLAG_OVERLAPPED for timeouts
							NULL );						//  hTemplate must be NULL for comm devices


	if (commFile == INVALID_HANDLE_VALUE) 
		{
		printf ("CreateFile failed with error %d.\n", GetLastError());
		commFile = NULL;
		goto ExitOpenCommPort;
		}

	if ( !GetCommState(commFile, &dcb) )	//  Build on current configuration by retrieving current settings
		{
		printf ("GetCommState failed with error %d.\n", GetLastError());
		CloseHandle(commFile);						// close the COM port
		commFile = NULL;
		goto ExitOpenCommPort;
		}
										//  Fill in some DCB values to set the com state
	dcb.BaudRate = baudRate;					//  baud rate (e.g. 9600 or 115200
	dcb.ByteSize = dataBits;					//  data size for Tx and Rx
	dcb.Parity   = parity;						//  parity type (NOPARITY = 0)
	dcb.StopBits = stopBits;					//  stop bit enum (ONESTOPBIT = 0)

	dcb.fOutxCtsFlow = false;					// Disable CTS monitoring
	dcb.fOutxDsrFlow = false;					// Disable DSR monitoring
	dcb.fDtrControl = DTR_CONTROL_DISABLE;		// Disable DTR monitoring
	dcb.fOutX = false;							// Disable XON/XOFF for transmission
	dcb.fInX = false;							// Disable XON/XOFF for receiving
	dcb.fRtsControl = RTS_CONTROL_DISABLE;		// Disable RTS (Ready To Send)
	
	if ( !SetCommState(commFile, &dcb) )
		{
		printf ("SetCommState failed with error %d.\n", GetLastError());
		CloseHandle(commFile);						// close the COM port
		commFile = NULL;
		goto ExitOpenCommPort;
		}

	if ( !GetCommState(commFile, &dcb) )	//  Build on current configuration by retrieving current settings
		{
		printf ("GetCommState failed with error %d.\n", GetLastError());
		CloseHandle(commFile);						// close the COM port
		commFile = NULL;
		goto ExitOpenCommPort;
		}

	printf("%s open at %d baud, %d data, %d parity, %d stop, no handshaking\n",
												port,dcb.BaudRate,dcb.ByteSize,dcb.Parity,dcb.StopBits+1 );

	if ( !PurgeComm(commFile, PURGE_RXABORT|PURGE_RXCLEAR|PURGE_TXABORT|PURGE_TXCLEAR) )
		{
		printf ("PurgeComm failed with error %d.\n", GetLastError());
		CloseHandle(commFile);						// close the COM port
		commFile = NULL;
		goto ExitOpenCommPort;
		}	

	cto.ReadIntervalTimeout = MAXDWORD;			// to make readFile timeout use:
	cto.ReadTotalTimeoutConstant = SERIAL_TIMEOUT_MS;
	cto.ReadTotalTimeoutMultiplier = MAXDWORD;
	cto.WriteTotalTimeoutMultiplier = 0;		// no timeouts when writing
	cto.WriteTotalTimeoutConstant = 0;

	if ( !SetCommTimeouts(commFile,&cto) )		// Set the timeouts in driver
		{
		printf("SetCommTimeouts failed with error %d.\n", GetLastError());
		CloseHandle(commFile);						// close the COM port
		commFile = NULL;
		goto ExitOpenCommPort;
		}

ExitOpenCommPort:
	return( commFile );							// return valid HANDLE if succeed
}

int ReadComPort(HANDLE commFile, BYTE *buffer, int length )	
{												// buffer holds at least length bytes
    DWORD bytesRead;								
	BYTE b;
	int i = 0;											

	if (commFile && (length > 0))	{				// have to read bytes one at a time
		printf("commFile exists\n");
		if (ReadFile(commFile, &b, 1, &bytesRead, NULL)) {
			printf("bytesread: %d\n", bytesRead);
			while ( bytesRead && (i<length) )			// until timeout between bytes
			{									
				buffer[i++] = b;						// store Rx char in buffer
				ReadFile(commFile, &b, 1, &bytesRead, NULL);	// get next character
			}
		}
	}

	return( i );									// return number of bytes read
}


////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

int main( int argc, char * argv[] )
{
	bool i, version, usage, success, zero;
	char *c, port[20] = "COM1";
	DWORD bytesWritten, baudRate = 115200;
	HANDLE commFile;
	BYTE spccmd[2] = { 0, 2 };			// cmd to return 4096x32-bit spectrum
	BYTE zerocmd[2] = { 1, 1 };			// cmd to zero out the MCA
	UINT32 spectrum[SPECTRUM_SIZE];
	BYTE allBytes[SPECTRUM_SIZE*4+sizeof(PACKET0_TYPE)];
	PACKET0_TYPE packet0;				// for the case of channels == 0
	int bytesRead, bytesToRead, bytesInSpectrum, bytesInPacket;
	int wait, request = 8;

	zero = false;
	success = true;
	usage = false;									// reset flags for all behaviors
	version = false;

	// Process Command Line Arguments ///////////////////////////////////////////////////////////
	
	for (int i=1; i<argc; i++)						// parse command line
		{
		if ( (argv[i][0] == '-') || (argv[i][0] == '/') )
		  {
		  switch (argv[i][1])
			{
			case 'b':
				if ( (argv[i][2] == '=') )
					{
					c = argv[i] + 3;			// ptr to start of number
					sscanf(c,"%u",&baudRate);	// unsigned 32-bit conversion
					}
				else usage = true;				// flag bad command line
				break;
			case 'p':
				if ( (argv[i][2] == '=') )
					{
					i = 0;
					c = argv[i] + 3;			// ptr to start of port name
					while( (*c != ' ') && (i < 19) )
						port[i++] = *c++;
					port[i] = '\0';				// null-terminate the string
					}
				else usage = true;				// flag bad command line
				break;
			case 'q':
				if ( (argv[i][2] == '=') )
					{
					c = argv[i] + 3;			// ptr to start of request type
					request = atoi(c);			// one of {0,1,2,4,8,16,32+1,32+2,32+4,32+8,32+16}
					}
				else usage = true;				// flag bad command line
				break;
			case 'H':
			case 'h':								// print the help and exit
			case '?':
				usage = true;
				break;
			case 'V':
			case 'v':								// print the code release version number
				version = true;
				break;
			case 'Z':
			case 'z':
				zero = true;
				break;
			default:
				usage = true;
			}
		  }
		}
	if ( version )									// exit program under these two conditions
		{
		printversion();								// User asked for version info
		success = false;
		}
	if ( usage )
		{
		printf("%s",help);							
		success = false;							// return 0 for failure in Windows
		}

////////////////////////// Run //////////////////////////////////////////////////////////////////////////////////////////
	
	if ( success )									// Start if command was parsed successfully	
		{
		for (int i=1; i<SPECTRUM_SIZE; i++) spectrum[i] = 0;

		printf("\nConnecting to MCA\n");
		
		commFile = OpenComPort(port,baudRate,8,NOPARITY,ONESTOPBIT);	// enums defined in Windows API for DCB structure
		if ( commFile == NULL )	
			{
			printf(("Device not connected or COM port incorrect.\n"));
			goto Exit;
			}

		if ( zero )				// user wants to first zero out the MCA
			{
			WriteFile(commFile, zerocmd, 2, &bytesWritten, NULL );
			Sleep(1);									// allow driver to send command
			bytesToRead = 2;							// reply should be zero cmd echo
			bytesRead = ReadComPort(commFile, zerocmd, bytesToRead );
			if ( bytesRead == 2 )
			  if ( (zerocmd[0] == 1) && (zerocmd[1] == 1) )
				printf("\nZero command was processed by MCA.\n");
			}

		printf("\nRequesting data from MCA...\n");

		bytesInSpectrum = 1024*(request%32);		// spectrum coming if remainder present
		bytesInPacket = 0;
		if ( (request == 0) || (request/32 == 1) )	// packet coming if multiple of 32
			bytesInPacket = sizeof(packet0);
		bytesToRead = bytesInSpectrum + bytesInPacket;

		spccmd[1] = (BYTE)request;					// issue 2-byte request for data 
		WriteFile(commFile, spccmd, 2, &bytesWritten, NULL );
		Sleep(1);									// allow driver to send command

		printf("\nReading data from MCA\n");		// read the data from serial port

		bytesRead = ReadComPort(commFile, allBytes, bytesToRead );
		if ( bytesRead == bytesToRead ) {
			printf("BytesRead: %d, BytesToRead: %d\n", bytesRead, bytesToRead);
			if ( bytesInSpectrum )					// move bytes to local spectrum
				{
				memcpy(spectrum,allBytes,bytesInSpectrum);
				printf("Spectrum:\nchannel,count\n");
				for (int i=1; i<bytesInSpectrum/4; i++)
					printf("%d,%u\n", i,spectrum[i]);	
				}
			if ( bytesInPacket )					// move packet bytes to local struct
				{
				memcpy(&packet0,allBytes+bytesInSpectrum,bytesInPacket);
				printf("cps,totalCount,totalPulseTime,usPerInterval,totalIntervals,capemcaId\n");
				printf("%g,%g,%g,%d,%d,%d\n",packet0.cps,packet0.totalCount,packet0.totalPulseTime,
										packet0.usPerInterval,packet0.totalIntervals,packet0.capemcaId);
				}
			}
		else printf("Data transmission error.\n");


Exit:							
		CloseHandle(commFile);						// close the COM port
		commFile = NULL;
		printf("\nDone.\n");
		}

	return( success );								// return 0 if anything failed (Windows)
}
