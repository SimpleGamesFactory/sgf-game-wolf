#include "Walls.h"

#include <stdlib.h>

#include "Door.h"
#include "SGF/Color565.h"

namespace Walls {

namespace {

constexpr Definition WALLS[] = {
  {1, "wall_red"},
  {2, "wall_green"},
  {3, "wall_skull"},
  {4, "wall_gold"},
  {5, "wall_purple"},
  {6, "wall_cyan"},
  {7, "wall_brown"},
  {8, "wall_pink"},
  {9, "wall_white"},
};

uint16_t fallbackColor(uint8_t tile) {
  switch (tile) {
    case 1: return Color565::rgb(220, 78, 52);
    case 2: return Color565::rgb(72, 208, 92);
    case 3: return Color565::rgb(126, 126, 138);
    case 4: return Color565::rgb(236, 192, 52);
    case 5: return Color565::rgb(204, 72, 216);
    case 6: return Color565::rgb(48, 212, 224);
    case 7: return Color565::rgb(216, 116, 44);
    case 8: return Color565::rgb(248, 52, 136);
    case 9: return Color565::rgb(248, 248, 248);
    default: return Color565::rgb(255, 255, 255);
  }
}

uint16_t fallbackTexel(uint8_t tile, int texX, int texY) {
  uint16_t base = fallbackColor(tile);
  uint16_t dark = Color565::darken(base);
  uint16_t light = Color565::lighten(base);

  bool edge = (texX == 0 || texY == 0 || texX == TEX_SIZE - 1 || texY == TEX_SIZE - 1);
  bool mortar = ((texX % 8) == 0) || ((texY % 8) == 0);
  bool stripe = ((texX / 3) % 2) == 0;
  bool checker = (((texX / 4) + (texY / 4)) % 2) == 0;
  bool diamond = abs(texX - 8) + abs(texY - 8) < 5;
  bool slit = (texX > 5 && texX < 10 && texY > 2 && texY < 13);

  switch (tile) {
    case 1:
      return (edge || mortar) ? dark : base;
    case 2:
      if (edge) {
        return dark;
      }
      return stripe ? light : base;
    case 3:
      return edge ? dark : base;
    case 4:
      if (edge || ((texY % 5) == 0)) {
        return dark;
      }
      return ((texX % 5) == 0) ? light : base;
    case 5:
      return edge ? dark : (diamond ? light : base);
    case 6:
      if (edge || slit) {
        return dark;
      }
      return (texY < 3 || texY > 12) ? light : base;
    case 7:
      if (edge) {
        return dark;
      }
      return ((texX + texY) % 6) < 3 ? base : light;
    case 8:
      if (edge) {
        return dark;
      }
      return ((texX - texY + TEX_SIZE) % 6) < 3 ? light : base;
    case 9:
      return (edge || checker) ? dark : light;
    default:
      return base;
  }
}

}

const Definition* definition(uint8_t tile) {
  for (unsigned i = 0; i < (sizeof(WALLS) / sizeof(WALLS[0])); i++) {
    if (WALLS[i].tile == tile) {
      return &WALLS[i];
    }
  }
  return 0;
}

const char* textureName(uint8_t tile) {
  const Definition* def = definition(tile);
  return def ? def->textureName : 0;
}

uint16_t texel(uint8_t tile, int texX, int texY) {
  if (Door::isTile(tile)) {
    return Door::texel(tile, texX, texY);
  }

  uint16_t fallback565 = fallbackTexel(tile, texX, texY);
  return Textures::texel(textureName(tile), texX, texY, fallback565);
}

}
