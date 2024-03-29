/* Copyright (C) 2024 Jacob Shin
   Based heavily on the USB Host 2.0 ADK Example: https://github.com/felis/USB_Host_Shield_2.0/blob/master/adk.h

/* CapeMCA interface support header */

#if !defined(_CapeMCA_H_)
#define _CapeMCA_H_

#include "Usb.h"

#define CapeMCA_VID 0x4701
#define CapeMCA_PID 0x0290

#define XOOM  //enables repeating getProto() and getConf() attempts
//necessary for slow devices such as Motorola XOOM
//defined by default, can be commented out to save memory

/* requests */

#define CapeMCA_GETPROTO      51  //check USB accessory protocol version
#define CapeMCA_SENDSTR       52  //send identifying string
#define CapeMCA_ACCSTART      53  //start device in accessory mode

#define bmREQ_CapeMCA_GET     USB_SETUP_DEVICE_TO_HOST|USB_SETUP_TYPE_VENDOR|USB_SETUP_RECIPIENT_DEVICE
#define bmREQ_CapeMCA_SEND    USB_SETUP_HOST_TO_DEVICE|USB_SETUP_TYPE_VENDOR|USB_SETUP_RECIPIENT_DEVICE

#define ACCESSORY_STRING_MANUFACTURER   0
#define ACCESSORY_STRING_MODEL          1
#define ACCESSORY_STRING_DESCRIPTION    2
#define ACCESSORY_STRING_VERSION        3
#define ACCESSORY_STRING_URI            4
#define ACCESSORY_STRING_SERIAL         5

#define CapeMCA_MAX_ENDPOINTS 3 //endpoint 0, bulk_IN, bulk_OUT

class CapeMCA;

class CapeMCA : public USBDeviceConfig, public UsbConfigXtracter {
private:
        /* ID strings */
        const char* manufacturer;
        const char* model;
        const char* description;
        const char* version;
        const char* uri;
        const char* serial;

        /* CapeMCA proprietary requests */
        uint8_t getProto(uint8_t* CapeMCAproto);
        // uint8_t sendStr(uint8_t index, const char* str);
        // uint8_t switchAcc(void);

protected:
        static const uint8_t epDataInIndex; // DataIn endpoint index
        static const uint8_t epDataOutIndex; // DataOUT endpoint index

        /* mandatory members */
        USB *pUsb;
        uint8_t bAddress;
        uint8_t bConfNum; // configuration number

        uint8_t bNumEP; // total number of EP in the configuration
        bool ready;

        /* Endpoint data structure */
        EpInfo epInfo[CapeMCA_MAX_ENDPOINTS];

        void PrintEndpointDescriptor(const USB_ENDPOINT_DESCRIPTOR* ep_ptr);

public:
        CapeMCA(USB *pUsb, const char* manufacturer,
                const char* model,
                const char* description,
                const char* version,
                const char* uri,
                const char* serial);

        // Methods for receiving and sending data
        uint8_t RcvData(uint16_t *nbytesptr, uint8_t *dataptr);
        uint8_t SndData(uint16_t nbytes, uint8_t *dataptr);


        // USBDeviceConfig implementation
        uint8_t ConfigureDevice(uint8_t parent, uint8_t port, bool lowspeed);
        uint8_t Init(uint8_t parent, uint8_t port, bool lowspeed);
        uint8_t Release();

        virtual uint8_t Poll() {
                return 0;
        };

        virtual uint8_t GetAddress() {
                return bAddress;
        };

        virtual bool isReady() {
                return ready;
        };

        virtual bool VIDPIDOK(uint16_t vid, uint16_t pid) {
                return (vid == CapeMCA_VID && (pid == CapeMCA_PID));
        };

        //UsbConfigXtracter implementation
        void EndpointXtract(uint8_t conf, uint8_t iface, uint8_t alt, uint8_t proto, const USB_ENDPOINT_DESCRIPTOR *ep);
}; //class CapeMCA : public USBDeviceConfig ...

/* get CapeMCA protocol version */

/* returns 2 bytes in *CapeMCAproto */
inline uint8_t CapeMCA::getProto(uint8_t* CapeMCAproto) {
        return ( pUsb->ctrlReq(bAddress, 0, bmREQ_CapeMCA_GET, CapeMCA_GETPROTO, 0, 0, 0, 2, 2, CapeMCAproto, NULL));
}

// /* send CapeMCA string */
// inline uint8_t CapeMCA::sendStr(uint8_t index, const char* str) {
//         return ( pUsb->ctrlReq(bAddress, 0, bmREQ_CapeMCA_SEND, CapeMCA_SENDSTR, 0, 0, index, strlen(str) + 1, strlen(str) + 1, (uint8_t*)str, NULL));
// }

// /* switch to accessory mode */
// inline uint8_t CapeMCA::switchAcc(void) {
//         return ( pUsb->ctrlReq(bAddress, 0, bmREQ_CapeMCA_SEND, CapeMCA_ACCSTART, 0, 0, 0, 0, 0, NULL, NULL));
// }

#endif // _CapeMCA_H_
