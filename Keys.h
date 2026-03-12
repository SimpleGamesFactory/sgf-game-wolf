#pragma once

#include <stdint.h>

enum class KeyColor : uint8_t {
  None = 0,
  Red,
  Green,
  Blue
};

namespace Keys {

constexpr uint8_t RED_BIT = 1u << 0;
constexpr uint8_t GREEN_BIT = 1u << 1;
constexpr uint8_t BLUE_BIT = 1u << 2;
constexpr int TEX_SIZE = 16;

bool isPickup(uint8_t tile);
KeyColor colorForPickup(uint8_t tile);
uint8_t bitFor(KeyColor color);
uint16_t color565(KeyColor color);
uint16_t minimapColor(uint8_t tile);
uint16_t texel(uint8_t tile, int texX, int texY);
const uint16_t* texture(uint8_t tile);

}  // namespace Keys
