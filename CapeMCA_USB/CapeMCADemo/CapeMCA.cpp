/* Copyright (C) 2024 Jacob Shin
   Based heavily on the USB Host 2.0 ADK Example: https://github.com/felis/USB_Host_Shield_2.0/blob/master/adk.cpp

/* CapeMCA interface */

#include "CapeMCA.h"

const uint8_t CapeMCA::epDataInIndex = 2;
const uint8_t CapeMCA::epDataOutIndex = 1;

CapeMCA::CapeMCA(USB *p, const char* manufacturer,
        const char* model,
        const char* description,
        const char* version,
        const char* uri,
        const char* serial) :

/* CapeMCA ID Strings */
manufacturer(manufacturer),
model(model),
description(description),
version(version),
uri(uri),
serial(serial),
pUsb(p), //pointer to USB class instance - mandatory
bAddress(0), //device address - mandatory
bConfNum(0), //configuration number
bNumEP(1), //if config descriptor needs to be parsed
ready(false) {
        // initialize endpoint data structures
        for(uint8_t i = 0; i < CapeMCA_MAX_ENDPOINTS; i++) {
                epInfo[i].epAddr = 0;
                epInfo[i].maxPktSize = (i) ? 0 : 8;
                epInfo[i].bmSndToggle = 0;
                epInfo[i].bmRcvToggle = 0;
                epInfo[i].bmNakPower = (i) ? USB_NAK_NOWAIT : USB_NAK_MAX_POWER;
        }//for(uint8_t i=0; i<CapeMCA_MAX_ENDPOINTS; i++...

        epInfo[epDataInIndex].epAddr = 0x81;
        epInfo[epDataOutIndex].epAddr = 0x1;



        // register in USB subsystem
        if(pUsb) {
                pUsb->RegisterDeviceClass(this); //set devConfig[] entry
        }
}

uint8_t CapeMCA::ConfigureDevice(uint8_t parent, uint8_t port, bool lowspeed) {
        return Init(parent, port, lowspeed); // Just call Init. Yes, really!
}

/* Connection initialization of CapeMCA device */
uint8_t CapeMCA::Init(uint8_t parent, uint8_t port, bool lowspeed) {
        uint8_t buf[sizeof (USB_DEVICE_DESCRIPTOR)];
        USB_DEVICE_DESCRIPTOR * udd = reinterpret_cast<USB_DEVICE_DESCRIPTOR*>(buf);
        uint8_t rcode;
        uint8_t num_of_conf; // number of configurations
        UsbDevice *p = NULL;
        EpInfo *oldep_ptr = NULL;

        // get memory address of USB device address pool
        AddressPool &addrPool = pUsb->GetAddressPool();

        USBTRACE("\r\nCapeMCA Init");

        // check if address has already been assigned to an instance
        if(bAddress) {
                USBTRACE("\r\nAddress in use");
                return USB_ERROR_CLASS_INSTANCE_ALREADY_IN_USE;
        }

        // Get pointer to pseudo device with address 0 assigned
        p = addrPool.GetUsbDevicePtr(0);

        if(!p) {
                USBTRACE("\r\nAddress not found");
                return USB_ERROR_ADDRESS_NOT_FOUND_IN_POOL;
        }

        if(!p->epinfo) {
                USBTRACE("epinfo is null\r\n");
                return USB_ERROR_EPINFO_IS_NULL;
        }

        // Save old pointer to EP_RECORD of address 0
        oldep_ptr = p->epinfo;

        // Temporary assign new pointer to epInfo to p->epinfo in order to avoid toggle inconsistence
        p->epinfo = epInfo;

        p->lowspeed = lowspeed;

        // Get device descriptor
        rcode = pUsb->getDevDescr(0, 0, sizeof (USB_DEVICE_DESCRIPTOR), (uint8_t*)buf);

        // Restore p->epinfo
        p->epinfo = oldep_ptr;

        if(rcode) {
                goto FailGetDevDescr;
        }

        // Allocate new address according to device class
        bAddress = addrPool.AllocAddress(parent, false, port);

        // Extract Max Packet Size from device descriptor
        epInfo[0].maxPktSize = udd->bMaxPacketSize0;

        // Assign new address to the device
        rcode = pUsb->setAddr(0, 0, bAddress);
        if(rcode) {
                p->lowspeed = false;
                addrPool.FreeAddress(bAddress);
                bAddress = 0;
                //USBTRACE2("setAddr:",rcode);
                return rcode;
        }//if (rcode...

        //USBTRACE2("\r\nAddr:", bAddress);
        // Spec says you should wait at least 200ms.
        //delay(300);

        p->lowspeed = false;

        //get pointer to assigned address record
        p = addrPool.GetUsbDevicePtr(bAddress);
        if(!p) {
                return USB_ERROR_ADDRESS_NOT_FOUND_IN_POOL;
        }

        p->lowspeed = lowspeed;

        // Assign epInfo to epinfo pointer - only EP0 is known
        rcode = pUsb->setEpInfoEntry(bAddress, 1, epInfo);
        if(rcode) {
                goto FailSetDevTblEntry;
        }

        //check to make sure device is CapeMCA device; if yes, configure and exit
        if(udd->idVendor == CapeMCA_VID &&
                (udd->idProduct == CapeMCA_PID)) {
                USBTRACE("\r\nCapeMCA device detected");
                /* go through configurations, find first bulk-IN, bulk-OUT EP, fill epInfo and quit */
                num_of_conf = udd->bNumConfigurations;

                //USBTRACE2("\r\nNC:",num_of_conf);
                for(uint8_t i = 0; i < num_of_conf; i++) {
                        ConfigDescParser < 0, 0, 0, 0 > confDescrParser(this);
                        delay(1);
                        rcode = pUsb->getConfDescr(bAddress, 0, i, &confDescrParser);
#if defined(XOOM)
                        //added by Jaylen Scott Vanorden
                        if(rcode) {
                                USBTRACE2("\r\nGot 1st bad code for config: ", rcode);
                                // Try once more
                                rcode = pUsb->getConfDescr(bAddress, 0, i, &confDescrParser);
                        }
#endif
                        if(rcode) {
                                goto FailGetConfDescr;
                        }
                        if(bNumEP > 2) {
                                break;
                        }
                } // for (uint8_t i=0; i<num_of_conf; i++...

                if(bNumEP == 3) {
                        // Assign epInfo to epinfo pointer - this time all 3 endpoins
                        rcode = pUsb->setEpInfoEntry(bAddress, 3, epInfo);
                        if(rcode) {
                                goto FailSetDevTblEntry;
                        }
                }

                // Set Configuration Value
                rcode = pUsb->setConf(bAddress, 0, bConfNum);
                if(rcode) {
                        goto FailSetConfDescr;
                }
                /* print endpoint structure */
                /*
                USBTRACE("\r\nEndpoint Structure:");
                USBTRACE("\r\nEP0:");
                USBTRACE2("\r\nAddr: ", epInfo[0].epAddr);
                USBTRACE2("\r\nMax.pkt.size: ", epInfo[0].maxPktSize);
                USBTRACE2("\r\nAttr: ", epInfo[0].epAttribs);
                USBTRACE("\r\nEpout:");
                USBTRACE2("\r\nAddr: ", epInfo[epDataOutIndex].epAddr);
                USBTRACE2("\r\nMax.pkt.size: ", epInfo[epDataOutIndex].maxPktSize);
                USBTRACE2("\r\nAttr: ", epInfo[epDataOutIndex].epAttribs);
                USBTRACE("\r\nEpin:");
                USBTRACE2("\r\nAddr: ", epInfo[epDataInIndex].epAddr);
                USBTRACE2("\r\nMax.pkt.size: ", epInfo[epDataInIndex].maxPktSize);
                USBTRACE2("\r\nAttr: ", epInfo[epDataInIndex].epAttribs);
                 */

                USBTRACE("\r\nConfiguration successful");
                ready = true;
                return 0; //successful configuration
        }

        goto Fail; 

        /* diagnostic messages */
FailGetDevDescr:
#ifdef DEBUG_USB_HOST
        NotifyFailGetDevDescr(rcode);
        goto Fail;
#endif

FailSetDevTblEntry:
#ifdef DEBUG_USB_HOST
        NotifyFailSetDevTblEntry(rcode);
        goto Fail;
#endif

FailGetConfDescr:
#ifdef DEBUG_USB_HOST
        NotifyFailGetConfDescr(rcode);
        goto Fail;
#endif

FailSetConfDescr:
#ifdef DEBUG_USB_HOST
        NotifyFailSetConfDescr(rcode);
        goto Fail;
#endif

Fail:
        // USBTRACE2("\r\nCapeMCA Init Failed, error code: ", rcode);
        Release();
        return rcode;
}

/* Extracts bulk-IN and bulk-OUT endpoint information from config descriptor */
void CapeMCA::EndpointXtract(uint8_t conf, uint8_t iface __attribute__((unused)), uint8_t alt __attribute__((unused)), uint8_t proto __attribute__((unused)), const USB_ENDPOINT_DESCRIPTOR *pep) {
        //ErrorMessage<uint8_t>(PSTR("Conf.Val"), conf);
        //ErrorMessage<uint8_t>(PSTR("Iface Num"), iface);
        //ErrorMessage<uint8_t>(PSTR("Alt.Set"), alt);

        //added by Yuuichi Akagawa
        if(bNumEP == 3) {
                return;
        }

        bConfNum = conf;

        if((pep->bmAttributes & bmUSB_TRANSFER_TYPE) == USB_TRANSFER_TYPE_BULK) {
                uint8_t index = ((pep->bEndpointAddress & 0x80) == 0x80) ? epDataInIndex : epDataOutIndex;
                // Fill in the endpoint info structure
                epInfo[index].epAddr = (pep->bEndpointAddress & 0x0F);
                epInfo[index].maxPktSize = (uint8_t)pep->wMaxPacketSize;

                bNumEP++;

                PrintEndpointDescriptor(pep);
        }
}

/* Performs a cleanup after failed Init() attempt */
uint8_t CapeMCA::Release() {
        pUsb->GetAddressPool().FreeAddress(bAddress);

        bNumEP = 1; //must have to be reset to 1

        bAddress = 0;
        ready = false;
        return 0;
}

uint8_t CapeMCA::RcvData(uint16_t *bytes_rcvd, uint8_t *dataptr) {
        //USBTRACE2("\r\nAddr: ", bAddress );
        //USBTRACE2("\r\nEP: ",epInfo[epDataInIndex].epAddr);
        return pUsb->inTransfer(bAddress, epInfo[epDataInIndex].epAddr, bytes_rcvd, dataptr);
}

uint8_t CapeMCA::SndData(uint16_t nbytes, uint8_t *dataptr) {
        return pUsb->outTransfer(bAddress, epInfo[epDataOutIndex].epAddr, nbytes, dataptr);
}

void CapeMCA::PrintEndpointDescriptor(const USB_ENDPOINT_DESCRIPTOR* ep_ptr) {
        Notify(PSTR("Endpoint descriptor:"), 0x80);
        Notify(PSTR("\r\nLength:\t\t"), 0x80);
        D_PrintHex<uint8_t > (ep_ptr->bLength, 0x80);
        Notify(PSTR("\r\nType:\t\t"), 0x80);
        D_PrintHex<uint8_t > (ep_ptr->bDescriptorType, 0x80);
        Notify(PSTR("\r\nEndpoint Address:\t"), 0x80);
        D_PrintHex<uint8_t > (ep_ptr->bEndpointAddress, 0x80);
        Notify(PSTR("\r\nAttributes:\t"), 0x80);
        D_PrintHex<uint8_t > (ep_ptr->bmAttributes, 0x80);
        Notify(PSTR("\r\nMaxPktSize:\t"), 0x80);
        D_PrintHex<uint16_t > (ep_ptr->wMaxPacketSize, 0x80);
        Notify(PSTR("\r\nPoll Intrv:\t"), 0x80);
        D_PrintHex<uint8_t > (ep_ptr->bInterval, 0x80);
        Notify(PSTR("\r\n"), 0x80);
}
