#pragma once

#include <stdint.h>

#include "Keys.h"

class Door {
public:
  static bool isTile(uint8_t tile);
  static KeyColor requiredKey(uint8_t tile);
  static bool isPassable(bool open);
  static uint16_t minimapColor(uint8_t tile, bool open);
  static uint16_t texel(uint8_t tile, int texX, int texY);
};
