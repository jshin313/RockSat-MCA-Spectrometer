#include "CapeMCA.h"


// Satisfy IDE, which only needs to see the include statment in the ino.
#ifdef dobogusinclude
#include <spi4teensy3.h>
#endif
#include <SPI.h>

USB Usb;
CapeMCA mca(&Usb, "TKJElectronics", // Manufacturer Name
              "ArduinoBlinkLED", // Model Name
              "Example sketch for the USB Host Shield", // Description (user-visible string)
              "1.0", // Version
              "http://www.tkjelectronics.dk/uploads/ArduinoBlinkLED.apk", // URL (web page to visit if no installed apps support the accessory)
              "123456789"); // Serial Number (optional)

uint32_t timer;
bool connected;

unsigned char cmd[2] = {0, 1};

void setup() {
  Serial.begin(115200);
#if !defined(__MIPSEL__)
  while (!Serial); // Wait for serial port to connect - used on Leonardo, Teensy and other boards with built-in USB CDC serial connection
#endif
  if (Usb.Init() == -1) {
    Serial.print("\r\nOSCOKIRQ failed to assert");
    while (1); // halt
  }
  Serial.print("\r\nUSB Initialized.");
}

void loop() {
  Usb.Task();

  if (mca.isReady()) {
    if (!connected) {
      connected = true;
      Serial.print(F("\r\nConnected to CapeMCA device."));
    }

    
    // uint8_t rcode = Usb.outTransfer(0x01, 0x1, sizeof(cmd), cmd);
    uint8_t rcode = mca.SndData(sizeof(cmd), (uint8_t*)&cmd);
    if (rcode == hrSUCCESS) {
      Serial.print(F("\r\nCmd Send Success: {"));
      Serial.print(cmd[0], HEX);
      Serial.print(F(", "));
      Serial.print(cmd[1], HEX);
      Serial.print(F("}"));
    } else {
      Serial.print(F("\r\nCmd Send Failed. Error Code: "));
      Serial.print(rcode, HEX);
    }
    
    delay(1000);

    uint8_t reply[1024];
    uint16_t bytes_read = 0;
    rcode = mca.RcvData(&bytes_read, reply);
    // rcode = Usb.inTransfer(0x01, 0x81, &bytes_read, reply);

    if (rcode) {
      Serial.print(F("\r\nFailed to read reply. Error Code: "));
      Serial.print(rcode, HEX);
    } else if (bytes_read > 0) {
      Serial.print(F("\r\nSuccessfully read data Packet: "));
      Serial.print(reply[0]);
    }


  } else {
    if (connected) {
      connected = false;
      Serial.print(F("\r\nCape MCA Device Disconnected."));
    }
  }

  delay(1000);
}