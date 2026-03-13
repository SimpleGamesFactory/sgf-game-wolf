#pragma once

#include <stdint.h>

#include "Keys.h"

class Door {
public:
  static constexpr int TEX_SIZE = 16;
  static constexpr uint8_t OPEN_AMOUNT_CLOSED = 0;
  static constexpr uint8_t OPEN_AMOUNT_OPEN = 255;
  static constexpr uint8_t PASSABLE_THRESHOLD = 224;
  static constexpr float OPEN_SPEED_PER_SEC = 1.6f;

  static bool isTile(uint8_t tile);
  static KeyColor requiredKey(uint8_t tile);
  static bool isPassable(uint8_t openAmount);
  static uint16_t minimapColor(uint8_t tile, uint8_t openAmount);
  static const char* textureName(uint8_t tile);
  static int shiftedTexX(int texX, uint8_t openAmount);
  static uint16_t texel(uint8_t tile, int texX, int texY);
};
