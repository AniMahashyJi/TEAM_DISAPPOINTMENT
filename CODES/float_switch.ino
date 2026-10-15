#define FLOAT_SWITCH 35

void setup() {
  Serial.begin(115200);
  pinMode(FLOAT_SWITCH, INPUT_PULLUP);
}

void loop() {
  int state = digitalRead(FLOAT_SWITCH);

  if (state == HIGH) {
    Serial.println("Switch ON");
  } else {
    Serial.println("Switch OFF");
  }

  delay(500);
}