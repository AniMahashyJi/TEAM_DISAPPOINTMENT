#define SOLENOID_PIN 27
void setup() {
  pinMode(SOLENOID_PIN, OUTPUT);
  digitalWrite(SOLENOID_PIN, LOW);
}
void loop() {
  digitalWrite(SOLENOID_PIN, HIGH);  // ON
  delay(5000);                       // 3 seconds

  digitalWrite(SOLENOID_PIN, LOW);   // OFF
  delay(5000);
}