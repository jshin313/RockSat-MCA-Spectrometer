/*

  Multiple Serial test

  Receives from the main serial port, sends to the others.

  Receives from serial port 1, sends to the main serial (Serial 0).

  This example works only with boards with more than one serial like Arduino Mega, Due, Zero etc.

  The circuit:

  - any serial device attached to Serial port 1

  - Serial Monitor open on Serial port 0

  created 30 Dec 2008

  modified 20 May 2012

  by Tom Igoe & Jed Roach

  modified 27 Nov 2015

  by Arturo Guadalupi

  This example code is in the public domain.

*/
bool command_executing = false;
const int cmd[] = {0, 1};

void setup() {

  // initialize both serial ports:

  Serial.begin(115200);

  Serial1.begin(115200);
}

void loop() {

  if (Serial1.available() && command_executing) {
    // Serial1.print("Serial1 Available");

    int inByte = Serial1.read();

    Serial.print(inByte);

  }

  if (Serial1.available() && !command_executing) {

    // int inByte = Serial.read();

    Serial1.write(cmd[0]);
    Serial1.write(cmd[1]);
  }
  
}