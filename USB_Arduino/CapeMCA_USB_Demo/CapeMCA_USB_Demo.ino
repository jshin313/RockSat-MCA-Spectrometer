#include <usbhub.h>
#include "pgmstrings.h"
#include "desc.h"
#ifdef dobogusinclude
#include <spi4teensy3.h>
#endif
#include <SPI.h>
#include <avr/wdt.h>

#define CapeMCA_NUM_EP 0x03
#define CONTROL_PIPE 0
#define INPUT_PIPE 1
#define OUTPUT_PIPE 2
#define EP_MAXPKTSIZE 64
#define EP_BULK 0x02
#define EP_POLL 0x00
#define DEV_DESCR_LEN 32
#define statusDeviceConnected 0x01
#define CapeMCA_VID 0x4701
#define CapeMCA_PID 0x290

#define CapeMCA_ADDR 0x01
#define EP_OUT 0x01
#define EP_IN 0x01  // For some reason it is 0x01 that returns the info. Why is 0x81 not working???

#define SWITCH_PIN 22

USB Usb;
EpInfo ep_info[CapeMCA_NUM_EP];
bool is_CapeMCA_configured = false;

static unsigned char mca_status;

uint8_t cmd[2] = { 0, 1 };  // returns 256x32-bit spectrum
#define SPECTRUM_SIZE 256

// uint8_t cmd[2] = {0, 2}; // returns 512x32-bit spectrum
// #define SPECTRUM_SIZE 512

void resetArray(uint8_t* inputArray, int arraySize);
void wdt_setup();

void CapeMCA_init();
byte CapeMCA_request();

void setup() {
  Serial.begin(115200);
#if !defined(__MIPSEL__)
  while (!Serial)
    ;  // Wait for serial port to connect - used on Leonardo, Teensy and other boards with built-in USB CDC serial connection
#endif
  Serial.println("Start");

  // For switching spectrometer on and off
  pinMode(SWITCH_PIN, OUTPUT);
  digitalWrite(SWITCH_PIN, LOW);

  while (Usb.Init() == -1) {
    Serial.println("USB Initialization FAILED.");
    delay(200);
  }
  Serial.println("USB Initialization Succeeded.");
  delay(200);

  // wdt_setup();
}

void loop() {

  Usb.Task();
  if (Usb.getUsbTaskState() == USB_STATE_RUNNING) {
    Serial.println("USB_STATE_RUNNING");

    if (!is_CapeMCA_configured) {
      CapeMCA_init();
    } else {
      byte rcode = CapeMCA_request();
      if (rcode == hrSUCCESS || rcode == hrBUSY) {
        wdt_reset();
      }
    }
  } else if (Usb.getUsbTaskState() != 0x20 && Usb.getUsbTaskState() != 0x40 && Usb.getUsbTaskState() != 0x51 && Usb.getUsbTaskState() != 0x50) {
      Serial.print("USB Task State: ");
      Serial.println(Usb.getUsbTaskState(), HEX);
      // // Turn spectrometer off and on again
      // wdt_disable();
      // digitalWrite(SWITCH_PIN, LOW);
      // delay(3000);
      // digitalWrite(SWITCH_PIN, HIGH);
      // delay(5000);
      // wdt_setup();
  }
}

void CapeMCA_init() {
  Serial.println("Initializing device.");

  USB_DEVICE_DESCRIPTOR buf;
  byte rcode = 0;
  USB_DEVICE_DESCRIPTOR* device_descriptor;
  mca_status = statusDeviceConnected;

  ep_info[CONTROL_PIPE] = *(Usb.getEpInfoEntry(0, 0));
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

  Usb.setEpInfoEntry(CapeMCA_ADDR, CapeMCA_NUM_EP, ep_info);

  rcode = Usb.getDevDescr(CapeMCA_ADDR, 0, 0x12, (uint8_t*)&buf);
  Serial.print("Vendor ID: 0x");
  Serial.println(buf.idVendor, HEX);
  Serial.print("Product ID: 0x");
  Serial.println(buf.idProduct, HEX);
  if ((buf.idVendor != CapeMCA_VID) || (buf.idProduct != CapeMCA_PID)) {
    Serial.println("The End Device is not a CapeMCA Device.");
    return;
  }
  Serial.println("USB Configuration of MCA Succeeded.");

  Usb.setConf(CapeMCA_ADDR, ep_info[CONTROL_PIPE].epAddr, 0x01);
  if (rcode) {
    Serial.println("Failed to configure device.");
    return;
  }
  Serial.println("Device is successfully configured.");

  is_CapeMCA_configured = true;

  Serial.println("Device connected");
  delay(200);

  AddressPool& addrPool = Usb.GetAddressPool();
  UsbDevice* p = addrPool.GetUsbDevicePtr(CapeMCA_ADDR);
  PrintAllAddresses(p);
  PrintAllDescriptors(p, &Usb);
}

byte CapeMCA_request() {
  Serial.println("Requesting data...");

  uint8_t buf[SPECTRUM_SIZE * 4];
  uint32_t spectrum[SPECTRUM_SIZE];

  uint16_t len = sizeof buf;
  byte rcode = Usb.outTransfer(CapeMCA_ADDR, ep_info[OUTPUT_PIPE].epAddr, sizeof cmd, cmd);
  if (rcode) {
    Serial.println("Sending command failed.");
    return rcode;
  } else {
    Serial.println("Succeeded in sending cmd.");
  }

  resetArray(buf, sizeof(buf));

  delay(10);

  rcode = Usb.inTransfer(CapeMCA_ADDR, ep_info[INPUT_PIPE].epAddr, &len, buf, EP_POLL);

  if (rcode) {
    Serial.print("Failed to read command reply from 0x81. Rcode: ");
    Serial.println(rcode);
  } else {


    Serial.println("Succeeded in reading reply.");
    memcpy(spectrum, buf, sizeof(buf));

    for (int i = 0; i < SPECTRUM_SIZE; i++) { 
      Serial.print(i);
      Serial.print(", ");
      Serial.println(spectrum[i], DEC);
    }

    delay(10);

  }

  delay(1000);

  return rcode;
}

void resetArray(uint8_t* inputArray, int arraySize) {
  for (int i = 0; i < arraySize; i++) {
    inputArray[i] = 0;
  }
}

void wdt_setup() {
  wdt_disable();
  delay(3000);
  wdt_enable(WDTO_2S);
  Serial.println("WDT ENABLED");
}