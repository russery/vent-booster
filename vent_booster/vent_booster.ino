/*
Controls an HVAC vent booster fan to improve flow in rooms with poor HVAC flow.

Copyright (C) 2024  Robert Ussery

This program is free software: you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation, either version 3 of the License, or
(at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with this program.  If not, see <https://www.gnu.org/licenses/>.
*/

#include "bsp.h"
#include "display.h"
#include <math.h>

// Setpoints and control constants:
constexpr float HVAC_ON_DELTA_C = 20.0f;
constexpr float HVAC_ON_HYSTERESIS_C = 5.0f;
constexpr float SETPOINT_HYSTERESIS_C = 0.5f;
constexpr float SETPOINT_CONST =
    50.0f; // Arbitrary to scale setpoint pot position to degC

constexpr float PWM_FREQ_HZ = 30.0f;
constexpr float DUTY_CYCLE_PERCENT_MAX = 100.0f;
constexpr float DUTY_CYCLE_PERCENT_MIN = 97.0f;
constexpr float DUTY_CYCLE_STEP = 0.005f;

constexpr unsigned long LOCKOUT_DURATION_MS = 5 * 60 * 1000;

// Temp measurement constants
constexpr float UNINIT_TEMP = -1000.0f;
constexpr uint32_t TEMP_AVG_CYCLES = 100;
constexpr uint32_t SETPOINT_AVG_CYCLES = 50;
constexpr float RBIAS_OHMS = 4700.0f;
constexpr float VBIAS_V = 3.3f;
constexpr float THERMISTOR_BETA = 3977.0f;
constexpr float R_INFINITY_OHMS = 10000.0f * exp(-THERMISTOR_BETA / 298.15);

Display display(&Wire);

float adc_counts_to_volts(int adc_counts) {
  return 3.30f * adc_counts / 1024.0f;
}

void read_thermistor(uint pin, float *temp_out_degC) {
  float volts = adc_counts_to_volts(analogRead(pin));
  float ohms = (VBIAS_V * RBIAS_OHMS / volts) - RBIAS_OHMS;
  float temp_degC = THERMISTOR_BETA / log(ohms / R_INFINITY_OHMS) - 272.15f;

  if (*temp_out_degC == UNINIT_TEMP) {
    // Uninitialized, just set the output to current value
    *temp_out_degC = temp_degC;
  } else {
    // Already initialized, start averaging
    *temp_out_degC =
        (*temp_out_degC * (TEMP_AVG_CYCLES - 1) + temp_degC) / TEMP_AVG_CYCLES;
  }
}

void read_setpoint_pot(float *temp_setpoint_degC) {
  float setpoint =
      adc_counts_to_volts(analogRead(BSP::SETPOINT_PIN)) * SETPOINT_CONST;
  if (*temp_setpoint_degC == UNINIT_TEMP) {
    // Uninitialized, just set the output to current value
    *temp_setpoint_degC = setpoint;
  } else {
    // Already initialized, start averaging
    *temp_setpoint_degC =
        (*temp_setpoint_degC * (SETPOINT_AVG_CYCLES - 1) + setpoint) /
        SETPOINT_AVG_CYCLES;
  }
}

// cppcheck-suppress unusedFunction
void setup() {
  analogReadResolution(10);
  pinMode(BSP::PWM_PIN, OUTPUT);

  Wire.begin(BSP::I2C_SDA_PIN, BSP::I2C_SCL_PIN);
  display.Start();
}

// cppcheck-suppress unusedFunction
void loop() {
  enum STATE { OFF, TURNON, ON, TURNOFF, LOCKOUT};
  constexpr char MODE_STR_OFF[8] = "OFF";
  constexpr char MODE_STR_HEAT[8] = "HEAT";
  constexpr char MODE_STR_COOL[8] = "COOL";
  constexpr char MODE_STR_LOCK[8] = "LOCK";
  static const char *curr_mode_str = MODE_STR_OFF;
  static float ambient_degC = UNINIT_TEMP;
  static float vent_degC = UNINIT_TEMP;
  static float setpoint_degC = UNINIT_TEMP;
  static STATE state = OFF;
  static float duty_percent = 0.0f;
  static unsigned long lockout_start_ms;

  read_thermistor(BSP::AMBIENT_TEMP_PIN, &ambient_degC);
  read_thermistor(BSP::VENT_TEMP_PIN, &vent_degC);
  read_setpoint_pot(&setpoint_degC);

  switch (state) {
  case OFF:
    if ( // Heating
        (vent_degC - ambient_degC >= HVAC_ON_DELTA_C) &&
        (ambient_degC < setpoint_degC)) {
      curr_mode_str = MODE_STR_HEAT;
      state = TURNON;
      duty_percent = DUTY_CYCLE_PERCENT_MIN;
    }
    if ( // Cooling
        (ambient_degC - vent_degC >= HVAC_ON_DELTA_C) &&
        (ambient_degC > setpoint_degC)) {
      curr_mode_str = MODE_STR_COOL;
      state = TURNON;
      duty_percent = DUTY_CYCLE_PERCENT_MIN;
    }
    break;
  case TURNON:
    duty_percent += DUTY_CYCLE_STEP;
    if (duty_percent >= DUTY_CYCLE_PERCENT_MAX) {
      duty_percent = DUTY_CYCLE_PERCENT_MAX;
      state = ON;
    }
    break;
  case ON:
    if (( // HVAC not running
            abs(vent_degC - ambient_degC) <=
            HVAC_ON_DELTA_C - HVAC_ON_HYSTERESIS_C) ||
        ( // Heat running, but over setpoint temp:
            (vent_degC - ambient_degC >= HVAC_ON_DELTA_C) &&
            (ambient_degC > setpoint_degC + SETPOINT_HYSTERESIS_C)) ||
        ( // Cooling running, but under setpoint temp:
            (ambient_degC - vent_degC >= HVAC_ON_DELTA_C) &&
            (ambient_degC < setpoint_degC - SETPOINT_HYSTERESIS_C))) {
      state = TURNOFF;
    }
    break;
  case TURNOFF:
    duty_percent -= DUTY_CYCLE_STEP;
    if (duty_percent <= DUTY_CYCLE_PERCENT_MIN) {
      duty_percent = 0.0f; // Set to zero to ensure it's off
      curr_mode_str = MODE_STR_LOCK;
      state = LOCKOUT;
      lockout_start_ms = millis();
    }
    break;
  case LOCKOUT:
    if (abs((long long)millis() - (long long)lockout_start_ms)> LOCKOUT_DURATION_MS){
      curr_mode_str = MODE_STR_OFF;
      state = OFF;
    }
    break;
  }

  analogWrite(BSP::PWM_PIN, (int)(duty_percent / 100 * 255));

  display.Update(setpoint_degC, ambient_degC, vent_degC, curr_mode_str);
}
