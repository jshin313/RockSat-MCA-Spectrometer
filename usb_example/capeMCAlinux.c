///////////////////////////////////////////////////////////////////////////////////////////
//
//  Simple program that uses libusb to read packet zero from the CapeSym'c STM32-based MCA.
//  This program was compiled and tested on Red Hat Enterprise 6 Linux operating system.
//
//  USB ports may be listed by the command: lsusb
//  USB ports are created in /dev/bus/usb often with root,root ownership and without write
//  permissions for users. So you may need to set write permissions on the USB port before
//  running this code. To change the permissions whenever the MCA is plugged in, 
//  (assuming the user who runs the following code is a member of GROUP users,)
//  create a file in /etc/udev/rules.d, such as 50-myusb.rules, with the following line:
//
//  SUBSYSTEMS=="usb",ATTRS{idVendor}=="4701",ATTRS{idProduct}=="0290",GROUP="users",MODE="0666"
//
//  After reboot any member of the users group should have read and write permissions to
//  the port in /dev/bus/usb that is assigned to the plugged in MCA device.
//
// Compile:
//   $ gcc -o capeMCA libusb.so capeMCAlinux.c
// Run:
//   $ ./capeMCA
//
// For Documentation on libusb see:
//   http://libusb.sourceforge.io/api-1.0/index.html
//
////////////////////////////////////////////////////////////////////////////////////////////

#include <stdio.h>
#include <stdlib.h>
#include <memory.h>
// here the libusb is in the folder with this program so
#include "libusb.h"
//change this if your libusb.h is somewhere else, e.g.
//#include <libusb-1.0/libusb.h>
#include "packet0type.h"

#define USB_VENDOR_ID	    0x4701      // USB vendor ID of STM32 microcontroller 
#define USB_PRODUCT_ID	    0x0290      // USB product ID of same

void print_endpoint(const struct libusb_endpoint_descriptor *endpoint)
{
	int i, ret;

	printf("      Endpoint:\n");
	printf("        bEndpointAddress:    %02xh\n", endpoint->bEndpointAddress);
	printf("        bmAttributes:        %02xh\n", endpoint->bmAttributes);
	printf("        wMaxPacketSize:      %u\n", endpoint->wMaxPacketSize);
	printf("        bInterval:           %u\n", endpoint->bInterval);
	printf("        bRefresh:            %u\n", endpoint->bRefresh);
	printf("        bSynchAddress:       %u\n", endpoint->bSynchAddress);
	
}

void print_altsetting(const struct libusb_interface_descriptor *interface)
{
	int i;

	printf("    Interface:\n");
	printf("      bInterfaceNumber:      %u\n", interface->bInterfaceNumber);
	printf("      bAlternateSetting:     %u\n", interface->bAlternateSetting);
	printf("      bNumEndpoints:         %u\n", interface->bNumEndpoints);
	printf("      bInterfaceClass:       %u\n", interface->bInterfaceClass);
	printf("      bInterfaceSubClass:    %u\n", interface->bInterfaceSubClass);
	printf("      bInterfaceProtocol:    %u\n", interface->bInterfaceProtocol);
	printf("      iInterface:            %u\n", interface->iInterface);

	for (i = 0; i < interface->bNumEndpoints; i++)
		print_endpoint(&interface->endpoint[i]);
}

void print_interface(const struct libusb_interface *interface)
{
	int i;

	for (i = 0; i < interface->num_altsetting; i++)
		print_altsetting(&interface->altsetting[i]);
}

void print_configuration(struct libusb_config_descriptor *config)
{
	int i;

	printf("    wTotalLength:            %u\n", config->wTotalLength);
	printf("    bNumInterfaces:          %u\n", config->bNumInterfaces);
	printf("    bConfigurationValue:     %u\n", config->bConfigurationValue);
	printf("    iConfiguration:          %u\n", config->iConfiguration);
	printf("    bmAttributes:            %02xh\n", config->bmAttributes);
	printf("    MaxPower:                %u\n", config->MaxPower);

	for (i = 0; i < config->bNumInterfaces; i++)
		print_interface(&config->interface[i]);
}

void print_packet0( PACKET0_TYPE pkt0 )
{
	printf("    cps:                     %g\n",pkt0.cps);
	printf("    totalCount:              %g\n",pkt0.totalCount);
	printf("    totalPulseTime:          %g s\n",pkt0.totalPulseTime);
	printf("    usPerInterval:           %u\n",pkt0.usPerInterval);
	printf("    totalIntervals:          %u\n",pkt0.totalIntervals);
    printf("    capemcaId:               %u\n",pkt0.capemcaId);
	if ( pkt0.detectors > 1 )
		{
		printf("    detectors:               %u\n",pkt0.detectors);
		printf("    cpiArray:                %u\n",pkt0.cpiArray);
		printf("    countInRangeArray:       %u\n",pkt0.countInRangeArray);
		printf("    xDirection:              %g\n",pkt0.xDirection);
		printf("    yDirection:              %g\n",pkt0.yDirection);
		printf("    zDirection:              %g\n",pkt0.zDirection);
		}
}

int main(int argc, char **argv)
{
	int i, j, active, err, bytesWritten, bytesRead;
	ssize_t cnt;	
	char string[64];		
	libusb_device **devs = NULL;
	libusb_device *dev = NULL;
	libusb_device_handle *handle = NULL;
	struct libusb_device_descriptor desc;
	struct libusb_config_descriptor *config;
	PACKET0_TYPE packet0;
	unsigned char allBytes[sizeof(PACKET0_TYPE)];
	unsigned char cmd[2] = {0, 0};

														// initalize the libusb library
	printf("Initializing libusb...\n");
	err = libusb_init(NULL);
	if ( err < 0 )										// API return value is zero on success
		{
		printf("libusb_init returned error = %s\n",libusb_error_name(err));
		goto abortmain;
		}

	cnt = libusb_get_device_list(NULL, &devs);			// list of MCA devices that are plugged in
	if ( cnt < 0 )
		{
		printf("libusb_get_device_list returned error = %s\n",libusb_error_name(cnt));
		goto abortmain;
		}

	printf("Scanning %d USB devices...\n",cnt);			// examine each device to see if it is MCA
	i = 0;
	while ((dev = devs[i++]) != NULL)
		{
		err = libusb_get_device_descriptor(dev, &desc);	// find MCA by vendor and product id values
		if ( !err )
		  if ( (desc.idVendor == USB_VENDOR_ID) && (desc.idProduct == USB_PRODUCT_ID) )
			{
			printf("\nOpening ");
			err = libusb_open(dev,&handle);				// open the USB device
			if ( err < 0 )
				{
				printf("\n  libusb_open returned error = %s\n",libusb_error_name(err));
				goto abortmain;
				}
			else if ( desc.iSerialNumber )				// display the MCA serial number
				{
				err = libusb_get_string_descriptor_ascii(handle,desc.iSerialNumber,string,sizeof(string));
				if ( err > 0 ) printf("%s",string);
				}
			printf("\n");								// show details about USB configuration...
			for (j = 0; j < desc.bNumConfigurations; j++)
				{
				err= libusb_get_config_descriptor(dev, j, &config);
				if ( err < 0 ) {
					printf("  Couldn't retrieve descriptor for config %d\n",j);
					continue;
					}
				printf("  Configuration %d:\n",j);		// ..just to help the programmer understand it
				print_configuration(config);
				libusb_free_config_descriptor(config);
				}

			printf("  Preparing for bulk transfers...\n");
			err = libusb_claim_interface(handle,0);		// claim first (and only) interface
			if ( err < 0 )
				{
				printf("libusb_claim_interface returned error = %s\n",libusb_error_name(err));
				goto abortmain;
				}
														// write command to outout pipe (endpt 1)

			printf("  Requesting packet0 of %d bytes...\n",sizeof(packet0));
			err = libusb_bulk_transfer(handle,1,cmd,2,&bytesWritten,10000);
			if ( err < 0 )
				printf("  Write failed with error %s\n",libusb_error_name(err));
			else
														// now read the 64-bytes packet0 response
				{
				err = libusb_bulk_transfer(handle,0x81,allBytes,sizeof(PACKET0_TYPE),&bytesRead,10000);
				if ( err < 0 ) printf("  Read failed with error %s\n",libusb_error_name(err));
				else
														// display packet0 to the user
					{
					printf("  Read %d bytes from endpoint %d\n",bytesRead,0x81);
					memcpy(&packet0,allBytes,sizeof(packet0));
					print_packet0(packet0);
					}
				}
	
			err = libusb_release_interface(handle,0); 	// must release before closing handle
			printf("Closing handle...\n");
			libusb_close(handle);
			handle = NULL;
			}
		}				// continue with next device in list, if any

abortmain:
														// free memory allocated for device list
	libusb_free_device_list(devs, 1);					// and clean up interface/handle when abort
	if ( handle )
		{
		libusb_release_interface(handle,0);
    	printf("Closing handle...\n");
		libusb_close(handle);
		}

	libusb_exit(NULL);									// free the library
	printf("done.\n");
	return 0;
}
