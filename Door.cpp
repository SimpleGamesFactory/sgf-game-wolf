#include "Door.h"

#include "SGF/Color565.h"
#include "Textures.h"

bool Door::isTile(uint8_t tile) {
  return tile == 'D' || tile == '1' || tile == '2' || tile == '3';
}

KeyColor Door::requiredKey(uint8_t tile) {
  switch (tile) {
    case '1': return KeyColor::Red;
    case '2': return KeyColor::Green;
    case '3': return KeyColor::Blue;
    default: return KeyColor::None;
  }
}

bool Door::isPassable(uint8_t openAmount) {
  return openAmount >= PASSABLE_THRESHOLD;
}

uint16_t Door::minimapColor(uint8_t tile, uint8_t openAmount) {
  if (openAmount > 0) {
    KeyColor key = requiredKey(tile);
    if (key != KeyColor::None) {
      return Color565::lighten(Keys::color565(key));
    }
    return Color565::rgb(120, 164, 116);
  }
  KeyColor key = requiredKey(tile);
  if (key != KeyColor::None) {
    return Keys::color565(key);
  }
  return Color565::rgb(142, 108, 56);
}

const char* Door::textureName(uint8_t tile) {
  switch (tile) {
    case '1': return "door_red";
    case '2': return "door_green";
    case '3': return "door_blue";
    default: return "door_plain";
  }
}

int Door::shiftedTexX(int texX, uint8_t openAmount) {
  int shift = (openAmount * (TEX_SIZE + 1)) / 256;
  return texX - shift;
}

uint16_t Door::texel(uint8_t tile, int texX, int texY) {
  uint16_t fallback = 0;
  uint16_t base = Color565::rgb(152, 112, 62);
  uint16_t dark = Color565::darken(base);
  uint16_t light = Color565::lighten(base);
  KeyColor key = requiredKey(tile);
  uint16_t metal = (key == KeyColor::None) ? Color565::rgb(188, 182, 164) : Keys::color565(key);
  uint16_t metalLight = Color565::lighten(metal);

  bool edge = (texX == 0 || texY == 0 || texX == 15 || texY == 15);
  bool plank = ((texX % 4) == 0);
  bool brace = (texY == 4 || texY == 11);
  bool lockPlate = (texX >= 11 && texX <= 13 && texY >= 6 && texY <= 9);
  bool handle = (texX == 12 && texY == 8);

  if (edge) {
    fallback = dark;
  } else if (handle) {
    fallback = metalLight;
  } else if (lockPlate) {
    fallback = metal;
  } else if (brace) {
    fallback = dark;
  } else if (plank) {
    fallback = light;
  } else {
    fallback = base;
  }
  return Textures::texel(textureName(tile), texX, texY, fallback);
}
