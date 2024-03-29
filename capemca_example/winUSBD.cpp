/////////////////////////////////////////////////////////////////////////////////////////////////////
// Class methods for WinUSB device communications
//   definitions in winUSBD.h
//
//    Copyright (C) 2014-2022  CapeSym, Inc.
/////////////////////////////////////////////////////////////////////////////////////////////////////

#include "winUSBD.h"

WinUSBD::WinUSBD()									// constructor
{
	self = this;
	handlesOpen = FALSE;
	openPlease = FALSE;
    winusbHandle = NULL;
    deviceHandle = NULL;
	deviceInfoData.cbSize = sizeof(SP_DEVINFO_DATA);	// size must be expicitly initialized
    devicePath[0] = 0;
	pipeInId = 0;
    pipeOutId = 0;
}

bool WinUSBD::IsValid( void )						// test validity of ptr
{
	if ( self == this ) return( true );
	puts("BUG ALERT: WinUSBD* failed validity test.");
	return( false );
}

WinUSBD & WinUSBD::operator=(const WinUSBD &assignfrom)
{
	handlesOpen = assignfrom.handlesOpen;			// assignment operator assigns attributes
	openPlease = assignfrom.openPlease;
	winusbHandle = assignfrom.winusbHandle;
	deviceHandle = assignfrom.deviceHandle;
	deviceInfoData = assignfrom.deviceInfoData;
	pipeInId = assignfrom.pipeInId;
	pipeOutId = assignfrom.pipeOutId;
	strcpy(devicePath,assignfrom.devicePath);
	return (*this);
}

WinUSBD::~WinUSBD()									// detructor
{
	if ( IsValid() )								// test validity of pointer before deleting
	  {
	  CloseDevice();								// make sure handles closed when destroyed
	  self = NULL;									// invalidate self ptr
	  }
}

void WinUSBD::Clear( void )							// reset all variables
{
	handlesOpen = FALSE;
	openPlease = FALSE;
    winusbHandle = NULL;
    deviceHandle = NULL;
    devicePath[0] = 0;
	pipeInId = 0;
    pipeOutId = 0;
}


/////////////////////////////////////////////////////////////////////////////////////////////////////
//    Retrieve the device path for the device indicated by the deviceInfoData

HRESULT WinUSBD::GetDevicePath( HDEVINFO hDeviceInfo, GUID guid )	// fill devicePath for the device
{
	BOOL                             bResult;
	HRESULT                          hr = S_OK;
    ULONG                            length, requiredLength = 0;  
	PSP_DEVICE_INTERFACE_DETAIL_DATA detailData = NULL;
	SP_DEVICE_INTERFACE_DATA         interfaceData;
	interfaceData.cbSize = sizeof(SP_DEVICE_INTERFACE_DATA);

	if ( self == this )
		{										// Get the first (and only) interface
	
		bResult = SetupDiEnumDeviceInterfaces(hDeviceInfo,&deviceInfoData,&guid,0,&interfaceData);

		if ( !bResult )
			{
			hr = HRESULT_FROM_WIN32(GetLastError());
			goto ExitGetDevicePath;
			}
													// Get the size of the path string on first call

		bResult = SetupDiGetDeviceInterfaceDetail(hDeviceInfo,&interfaceData,NULL,0,&requiredLength,NULL);

		if ( (!bResult) && (ERROR_INSUFFICIENT_BUFFER != GetLastError()) )
			{
			hr = HRESULT_FROM_WIN32(GetLastError());
			goto ExitGetDevicePath;
			}
													// Allocate temporary space for SetupDi structure

		detailData = (PSP_DEVICE_INTERFACE_DETAIL_DATA)LocalAlloc(LMEM_FIXED, requiredLength);

		if (NULL == detailData)
			{
			hr = E_OUTOFMEMORY;
			goto ExitGetDevicePath;
			}

		detailData->cbSize = sizeof(SP_DEVICE_INTERFACE_DETAIL_DATA);
		length = requiredLength;
													// now get the interface's path string

		bResult = SetupDiGetDeviceInterfaceDetail(hDeviceInfo,&interfaceData,detailData,length,&requiredLength,NULL);

		if ( !bResult )
			{
			hr = HRESULT_FROM_WIN32(GetLastError());
			goto ExitGetDevicePath;
			}
													// devicePath will be NULL-terminated.

		strncpy(devicePath, detailData->DevicePath, MAX_PATH);					
		}

ExitGetDevicePath:								// clean up on exit

    if ( detailData ) LocalFree(detailData);
	if ( hr != S_OK )
		 printf("Error getting device path: LastError=%d\n",hr);
	return hr;
}

bool WinUSBD::ExtractIdentifierFromPath( char * identifier )	
{												// identifier should also be MAX_PATH
	bool success = false;

	if ( self == this )							// validate object
	  if ( devicePath[0] )						// must have found a device path
		{
		int i = strlen(devicePath);
		while ( (i>0) && (devicePath[i] != '#') ) i--;	// find last # in path
		int j = i-1;
		while ( (j>0) && (devicePath[j] != '#') ) j--;	// find next to last #
		int k = 0;
		if ( (i>0) && (j>0) )						// if found them both then
			{	
			j++;									// move to first char between #s
			while ( j<i ) identifier[k++] = devicePath[j++];
			identifier[k] = '\0';					// null terminate
			success = true;
			}
		else strcpy(identifier,"unknown");			// when no identfier found
		}

	return( success );							// return false and "unknown"
}

///////////////////////////////////////////////////////////////////////////////////////////////////
//  FindBulkTransferEndpoints() finds the ids of the Bulk Transfer In/Out endpoints which are
//   predefined in the microcontroller code.

BOOL WinUSBD::FindBulkTransferEndpoints( bool printToConsole )
{
	BOOL bResult = FALSE;								// default return value
	BYTE value = 0;										// disable ALLOW_PARTIAL_READS
	ULONG length = 4;
	ULONG timeout = 1111;								// timeout in milliseconds
	WINUSB_PIPE_INFORMATION  Pipe;
	char *pipeTypeString[] = { "Control\0", "Isochronous\0", "Bulk\0", "Interrupt\0" };
    USB_INTERFACE_DESCRIPTOR InterfaceDescriptor;

	if ( self == this )									// first, validate object exists
	  if ( winusbHandle != INVALID_HANDLE_VALUE )		// and that handle was created
		{
		bResult = TRUE;									// success until something goes wrong
		
		ZeroMemory(&InterfaceDescriptor, sizeof(USB_INTERFACE_DESCRIPTOR));
		InterfaceDescriptor.bLength = sizeof(USB_INTERFACE_DESCRIPTOR);
		InterfaceDescriptor.bDescriptorType = USB_CONFIGURATION_DESCRIPTOR_TYPE; 
		ZeroMemory(&Pipe, sizeof(WINUSB_PIPE_INFORMATION));

		bResult = WinUsb_QueryInterfaceSettings(winusbHandle, 0, &InterfaceDescriptor);

		if (bResult)
			{
			if ( printToConsole ) printf("Number of Endpoints for Interface 0 is %d\n",InterfaceDescriptor.bNumEndpoints);

			for (int index = 0; index < InterfaceDescriptor.bNumEndpoints; index++)
				{
				if ( printToConsole ) printf("Endpoint index %d:",index);
				bResult = WinUsb_QueryPipe(winusbHandle, 0, index, &Pipe);

				if (bResult)
					{
					if ( printToConsole ) printf(" Pipe type: %s, Pipe: %d", pipeTypeString[Pipe.PipeType], Pipe.PipeId);
					if (Pipe.PipeType == UsbdPipeTypeBulk)
						{
						if (USB_ENDPOINT_DIRECTION_IN(Pipe.PipeId))			// timeout not needed because all cmds get replies?
							{
							if ( printToConsole ) printf(", Direction: IN");
							pipeInId = Pipe.PipeId;
							//WinUsb_SetPipePolicy(winusbHandle,Pipe.PipeId,PIPE_TRANSFER_TIMEOUT,sizeof(timeout),&timeout);
							WinUsb_SetPipePolicy(winusbHandle,Pipe.PipeId,ALLOW_PARTIAL_READS,sizeof(value),&value);

							//WinUsb_GetPipePolicy(winusbHandle,Pipe.PipeId,PIPE_TRANSFER_TIMEOUT,&length,&timeout);
							//if ( printToConsole ) printf("Read pipe PIPE_TRANSFER_TIMEOUT=%ld ms\n",timeout);
							//length = 1;
							//WinUsb_GetPipePolicy(winusbHandle,Pipe.PipeId,ALLOW_PARTIAL_READS,&length,&value);
							//if ( printToConsole ) printf("Read pipe ALLOW_PARTIAL_READS=%d\n",value);
							//length = 1;
							//WinUsb_GetPipePolicy(winusbHandle,Pipe.PipeId,AUTO_FLUSH,&length,&value);
							//if ( printToConsole ) printf("Read pipe AUTO_FLUSH=%d\n",value);
							}
						if (USB_ENDPOINT_DIRECTION_OUT(Pipe.PipeId))
							{
							if ( printToConsole ) printf(", Direction: OUT");
							pipeOutId = Pipe.PipeId;
							}

						}
					if ( printToConsole ) printf("\n");
					}
				else if ( printToConsole ) printf(" QueryPipe failed\n");
				}
			}
		}

    return bResult;
}

///////////////////////////////////////////////////////////////////////////////////////////////////
//  QueryDeviceProperties() writes some diagnostic information to the console window.

BOOL WinUSBD::QueryDeviceProperties( void )					// display info in console
{
    USB_DEVICE_DESCRIPTOR deviceDesc;
    BOOL                  bResult;
    ULONG                 lengthReceived;
	UCHAR				  deviceSpeed;
    ULONG				  length = sizeof(UCHAR);

	if ( self == this )									// validate object exists
		{												// Get device descriptor

		bResult = WinUsb_GetDescriptor(winusbHandle,USB_DEVICE_DESCRIPTOR_TYPE,0,0,(PBYTE)&deviceDesc,sizeof(deviceDesc),&lengthReceived);

		if ( !bResult || (lengthReceived != sizeof(deviceDesc)) )
			{
			printf("Error getting descriptor: LastError=%d, lengthReceived=%d\n",GetLastError(),lengthReceived);
			return bResult;
			}
														// Print a few parts of the device descriptor

		printf("Device Ids: VID_%04X, PID_%04X\nUSB %d.%0d ",
					deviceDesc.idVendor, deviceDesc.idProduct, ((deviceDesc.bcdUSB&0xFF00)>>8),(deviceDesc.bcdUSB&0x00FF));

		//index = deviceDesc.iSerialNumber;
		//bResult = WinUsb_GetDescriptor(winusbHandle,USB_STRING_DESCRIPTOR_TYPE,0,0,(PBYTE)&deviceDesc,sizeof(deviceDesc),&lengthReceived);
		//f ( !bResult || (lengthReceived != sizeof(deviceDesc)) )
		//	{
		//    printf(_T("Error getting descriptor: LastError=%d, lengthReceived=%d\n"),GetLastError(),lengthReceived);
		//    return bResult;
		//	}

		bResult = WinUsb_QueryDeviceInformation(winusbHandle, DEVICE_SPEED, &length, &deviceSpeed);
		if ( !bResult )
			printf("Error getting device speed: %d.\n", GetLastError());
		else
			{
			if( deviceSpeed == 0x01)
				printf("(Full speed 12Mb/s)");
			if( deviceSpeed == 0x03)
				printf("(High speed)");
			}
		printf("\n");
		}

    return bResult;
}

/////////////////////////////////////////////////////////////////////////////////////////////////////////
//    Open all needed handles to interact with the device.

HRESULT WinUSBD::OpenDevice( GUID guid, PBOOL FailureDeviceNotFound, bool printToConsole )
{
    BOOL					bResult;
    ULONG					lengthReceived;
	HRESULT					hr = S_OK;

	if ( self == this )									// verify object exists
		{
		handlesOpen = FALSE;
		if ( devicePath[0] == 0 ) return(!S_OK);		// registry path to device must be already filled in

														// create file for reading/writing to USB

		deviceHandle = CreateFile(devicePath, GENERIC_WRITE | GENERIC_READ,
											  FILE_SHARE_WRITE | FILE_SHARE_READ,
											  NULL,
											  OPEN_EXISTING,
											  FILE_ATTRIBUTE_NORMAL | FILE_FLAG_OVERLAPPED,
											  NULL);

		if ( INVALID_HANDLE_VALUE == deviceHandle )		// return error if no device handle
			{
			hr = HRESULT_FROM_WIN32(GetLastError());
			return hr;
			}											// initialize the winusb interface handle

		bResult = WinUsb_Initialize(deviceHandle, &winusbHandle);

		if (!bResult)
			{
			hr = HRESULT_FROM_WIN32(GetLastError());
			CloseHandle(deviceHandle);
			deviceHandle = NULL;
			return hr;
			}

		if ( printToConsole )
			QueryDeviceProperties();					// print a few device descriptors to the console

		handlesOpen = TRUE;
		}

    return hr;
}

///////////////////////////////////////////////////////////////////////////////////////////////////
//    Perform required cleanup when the device is no longer needed.

VOID WinUSBD::CloseDevice( void )
{
	if ( self == this )									// verify object exists
	  if ( handlesOpen )								// do nothing if not open
		{
		WinUsb_Free(winusbHandle);
		CloseHandle(deviceHandle);
		handlesOpen = FALSE;
		winusbHandle = NULL;
		deviceHandle = NULL;
		}
}

///////////////////////////////////////////////////////////////////////////////////////////////////
//     Registers an HWND for notification of changes in the device interfaces
//     for the specified interface class GUID. 

HDEVNOTIFY DoRegisterDeviceInterfaceToHwnd(GUID guid, HWND hWnd)
{
	HDEVNOTIFY hDeviceNotify;
    DEV_BROADCAST_DEVICEINTERFACE NotificationFilter;

    ZeroMemory(&NotificationFilter, sizeof(NotificationFilter));
    NotificationFilter.dbcc_size = sizeof(DEV_BROADCAST_DEVICEINTERFACE);
    NotificationFilter.dbcc_devicetype = DBT_DEVTYP_DEVICEINTERFACE;
    NotificationFilter.dbcc_classguid = guid;

    hDeviceNotify = RegisterDeviceNotification(hWnd,&NotificationFilter,DEVICE_NOTIFY_WINDOW_HANDLE);

    return(hDeviceNotify);
}

/////////////////////////////////////////////////////////////////////////////////////////////////////
//   Returns count of number of devices matching the GUID that are present in the system.

int WinUSBDs::EnumerateDevices( GUID guid )
{
	WinUSBD			*winUSBD;
	HDEVINFO		hDeviceInfo;
	DWORD			number_of_devices = 0;
	bool			deviceFound;
	SP_DEVINFO_DATA	deviceInfoData;
														// need to set size for structure
	deviceInfoData.cbSize = sizeof(SP_DEVINFO_DATA);
														// get device info set for the guid

    hDeviceInfo = SetupDiGetClassDevs(&guid,NULL,NULL,DIGCF_PRESENT|DIGCF_DEVICEINTERFACE);
    if (hDeviceInfo == INVALID_HANDLE_VALUE) return( 0 );	

	do {												// enumerate each device present
		deviceFound = false;
		if ( SetupDiEnumDeviceInfo(hDeviceInfo,number_of_devices,&deviceInfoData) )
			{
			number_of_devices++;						// found device matching the guid

			winUSBD = new WinUSBD();					// create winUSBD for it
			if ( winUSBD == NULL ) break;

			winUSBD->deviceInfoData = deviceInfoData;	// copy the whole data structure

			if ( S_OK == winUSBD->GetDevicePath(hDeviceInfo,guid) )	// retrieve DevicePath string
				this->Add( winUSBD );					// and then add to our list
			else delete winUSBD;

			deviceFound = true;							// continue to next device
			}
		}
	while ( deviceFound );

	SetupDiDestroyDeviceInfoList(hDeviceInfo);			// discard the info set

	return( (int)number_of_devices );
}

void WinUSBDs::UnenumerateDevices( void )
{
	while ( first ) DeleteFirst();
}

WinUSBD * WinUSBDs::Find( TCHAR * path )				// return ptr to list item with path
{
	WinUSBD	*winUSBD = first;

	while ( winUSBD )
		{
		if ( !strncmp(path,winUSBD->devicePath,MAX_PATH) )	// path strings must match
			break;
		winUSBD = winUSBD->next;
		}

	return( winUSBD );
}
