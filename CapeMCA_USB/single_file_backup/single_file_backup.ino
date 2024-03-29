#include <usbhub.h>
#include "pgmstrings.h"
#ifdef dobogusinclude
#include <spi4teensy3.h>
#include <Max3421e.h>
#include <Max3421e_constants.h>
#include <avr/pgmspace.h>
#endif
#include <SPI.h>

#define rHCTL       0xe8    //29<<3
#define bmBUSRST        0x01
#define WAD_NUM_EP 0x03
#define CONTROL_PIPE    0
#define INPUT_PIPE    1
#define OUTPUT_PIPE    2
#define EP_MAXPKTSIZE    64
#define EP_BULK    0x02
#define EP_POLL    0x00
#define DEV_DESCR_LEN    32
#define statusDeviceConnected 0x01
#define statusWADConnected 0x04
#define WAD_VID    0x4701
#define WAD_PID    0x290

#define WAD_ADDR 0x01
#define EP_OUT 0x01
#define EP_IN 0x01  // For some reason it is 0x03 that returns the info. Why is 0x83 not working???


MAX3421E  Max;
USB     Usb;
EpInfo ep_info[WAD_NUM_EP];
bool is_WAD_configured = false;

static unsigned char wad_status;


uint8_t cmd[2] = {0, 2};

void printArray(uint8_t* inputArray, int arraySize);
void resetArray(uint8_t* inputArray, int arraySize);

void getTimestamp(uint8_t* values);
int convert2HexToDec(uint8_t* buf);
float convertHexToFloat(int b0, int b1, int b2, int b3);
void print_hex(int v, int num_places);
void printProgStr(const char* str);
uint8_t getdevdescr(uint8_t addr, uint8_t &num_conf);
void WAD_init();
byte WAD_request();
void WAD_poll();

void setup(){
  Serial.begin( 115200 );
#if !defined(__MIPSEL__)
  while (!Serial); // Wait for serial port to connect - used on Leonardo, Teensy and other boards with built-in USB CDC serial connection
#endif
  Serial.println("Start");
  if (Usb.Init() == -1){
    Serial.println("OSC did not start.");
  }
  delay(200);
}

void loop(){
  Usb.Task();
  if(Usb.getUsbTaskState() == USB_STATE_RUNNING){
    if (!is_WAD_configured){
      WAD_init();  
    }else{
      byte rcode = WAD_request();
      if(rcode && rcode != hrNAK){
          Serial.print("Fail to sending cmd to device. Rcode: ");
          Serial.println(rcode);
          return;
      }
    }
   }
}

void WAD_init(){
   Serial.println("Initializing device.");
   USB_DEVICE_DESCRIPTOR buf;
   byte rcode = 0;
   USB_DEVICE_DESCRIPTOR* device_descriptor;
   wad_status = statusDeviceConnected;

   ep_info[CONTROL_PIPE] = *(Usb.getEpInfoEntry(0,0)); 
   ep_info[OUTPUT_PIPE].epAddr = EP_OUT;
   ep_info[OUTPUT_PIPE].epAttribs = EP_BULK;
   ep_info[OUTPUT_PIPE].bmSndToggle = bmSNDTOG0;
   ep_info[OUTPUT_PIPE].bmRcvToggle = bmRCVTOG0;
   ep_info[OUTPUT_PIPE].maxPktSize = EP_MAXPKTSIZE;
  
   ep_info[INPUT_PIPE].epAddr = EP_IN;
   ep_info[INPUT_PIPE].epAttribs = EP_BULK;
   ep_info[INPUT_PIPE].bmSndToggle = bmSNDTOG0;
   ep_info[INPUT_PIPE].bmRcvToggle = bmRCVTOG0;
   ep_info[INPUT_PIPE].maxPktSize = EP_MAXPKTSIZE;

   Usb.setEpInfoEntry(WAD_ADDR, WAD_NUM_EP, ep_info);

   rcode = Usb.getDevDescr(WAD_ADDR, 0, 0x12, ( uint8_t *)&buf );
   Serial.print("Vendor ID: 0x");
   Serial.println(buf.idVendor, HEX);
   Serial.print("Product ID: 0x");
   Serial.println(buf.idProduct, HEX);
   if((buf.idVendor != WAD_VID) || (buf.idProduct != WAD_PID)){
    Serial.println("The End Device is not a Welch Allyn Device.");
    return;
   }
   Serial.println("Connection Succeed");
   is_WAD_configured = true;

   Usb.setConf(WAD_ADDR, ep_info[CONTROL_PIPE].epAddr, 0x01);
   if(rcode){
      Serial.println("Failed to configure device.");
      return;
   }
   Serial.println("Device is successfully configured.");

   
   Serial.println("Device connected");
   delay(200);
}

byte WAD_request(){
  uint8_t buf[2048];

  uint32_t spectrum[sizeof(buf)/sizeof(uint32_t)];

  uint16_t len = sizeof buf;
  byte rcode = 0;
  
  rcode = Usb.outTransfer(WAD_ADDR, ep_info[OUTPUT_PIPE].epAddr, sizeof cmd, cmd);
  if(rcode){
    Serial.println("Sending command failed.");
    return rcode;
  } else {
    Serial.println("Succeeded in sending cmd.");
  }
  
  resetArray(buf, sizeof(buf));
  
  delay(1000);

  rcode = Usb.inTransfer(WAD_ADDR, ep_info[INPUT_PIPE].epAddr, &len, buf, EP_POLL);
 
  // Serial.print(len);
  // Serial.println(" bytes read");

  if(rcode && rcode != hrNAK){
    Serial.print("Failed to read command reply from 0x81. Rcode: ");
    Serial.println(rcode);
  } else {
    Serial.println("Succeeded in reading reply.");
    memcpy(spectrum, buf, sizeof(buf));

    for (int i = 0; i < sizeof(spectrum)/sizeof(spectrum[0]); i++) {
      Serial.print(i);
      Serial.print(", ");
      Serial.println(spectrum[i], DEC);
    }
  }

  delay(1000);

  return rcode;
}

void printArray(uint8_t* inputArray, int arraySize){
  Serial.print("Array Length: ");
  Serial.println(arraySize);
  Serial.println("Array: ");
  for (int i = 0; i < arraySize; i++) {
    int value = *(inputArray+i);
    if (value < 10)
      Serial.print("  ");
    else if (value < 100)
      Serial.print(" ");
    if (i%10 == 9)
      Serial.println(*(inputArray+i));
    else {
      Serial.print(*(inputArray+i));
      Serial.print(", ");
    }
  }
  Serial.println();
}

void resetArray(uint8_t* inputArray, int arraySize){
  for(int i = 0; i < arraySize; i++){
    inputArray[i] = 0;
  }
}
