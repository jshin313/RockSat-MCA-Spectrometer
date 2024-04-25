
#define SWITCH_PIN 22

void setup(){
  pinMode(SWITCH_PIN, OUTPUT);
  digitalWrite(SWITCH_PIN, LOW);
}

void loop(){
  digitalWrite(SWITCH_PIN, HIGH);
  delay(3000);
  digitalWrite(SWITCH_PIN, LOW);
  delay(3000);
}