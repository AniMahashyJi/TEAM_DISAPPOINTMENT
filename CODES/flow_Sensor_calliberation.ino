#include <BTS7960.h>

#define L_EN   16
#define R_EN   17
#define L_PWM  18
#define R_PWM  19

#define FLOW_SENSOR_PIN 14

BTS7960 motor1(L_EN, R_EN, L_PWM, R_PWM);

volatile unsigned long pulseCount = 0;

void IRAM_ATTR countPulse() {
  pulseCount++;
}

void setup() {
  Serial.begin(115200);

  // Motor setup
  motor1.begin();
  motor1.enable();

  // Flow sensor setup
  pinMode(FLOW_SENSOR_PIN, INPUT_PULLUP);
  attachInterrupt(
    digitalPinToInterrupt(FLOW_SENSOR_PIN),
    countPulse,
    FALLING
  );

  // Start motor
  motor1.pwm = 255; // pwm setting at different for different values, 0.2*255, 0.4*255...etc
  motor1.back();
}

void loop() {
  // Read and reset pulse count safely
  noInterrupts();
  unsigned long pulses = pulseCount;
  pulseCount = 0;
  interrupts();

  // Print pulses counted in the last second
  Serial.println(pulses);

  delay(1000);
}