#include <math.h>


// #define DEBUG

// Setpoints and control constants:
constexpr float HVAC_ON_DELTA_C = 6.0f;
constexpr float HVAC_ON_HYSTERESIS_C = 2.0f;
constexpr float HEAT_MAX_TEMP_C = 22.0f;
constexpr float COOL_MIN_TEMP_C = 20.0f;

constexpr float PWM_FREQ_HZ = 30.0f;
constexpr float DUTY_CYCLE_PERCENT_MAX = 95.0f;
constexpr float DUTY_CYCLE_PERCENT_MIN = 20.0f;
constexpr float DUTY_CYCLE_STEP = 0.02f;

// Pin Definitions
constexpr uint PWM_PIN = 10;
constexpr uint AMBIENT_TEMP_PIN = A0;
constexpr uint VENT_TEMP_PIN = A1;


// Temp measurement constants
constexpr float UNINIT_TEMP = -1000.0f;
constexpr uint32_t TEMP_AVG_CYCLES = 50;
constexpr float RBIAS_OHMS = 4700.0f;
constexpr float VBIAS_V = 3.3f;
constexpr float THERMISTOR_BETA = 3977.0f;
constexpr float R_INFINITY_OHMS = 10000.0f * exp(-THERMISTOR_BETA / 298.15);


void read_thermistor(uint pin, float *temp_out_degC){
  float volts = 3.30f * analogRead(pin) / 1024.0f;
  float ohms = (VBIAS_V * RBIAS_OHMS / volts) - RBIAS_OHMS;
  float temp_degC = THERMISTOR_BETA / log(ohms / R_INFINITY_OHMS) - 272.15f;

  if(*temp_out_degC == UNINIT_TEMP){
    // Uninitialized, just set the output to current value
    *temp_out_degC = temp_degC;
  } else {
    // Already initialized, start averaging
    *temp_out_degC = (*temp_out_degC * (TEMP_AVG_CYCLES-1) + temp_degC) / TEMP_AVG_CYCLES;
  }
}

void bit_bang_pwm(float duty_percent){
  static unsigned long period_start_ms = 0;

  // Enforce minimum and maximum duty cycles:
  if (duty_percent < 1.0f){
    digitalWrite(PWM_PIN, LOW);
    return;
  } else if (duty_percent > 99.0f){
    digitalWrite(PWM_PIN, HIGH);
    return;
  }


  // analogWrite(PWM_PIN, duty_percent / 100.0 * 255.0f);
  duty_percent /= 100.0f;
  unsigned long now_ms = millis();
  unsigned long period_time_ms = now_ms - period_start_ms;
  unsigned long period_ms = (unsigned long)(1000.0f / PWM_FREQ_HZ);
  unsigned long high_time_ms = (unsigned long)((float)period_ms * duty_percent);
  if(period_time_ms >= period_ms){
    // Time to start a new cycle with a rising edge
    digitalWrite(PWM_PIN, HIGH);
    period_start_ms = now_ms;
  } else if(period_time_ms > high_time_ms){
    // Done with high time, start low time
    digitalWrite(PWM_PIN, LOW);
  }
}

void setup() {
#ifdef DEBUG
  Serial.begin(2e6);
  Serial.println("Vent Controller");
#endif

  analogReadResolution(10);
  pinMode(PWM_PIN, OUTPUT);
}

void loop() {
  enum STATE {OFF, TURNON, ON, TURNOFF};
  static float ambient_degC = UNINIT_TEMP;
  static float vent_degC = UNINIT_TEMP;
  static STATE state = OFF;
  static float duty_percent = 0.0f;

  read_thermistor(AMBIENT_TEMP_PIN, &ambient_degC);
  read_thermistor(VENT_TEMP_PIN, &vent_degC);

  switch(state)
  {
    case OFF:
      if ((// Heating
        (vent_degC - ambient_degC >= HVAC_ON_DELTA_C) &&
        // Only turn on if temp is less than desired
        (ambient_degC < HEAT_MAX_TEMP_C))
        ||
        (// Cooling
        (vent_degC - ambient_degC >= HVAC_ON_DELTA_C) &&
        // Only turn on if temp is greater than desired
        (ambient_degC < COOL_MIN_TEMP_C)
        ))
      {
        state = TURNON;
        duty_percent = DUTY_CYCLE_PERCENT_MIN;
      }
      break;
    case TURNON:
      duty_percent += DUTY_CYCLE_STEP;
      if (duty_percent >= DUTY_CYCLE_PERCENT_MAX)
      {
        duty_percent = DUTY_CYCLE_PERCENT_MAX;
        state = ON;
      }
      break;
    case ON:
      if (// HVAC not running
        abs(vent_degC - ambient_degC) <= HVAC_ON_DELTA_C - HVAC_ON_HYSTERESIS_C)
      {
        state = TURNOFF;
      }
      break;
    case TURNOFF:
      duty_percent -= DUTY_CYCLE_STEP;
      if(duty_percent <= DUTY_CYCLE_PERCENT_MIN)
      {
        duty_percent = 0.0f; // Set to zero to ensure it's off
        state = OFF;
      }
      break;
  }

  bit_bang_pwm(duty_percent);

#ifdef DEBUG
  Serial.print("Ambient = ");
  Serial.print(ambient_degC);
  Serial.print("C, Vent = ");
  Serial.print(vent_degC);
  Serial.print("C, duty = ");
  Serial.print(duty_percent);
  Serial.println("%");
#endif
}

