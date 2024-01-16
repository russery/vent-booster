/*
Includes pin definitions and other hardware-specific things.

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

#ifndef __BSP_H
#define __BSP_H

#include <Arduino.h>

class BSP {
public:
  static constexpr char I2C_SDA_PIN = 21;
  static constexpr char I2C_SCL_PIN = 22;

  static constexpr char PWM_PIN = 10;
  static constexpr char AMBIENT_TEMP_PIN = A0;
  static constexpr char VENT_TEMP_PIN = A1;
  static constexpr char SETPOINT_PIN = A2;
};

#endif // __BSP_H