#include "Keys.h"

#include "SGF/Color565.h"

namespace Keys {
namespace {

uint16_t darkVariant(uint16_t color565) {
  return Color565::darken(color565);
}

uint16_t lightVariant(uint16_t color565) {
  return Color565::lighten(color565);
}

}  // namespace

bool isPickup(uint8_t tile) {
  return tile == 'r' || tile == 'g' || tile == 'b';
}

KeyColor colorForPickup(uint8_t tile) {
  switch (tile) {
    case 'r': return KeyColor::Red;
    case 'g': return KeyColor::Green;
    case 'b': return KeyColor::Blue;
    default: return KeyColor::None;
  }
}

uint8_t bitFor(KeyColor color) {
  switch (color) {
    case KeyColor::Red: return RED_BIT;
    case KeyColor::Green: return GREEN_BIT;
    case KeyColor::Blue: return BLUE_BIT;
    default: return 0;
  }
}

uint16_t color565(KeyColor color) {
  switch (color) {
    case KeyColor::Red: return Color565::rgb(214, 72, 66);
    case KeyColor::Green: return Color565::rgb(70, 190, 96);
    case KeyColor::Blue: return Color565::rgb(72, 126, 224);
    default: return Color565::rgb(150, 150, 150);
  }
}

uint16_t minimapColor(uint8_t tile) {
  return color565(colorForPickup(tile));
}

uint16_t texel(uint8_t tile, int texX, int texY) {
  KeyColor color = colorForPickup(tile);
  if (color == KeyColor::None) {
    return 0;
  }

  uint16_t base = color565(color);
  uint16_t dark = darkVariant(base);
  uint16_t light = lightVariant(base);
  uint16_t metal = Color565::rgb(216, 206, 170);
  uint16_t metalDark = Color565::rgb(128, 118, 92);

  bool ring =
    ((texX - 4) * (texX - 4) + (texY - 5) * (texY - 5) <= 12) &&
    ((texX - 4) * (texX - 4) + (texY - 5) * (texY - 5) >= 4);
  bool shaft = texX >= 5 && texX <= 7 && texY >= 5 && texY <= 12;
  bool toothA = texX >= 7 && texX <= 10 && texY >= 10 && texY <= 11;
  bool toothB = texX >= 7 && texX <= 9 && texY >= 7 && texY <= 8;
  bool gem = texX >= 2 && texX <= 6 && texY >= 3 && texY <= 7;
  bool gemHighlight = texX >= 3 && texX <= 4 && texY >= 4 && texY <= 5;

  if (!(ring || shaft || toothA || toothB || gem)) {
    return 0;
  }
  if (gemHighlight) {
    return light;
  }
  if (gem) {
    return base;
  }
  if ((texX == 5 || texX == 7) && texY >= 5 && texY <= 12) {
    return metalDark;
  }
  if (toothA || toothB) {
    return metalDark;
  }
  return metal;
}

}  // namespace Keys
