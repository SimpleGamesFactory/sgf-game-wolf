#include "Door.h"

#include "SGF/Color565.h"

bool Door::isTile(uint8_t tile) {
  return tile == 'D';
}

bool Door::isPassable(bool open) {
  return open;
}

uint16_t Door::minimapColor(bool open) {
  if (open) {
    return Color565::rgb(120, 164, 116);
  }
  return Color565::rgb(142, 108, 56);
}

uint16_t Door::texel(int texX, int texY) {
  uint16_t base = Color565::rgb(152, 112, 62);
  uint16_t dark = Color565::darken(base);
  uint16_t light = Color565::lighten(base);
  uint16_t metal = Color565::rgb(188, 182, 164);

  bool edge = (texX == 0 || texY == 0 || texX == 15 || texY == 15);
  bool plank = ((texX % 4) == 0);
  bool brace = (texY == 4 || texY == 11);
  bool lockPlate = (texX >= 11 && texX <= 13 && texY >= 6 && texY <= 9);
  bool handle = (texX == 12 && texY == 8);

  if (edge) {
    return dark;
  }
  if (handle) {
    return light;
  }
  if (lockPlate) {
    return metal;
  }
  if (brace) {
    return dark;
  }
  if (plank) {
    return light;
  }
  return base;
}
