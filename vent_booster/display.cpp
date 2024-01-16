/*
Handles display of data on a SSD1306-compatible OLED display.

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

#include "display.h"
#include "bsp.h"
#include <algorithm>

void Display::Start(void) {
  if (!display_.begin(
          SSD1306_SWITCHCAPVCC, // Generate display voltage from 3.3V internally
          SCREEN_ADDRESS,
          false, // reset = false; no hardware reset
          false  // periphBegin = false; Wire.begin() is handled elsewhere
          )) {
    Serial.println(F("SSD1306 allocation failed"));
    return;
  }

  display_.dim(true);
  display_.setTextColor(SSD1306_WHITE);
  display_.setTextWrap(false);
  display_.cp437(true); // Use "Code Page 437" font
  display_.clearDisplay();
  display_.display();
}

float c_to_f(float temp_C) { return temp_C * 9 / 5 + 32; }

uint get_whole(float num) { return (uint)num; }

uint get_mantissa(float num) { return (uint)((num - get_whole(num)) * 10); }

void Display::Update(const float setpoint_C, const float ambient_C,
                     const float vent_C, const char *mode) {
  char buff[32] = {0};
  display_.setTextSize(1, 2);
  display_.clearDisplay();

  // First column:
  display_.setCursor(0, 0);
  float val = c_to_f(setpoint_C);
  sprintf(buff, "Set:%d.%dF", get_whole(val), get_mantissa(val));
  display_.write(buff);
  display_.setCursor(0, 18);
  val = c_to_f(ambient_C);
  sprintf(buff, "Amb:%d.%dF", get_whole(val), get_mantissa(val));
  display_.write(buff);

  // Second column:
  display_.setCursor(64, 0);
  val = c_to_f(vent_C);
  sprintf(buff, "Vent:%d.%dF", get_whole(val), get_mantissa(val));
  display_.write(buff);
  display_.setCursor(64, 18);
  sprintf(buff, "Mode:%s", mode);
  display_.write(buff);

  display_.display();
}