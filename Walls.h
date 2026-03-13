#pragma once

#include <stdint.h>

#include "Textures.h"

namespace Walls {

constexpr int TEX_SIZE = Textures::TEX_SIZE;

struct Definition {
  uint8_t tile;
  const char* textureName;
};

const Definition* definition(uint8_t tile);
const char* textureName(uint8_t tile);
uint16_t texel(uint8_t tile, int texX, int texY);

}
