#include "Textures.h"

#include <string.h>

#include "TexturesGenerated.h"

namespace Textures {

namespace {

int clampCoord(int value) {
  if (value < 0) {
    return 0;
  }
  if (value >= TEX_SIZE) {
    return TEX_SIZE - 1;
  }
  return value;
}

}

const Asset* find(const char* name) {
  if (name == 0 || name[0] == '\0') {
    return 0;
  }

  for (int i = 0; i < TexturesGenerated::ASSET_COUNT; i++) {
    const Asset& asset = TexturesGenerated::ASSETS[i];
    if (asset.name != 0 && strcmp(asset.name, name) == 0) {
      return &asset;
    }
  }
  return 0;
}

const uint16_t* pixels(const char* name) {
  const Asset* asset = find(name);
  return asset ? asset->pixels : 0;
}

uint16_t texel(const char* name, int texX, int texY, uint16_t fallback565) {
  const uint16_t* tex = pixels(name);
  if (tex == 0) {
    return fallback565;
  }

  texX = clampCoord(texX);
  texY = clampCoord(texY);
  return tex[texY * TEX_SIZE + texX];
}

}
