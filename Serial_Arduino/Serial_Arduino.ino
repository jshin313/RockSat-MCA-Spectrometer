bool command_executing = false;
const int cmd_1024[] = {0, 1};


void setup() {

  // initialize both serial ports:

  Serial.begin(115200);

  Serial1.begin(115200);
}

void loop() {
  uint32_t spectrum[256];

  if (Serial1.available() > 0 && !command_executing) {

    // int inByte = Serial.read();
    Serial.println("Writing command ");
    Serial1.write(cmd_packet0[0]);
    Serial1.write(cmd_packet0[1]);
    command_executing = true;

    char buffer[1024];
    Serial1.readBytes(buffer, 1024);
    Serial.println("Read 1024 bytes:");

    memcpy(spectrum, buffer, sizeof(buffer));
    for (int i= 0; i < sizeof(spectrum); i++) {
      Serial.println(spectrum[i], DEC);
    }
    Serial.println();
    
    command_executing = false;
    delay(100);
  }
  
}