#pragma once

#include <stdint.h>

class Door {
public:
  static bool isTile(uint8_t tile);
  static bool isPassable(bool open);
  static uint16_t minimapColor(bool open);
  static uint16_t texel(int texX, int texY);
};
