#include <math.h>

// Pin Definitions
constexpr uint FAN_PIN = 10;
constexpr uint AMBIENT_TEMP_PIN = A0;
constexpr uint VENT_TEMP_PIN = A1;

// PWM Constants:
constexpr float PWM_FREQ_HZ = 200.0f;
constexpr float DUTY_CYCLE_PERCENT = 85.0f;

// Temp measurement constants
constexpr uint32_t TEMP_AVG_CYCLES = 100;
constexpr float RBIAS_OHMS = 4700.0f;
constexpr float VBIAS_V = 3.3f;
constexpr float THERMISTOR_BETA = 3977.0f;
constexpr float R_INFINITY_OHMS = 10000.0f * exp(-THERMISTOR_BETA / 298.15);


void read_thermistor(uint pin, float *temp_out_degC){
  float volts = 3.30f * analogRead(pin) / 1024.0f;
  float ohms = (VBIAS_V * RBIAS_OHMS / volts) - RBIAS_OHMS;
  float temp_degC = THERMISTOR_BETA / log(ohms / R_INFINITY_OHMS) - 272.15f;

  if(*temp_out_degC == -1000){
    // Uninitialized, just set the output to this value
    *temp_out_degC = temp_degC;
  } else {
    // Already initialized, start averaging
    *temp_out_degC = (*temp_out_degC * (TEMP_AVG_CYCLES-1) + temp_degC) / TEMP_AVG_CYCLES;
  }
}

void setup() {
  Serial.begin(115200);
  delay(5000);
  Serial.println("Vent Controller");

  analogReadResolution(10);
  pinMode(FAN_PIN, OUTPUT);
}

enum STATE {OFF, ON};


void loop() {
  static float ambient_degC = -1000;
  static float vent_degC = -1000;
  static STATE state;

  read_thermistor(AMBIENT_TEMP_PIN, &ambient_degC);
  read_thermistor(VENT_TEMP_PIN, &vent_degC);

  switch(state){
    case OFF:
      if (
        // HVAC running
        (abs(vent_degC - ambient_degC) >= 6.0f) ||
        // Out of comfort range
        ((ambient_degC < 17.0f) || (ambient_degC > 24.0f))){
        state = ON;
        digitalWrite(FAN_PIN, HIGH);
      }
      break;
    case ON:
      if (
        // HVAC not running
        (abs(vent_degC - ambient_degC) <= 5.0f) &&
        // Back in comfort range
        ((ambient_degC > 17.5f) && (ambient_degC < 23.5f))){
        state = OFF;
        digitalWrite(FAN_PIN, LOW);
      }
      break;
  }

  Serial.print("Ambient = ");
  Serial.print(ambient_degC);
  Serial.print("C, Vent = ");
  Serial.print(vent_degC);
  Serial.println("C");
}
