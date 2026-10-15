#include <BTS7960.h>

#define L_EN   16
#define R_EN   17
#define L_PWM  18
#define R_PWM  19

BTS7960 motor1(L_EN, R_EN, L_PWM, R_PWM);

void setup() {
  Serial.begin(9600);

  motor1.begin();
  motor1.enable();
}

void loop() {

  motor1.pwm = 255;
  motor1.back();

  delay(100);
}