#include <BTS7960.h>

// ==================== TACTILE SWITCH ====================
#define SWITCH_PIN 34

// ==================== FLOAT SWITCH ======================
#define FLOAT_SWITCH 35

// ==================== SOLENOID VALVE =====================
#define SOLENOID_PIN 27
// HIGH = SOV ON
// LOW  = SOV OFF

// ==================== PUMP 1 =============================
#define L_EN1   13
#define R_EN1   26
#define L_PWM1  32
#define R_PWM1  33

// ==================== PUMP 2 =============================
#define L_EN2   16
#define R_EN2   17
#define L_PWM2  18
#define R_PWM2  19
az
// ==================== PUMP 3 =============================
#define L_EN3   21
#define R_EN3   22
#define L_PWM3  23
#define R_PWM3  25

// ==================== FLOW SENSOR =========================
#define FLOW_SENSOR_PIN 14

// ==================== MOTOR OBJECTS =======================
BTS7960 pump1(L_EN1, R_EN1, L_PWM1, R_PWM1);
BTS7960 pump2(L_EN2, R_EN2, L_PWM2, R_PWM2);
BTS7960 pump3(L_EN3, R_EN3, L_PWM3, R_PWM3);

// ==================== PWM VALUES ==========================
int PUMP1_PWM = 128;
int PUMP2_PWM = 255;
int PUMP3_PWM = 255;

// ==================== CASE 2 VOLUME CONTROL ===============
const double CASE2_TARGET_LITRES = 3.0;

double case2TransferredLitres = 0.0;
double case1TransferredLitres = 0.0;

bool case2Completed = false;

// ==================== FLOW SENSOR =========================
volatile unsigned long pulseCount = 0;

void IRAM_ATTR countPulse() {
  pulseCount++;
}

// ==================== SYSTEM STATES =======================
bool previousSystemState = false;
int previousFloatState = -1;

unsigned long lastFlowPrint = 0;

// ==================== SETUP ===============================
void setup() {

  Serial.begin(115200);

  // GPIO 34 has no internal pull-up/pull-down.
  // Provide an external bias or suitable switch circuit.
  pinMode(SWITCH_PIN, INPUT);

  pinMode(FLOAT_SWITCH, INPUT_PULLUP);
  pinMode(SOLENOID_PIN, OUTPUT);

  // Initially, everything is OFF.
  digitalWrite(SOLENOID_PIN, LOW);

  // Initialize Pump 1
  pump1.begin();
  pump1.enable();
  pump1.pwm = 0;
  pump1.stop();

  // Initialize Pump 2
  pump2.begin();
  pump2.enable();
  pump2.pwm = 0;
  pump2.stop();

  // Initialize Pump 3
  pump3.begin();
  pump3.enable();
  pump3.pwm = 0;
  pump3.stop();

  // Initialize flow sensor
  pinMode(FLOW_SENSOR_PIN, INPUT_PULLUP);

  attachInterrupt(
    digitalPinToInterrupt(FLOW_SENSOR_PIN),
    countPulse,
    FALLING
  );

  lastFlowPrint = millis();

  Serial.println("================================");
  Serial.println("MASTER SYSTEM INITIALIZED");
  Serial.println("SOV: OFF");
  Serial.println("PUMP 1: OFF");
  Serial.println("PUMP 2: OFF");
  Serial.println("PUMP 3: OFF");
  Serial.println("CASE 2 TARGET: 3.0 L");
  Serial.println("================================");
}

// ==================== MAIN LOOP ==========================
void loop() {

  int switchState = digitalRead(SWITCH_PIN);
  int floatState = digitalRead(FLOAT_SWITCH);

  // LOW  = Case 1
  // HIGH = Case 2
  bool case1 = (switchState == LOW);

  // =======================================================
  // CASE 1: TACTILE SWITCH ON
  // Pump 2 ON
  // Float switch controls SOV and Pump 1
  // Pump 3 OFF
  // =======================================================
  if (case1) {

    // Execute when entering Case 1
    if (!previousSystemState) {

      Serial.println();
      Serial.println("[CASE 1] TACTILE SWITCH ON");

      // Reset Case 2 completion latch so a new cycle
      // can run after the operator switches back to Case 1.
      case2Completed = false;
      case2TransferredLitres = 0.0;

      // Ensure SOV starts OFF
      digitalWrite(SOLENOID_PIN, LOW);

      // Ensure Pump 1 starts OFF until float is checked
      pump1.pwm = 0;
      pump1.stop();

      // Ensure Pump 3 is OFF
      pump3.pwm = 0;
      pump3.stop();

      // Start Pump 2
      pump2.pwm = PUMP2_PWM;
      pump2.back();

      Serial.println("Pump 2 ON");
      Serial.println("Pump 3 OFF");

      // Force float-switch logic to execute
      previousFloatState = -1;
    }

    // Float switch control
    if (floatState != previousFloatState) {

      Serial.println();
      Serial.println("[CASE 1] FLOAT SWITCH CONTROL");

      if (floatState == LOW) {

        Serial.println("Float switch: ON");

        // SOV OFF first
        digitalWrite(SOLENOID_PIN, LOW);
        delay(300);

        // Pump 1 OFF
        pump1.pwm = 0;
        pump1.stop();

        Serial.println("SOV OFF | Pump 1 OFF");
      }

      else {

        Serial.println("Float switch: OFF");

        // SOV ON first
        digitalWrite(SOLENOID_PIN, HIGH);
        delay(300);

        // Pump 1 ON
        pump1.pwm = PUMP1_PWM;
        pump1.back();

        Serial.println("SOV ON | Pump 1 ON");
      }

      previousFloatState = floatState;
    }

    previousSystemState = true;
  }

  // =======================================================
  // CASE 2: TACTILE SWITCH OFF
  // SOV OFF
  // Pump 1 OFF
  // Pump 2 OFF
  // Pump 3 ON UNTIL 3 L IS TRANSFERRED
  // =======================================================
  else {

    // Execute only once when entering Case 2
    if (previousSystemState || previousFloatState != -1) {

      Serial.println();
      Serial.println("[CASE 2] TACTILE SWITCH OFF");

      // Reset Case 2 volume at the start of the cycle
      case2TransferredLitres = 0.0;
      case2Completed = false;

      // SOV OFF first
      digitalWrite(SOLENOID_PIN, LOW);
      Serial.println("SOV OFF");

      delay(300);

      // Pump 1 OFF
      pump1.pwm = 0;
      pump1.stop();
      Serial.println("Pump 1 OFF");

      // Pump 2 OFF
      pump2.pwm = PUMP2_PWM;
      pump2.back();
      Serial.println("Pump 2 OFF");

      // Start Pump 3 only if Case 2 has not completed
      pump3.pwm = PUMP3_PWM;
      pump3.back();

      Serial.println("Pump 3 ON");
      Serial.println("Target volume: 3.0 L");
    }

    // Keep Pump 3 OFF after reaching the target
    if (case2Completed) {

      digitalWrite(SOLENOID_PIN, LOW);

      pump1.pwm = 0;
      pump1.stop();

      pump2.pwm = 0;
      pump2.stop();

      pump3.pwm = 0;
      pump3.stop();
    }

    previousSystemState = false;
    previousFloatState = -1;
  }

  // =======================================================
  // STAGE 3: FLOW MEASUREMENT AND VOLUME ACCUMULATION
  // =======================================================
  unsigned long currentTime = millis();

  if (currentTime - lastFlowPrint >= 1000) {

    unsigned long elapsed = currentTime - lastFlowPrint;
    lastFlowPrint = currentTime;

    // Safely read and reset the pulse counter
    noInterrupts();
    unsigned long pulses = pulseCount;
    pulseCount = 0;
    interrupts();

    // Datasheet calibration: Q = a*f + b
    const float a = 0.008176;
    const float b = 0.151951;

    // Pulse frequency in Hz
    float frequency =
        (float)pulses * 1000.0 / (float)elapsed;

    // Flow rate in L/min
    float flowLmin = a * frequency + b;

    // Avoid counting the calibration offset as real flow
    // when there are no pulses.
    if (pulses == 0) {
      flowLmin = 0.0;
    }

    // Flow rate in mL/s
    float flowMLsec = flowLmin * 1000.0 / 60.0;

    // Convert measured flow to transferred volume
    double volumeIncrement =
        (double)flowLmin * ((double)elapsed / 60000.0);

    // Case 1 total volume: retained separately
    if (case1) {

      case1TransferredLitres += volumeIncrement;
    }

    // Case 2 total volume: count only while Pump 3 is active
    else if (!case2Completed) {

      case2TransferredLitres += volumeIncrement;

      // Stop immediately after the latest volume update
      if (case2TransferredLitres >= CASE2_TARGET_LITRES) {

        // SOV OFF
        digitalWrite(SOLENOID_PIN, LOW);

        // Stop Pump 1
        pump1.pwm = 0;
        pump1.stop();

        // Stop Pump 2
        pump2.pwm = 0;
        pump2.stop();

        // Stop Pump 3
        pump3.pwm = 0;
        pump3.stop();

        case2Completed = true;

        Serial.println();
        Serial.println("================================");
        Serial.println("[CASE 2 COMPLETE]");
        Serial.println("3 L TARGET REACHED");
        Serial.println("SOV OFF");
        Serial.println("PUMP 1 OFF");
        Serial.println("PUMP 2 OFF");
        Serial.println("PUMP 3 OFF");
        Serial.println("SYSTEM SHUT DOWN");
        Serial.println("================================");
      }
    }

    // ==================== SERIAL REPORT ==================

    Serial.println();
    Serial.println("========== FLOW REPORT ==========");

    Serial.print("Active case: ");
    if (case1) {
      Serial.println("CASE 1");
    } else if (case2Completed) {
      Serial.println("CASE 2 COMPLETE - SYSTEM OFF");
    } else {
      Serial.println("CASE 2");
    }

    Serial.print("Elapsed time: ");
    Serial.print(currentTime / 1000.0, 1);
    Serial.println(" s");

    Serial.print("Pulse count: ");
    Serial.println(pulses);

    Serial.print("Frequency: ");
    Serial.print(frequency, 2);
    Serial.println(" Hz");

    Serial.print("Flow rate: ");
    Serial.print(flowLmin, 4);
    Serial.println(" L/min");

    Serial.print("Flow rate: ");
    Serial.print(flowMLsec, 2);
    Serial.println(" mL/s");

    Serial.print("Case 1 transferred volume: ");
    Serial.print(case1TransferredLitres, 4);
    Serial.println(" L");

    Serial.print("Case 2 transferred volume: ");
    Serial.print(case2TransferredLitres, 4);
    Serial.println(" L");

    Serial.println("=================================");

    // CSV output for plotting
    Serial.println(
      "PLOT,time_s,flow_L_min,flow_mL_s,case1_L,case2_L"
    );

    Serial.print("PLOT,");
    Serial.print(currentTime / 1000.0, 1);
    Serial.print(",");
    Serial.print(flowLmin, 4);
    Serial.print(",");
    Serial.print(flowMLsec, 2);
    Serial.print(",");
    Serial.print(case1TransferredLitres, 4);
    Serial.print(",");
    Serial.println(case2TransferredLitres, 4);
  }

  delay(10);
}