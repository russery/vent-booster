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

#include <Adafruit_SSD1306.h>
#include <Wire.h>

class Display {
private:
  TwoWire *wire_;
  // Display dimensions:
  static constexpr uint8_t SCREEN_WIDTH PROGMEM = 128;
  static constexpr uint8_t SCREEN_HEIGHT PROGMEM = 32;

  static constexpr int8_t OLED_RESET PROGMEM =
      -1; // Reset pin (-1 means no reset)
  static constexpr uint8_t SCREEN_ADDRESS =
      0x3C; // 0x3D for 128x64, 0x3C for 128x32
  Adafruit_SSD1306 display_;

public:
  explicit Display(TwoWire *wire) : wire_(wire) {
    display_ = Adafruit_SSD1306(SCREEN_WIDTH, SCREEN_HEIGHT, wire_, OLED_RESET);
  };
  void Start(void);
  void Update(const float setpoint_C, const float ambient_C, const float vent_C,
              const char *mode);
};