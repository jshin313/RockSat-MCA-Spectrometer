/////////////////////////////////////////////////////////////////////////////////////////////////////
// Class definition for WinUSB device communications
//   methods in winUSBD.cpp
//
//    Copyright (C) 2014-2022  CapeSym, Inc.
/////////////////////////////////////////////////////////////////////////////////////////////////////

#pragma once

#include <windows.h>
#include <initguid.h>			// for USB communications
#include <SetupAPI.h>
#include <usb.h>
#include <winusb.h>
#include <dbt.h>
								// C RunTime Header Files
#include <stdlib.h>
#include <stdio.h>				// for printf to console
#include "lists.h"

////////////////////////////////////////////////////////////////////////////////////////////////
// Define the Device Interface GUID used by all WinUsb devices that this application talks to.
// Should match "DeviceInterfaceGUIDs" registry value specified in the INF file or registry.
// For the STM32H7 microcontroller application we have the STM-provided interface:
//
DEFINE_GUID(GUID_DEVINTERFACE_USBDevice, 0x13EB360B, 0xBC1E, 0x46CB,
												0xAC, 0x8B, 0xEF, 0x3D, 0xA4, 0x7B, 0x40, 0x62);

// Note that some information can be found in the WinUSB driver details (Device Manager):
//   Hardware Ids: VID=4701, PID=0290
//   Compatible Ids: Class=FF, SubClass=0, Protocol=0
//   Class Guid: {88bae032-5a81-49f0-bc3d-a4ff138216d6} (a.k.a. Driver key)
//
// But the GUID you need here is the that of the DeviceInterface itself, which can be found in
// the Registry ( Windows System > Run > regedit ) under the following key:
//		HKEY_LOCAL_MACHINE\SYSTEM\CurrentControlSet\Enum\USB\
// When the USB device is plugged in, you can find the device by its VID&PID as found above.
//

class WinUSBD {										// list item for WinUSB device handles
public:
	WinUSBD *self, *prev, *next;
	BOOL                    handlesOpen, openPlease;
    WINUSB_INTERFACE_HANDLE winusbHandle;			// winusb handle returned by WinUsb_Initialize()
    HANDLE                  deviceHandle;			// device handle created by CreateFile()
	SP_DEVINFO_DATA			deviceInfoData;			// device info element returned by SetupDiEnumDeviceInfo()
    TCHAR                   devicePath[MAX_PATH];
	UCHAR					pipeInId;
    UCHAR					pipeOutId;

	WinUSBD();										// constructor
	bool IsValid( void );							// for ptr validity testing
	WinUSBD & operator=(const WinUSBD &assignfrom);	// assignment operator
	~WinUSBD();										// destructor
	void Clear( void );								// reset all variables
	HRESULT GetDevicePath( HDEVINFO hDeviceInfo, GUID guid );	// fill devicePath based on deviceInfoData
	bool ExtractIdentifierFromPath( char * identifier );		// get id string from devicePath
	BOOL FindBulkTransferEndpoints( bool printToConsole );		// show progress when argument is true
	BOOL QueryDeviceProperties(	void );
	HRESULT OpenDevice( GUID guid, PBOOL FailureDeviceNotFound, bool printToConsole );
	void CloseDevice( void );						// open & close called from deviceUSB on local copy
};

HDEVNOTIFY DoRegisterDeviceInterfaceToHwnd(GUID guid, HWND hWnd);	// use to enable attach/detach messages

class WinUSBDs : public List<WinUSBD> {				// linked list of WinUSB devices
public:
	int EnumerateDevices( GUID guid );				// fill list with devices present
	void UnenumerateDevices( void );				// remove all enumerated devices
	WinUSBD * Find( TCHAR * path );					// return ptr to list item with path
};

