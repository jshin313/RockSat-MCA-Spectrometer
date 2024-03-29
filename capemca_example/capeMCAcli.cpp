///////////////////////////////////////////////////////////////////////////////////////////
//  Command line version to support window-less recording of MCA spectra                 //
//                                                                                       //
//    Copyright (C) 2020-2022  CapeSym, Inc.                                                  //
//                                                                                       //
///////////////////////////////////////////////////////////////////////////////////////////

#include "version.h"
#include "winUSBD.h"

/* #define SPECTRUM_SIZE 4096							// number of channels in spectrum */
#define SPECTRUM_SIZE 512							// number of channels in spectrum

WinUSBDs winUSBDs;									// USB devices that have been enumerated
													// Linux-style command-line help message

static char help[] = "CapeMCA Command Line Interface\n\n\
Usage: CapeMCA_cli [flags]\n\n\
Flags:\n\
  -h : display this help message\n\
  -v : print version info\n\
\nRead energy spectrum from 1 or more macropixels.\
\nSpectral output is streamed to the console.\n";
													// Print the version number
void printversion()
{
	INT_PTR p;										// use to determine 32 vs. 64-bit OS
	char version[64];

	if ( sizeof(p) == 4 ) sprintf(version, "32-bit Version %d.%d.%d\n",VERSION_MAJOR,VERSION_MINOR,VERSION_RELEASE);
	else sprintf(version,"64-bit Version %d.%d.%d\n",VERSION_MAJOR,VERSION_MINOR,VERSION_RELEASE);
	
	printf("\nCapeMCA CLI %s\n",version);
	printf("Trademark (TM) 2021 CapeSym, Inc.\nCopyright (c) 2020-2022 CapeSym, Inc.\n\n");
}

////// Non-threaded, windowless, blocking USB communications ////////////////////////////////////////////////////////

bool Connect( WinUSBD *winUSBD )	// open USB device and start the communication thread
{
	HRESULT hr;
    BOOL noDevice;									// find by GUID of Device Interface

	hr = winUSBD->OpenDevice(GUID_DEVINTERFACE_USBDevice, &noDevice,false);	

    if ( FAILED(hr) )
		{
        if (noDevice)
            printf("Device not connected or driver not installed\n");
        else
            printf("Failed looking for device, HRESULT 0x%x\n", hr);
        return false;
		}
													// find bulk transfer endpoints
	hr = winUSBD->FindBulkTransferEndpoints(false);	// and set timeout for ReadPipe

	if ( FAILED(hr) )
		{
		hr = HRESULT_FROM_WIN32(GetLastError());
        printf("Error finding bulk transfer endpoints: %d\n", hr);
		return( false );
		}

	return( true );
}

void SendRequest( WinUSBD *winUSBD )	// send request for a spectrum to device
{
	ULONG cbWritten;
	/* BYTE spccmd[2] = { 0, 16 };			// cmd to return 4096x32-bit spectrum */
	BYTE spccmd[2] = { 0, 2 };			// cmd to return 512x32-bit spectrum

	WinUsb_WritePipe(winUSBD->winusbHandle, winUSBD->pipeOutId, spccmd, 2, &cbWritten, 0);
}

bool ReceiveSpectrum( WinUSBD *winUSBD, UINT32 *spectrum )	// wait for reception of spectrum from device
{
	ULONG cbRead;
	int bytesToRead = SPECTRUM_SIZE*4;
	BYTE spectrumBytes[SPECTRUM_SIZE*4];
	bool success = false;

	if ( WinUsb_ReadPipe(winUSBD->winusbHandle, winUSBD->pipeInId, spectrumBytes, bytesToRead, &cbRead, 0) )
		{
		memcpy(spectrum,spectrumBytes,bytesToRead);		// move byte array to spectrum array
		success = true;
		}

	return( success );
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

int main( int argc, char * argv[] )
{
	bool version, usage, success;
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
			case 'H':
			case 'h':								// print the help and exit
			case '?':
				usage = true;
				break;
			case 'V':
			case 'v':								// print the code release version number
				version = true;
				break;
			default:
				usage = true;
			}
		  }
		}
	if ( version )									// exit program under these two conditions
		{
		printversion();								// User asked for version info
		return( 0 );
		}
	if ( usage )
		{
		printf("%s",help);							
		return( 0 );								// return 0 for failure in Windows
		}

////////////////////////// Run //////////////////////////////////////////////////////////////////////////////////////////
	
	if ( success )									// Start if command was parsed successfully	
		{
		WinUSBD *winUSBD;
		char name[MAX_PATH];
		bool connected[256];						// allow up to 256 MCAs to attached to this PC
		UINT32 s[SPECTRUM_SIZE], spectrum[SPECTRUM_SIZE];
		int n;

		for (int i=1; i<SPECTRUM_SIZE; i++) spectrum[i] = 0;

		printf("\nEnumerating MCAs..");

		winUSBDs.EnumerateDevices(GUID_DEVINTERFACE_USBDevice);	// find all STM32 WINUSB MCAs
		n = winUSBDs.Number();
		printf(". found %d MCAs\n", n);
		if ( n < 1 ) goto Exit;

		printf("\nConnecting to MCAs\n");
		
		n = 0;
		winUSBD = winUSBDs.first;
		while ( winUSBD )									// attempt to connect to them 						
			{
			winUSBD->ExtractIdentifierFromPath(name);		// GET the name of each MCA
			n++;
			printf("\n%d: %s : ",n,name);
			
			if ( Connect(winUSBD) )							// attempt USB connection
				{
				connected[n] = true;
				printf("connected\n");
				}
			else											// if fail to connect, fahgetaboudit
				{
				connected[n] = false;
				printf("connect failed\n");
				}

			winUSBD = winUSBD->next;						// until all attached MCAs are tried
			}

		printf("\nRequesting spectra from MCAs\n");

		n = 0;
		winUSBD = winUSBDs.first;
		while ( winUSBD )									// send request(s) for spectrum
			{
			n++;
			if ( connected[n] )
				SendRequest(winUSBD);

			winUSBD = winUSBD->next;						// until all attached MCAs are tried
			}
		
		printf("\nReading spectra from MCAs\n");

		n = 0;
		winUSBD = winUSBDs.first;
		while ( winUSBD )									// receive spectrum from each MCA
			{
			n++;
			printf("Spectrum %d:\n",n);
			Sleep(1000);									// wait for a second to let the user see it
			
			if ( connected[n] )
			  if ( ReceiveSpectrum(winUSBD,s) )				// wait for spectrum to be returned
				{
				for (int i=1; i<SPECTRUM_SIZE; i++)			
					spectrum[i] += s[i];					// sum into spectrum
				}

			winUSBD = winUSBD->next;						// until all attached MCAs are tried
			}

		printf("channel,count\n");
		for (int i=1; i<SPECTRUM_SIZE; i++)
			printf("%d,%u\n", i,spectrum[i]);				// print spectrum to console

Exit:							
		winUSBDs.UnenumerateDevices();						// release all MCAs
		printf("\nDone.\n");
		}

	return( success );								// return 0 if anything failed (Windows)
}
