#pragma once

#include <stdint.h>

namespace Textures {

constexpr int TEX_SIZE = 16;
constexpr int PALETTE_SIZE = 128;

struct Asset {
  const char* name;
  const uint16_t* pixels;
};

const Asset* find(const char* name);
const uint16_t* pixels(const char* name);
uint16_t texel(const char* name, int texX, int texY, uint16_t fallback565);

}
