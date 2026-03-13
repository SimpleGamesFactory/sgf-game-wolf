#include "Keys.h"

#include <string.h>

#include "SGF/Color565.h"

namespace Keys {
namespace {

constexpr int KEY_COLOR_COUNT = 3;

uint16_t lightVariant(uint16_t color565) {
  return Color565::lighten(color565);
}

int colorIndex(KeyColor color) {
  switch (color) {
    case KeyColor::Red: return 0;
    case KeyColor::Green: return 1;
    case KeyColor::Blue: return 2;
    default: return -1;
  }
}

uint16_t buildTexel(KeyColor color, int texX, int texY) {
  if (color == KeyColor::None) {
    return 0;
  }

  uint16_t base = color565(color);
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

const uint16_t* textureForColor(KeyColor color) {
  static bool initialized = false;
  static uint16_t textures[KEY_COLOR_COUNT][TEX_SIZE * TEX_SIZE];
  if (!initialized) {
    for (int colorIdx = 0; colorIdx < KEY_COLOR_COUNT; colorIdx++) {
      KeyColor buildColor = KeyColor::Red;
      if (colorIdx == 1) {
        buildColor = KeyColor::Green;
      } else if (colorIdx == 2) {
        buildColor = KeyColor::Blue;
      }
      for (int texY = 0; texY < TEX_SIZE; texY++) {
        for (int texX = 0; texX < TEX_SIZE; texX++) {
          textures[colorIdx][texY * TEX_SIZE + texX] = buildTexel(buildColor, texX, texY);
        }
      }
    }
    initialized = true;
  }

  int idx = colorIndex(color);
  if (idx < 0) {
    return nullptr;
  }
  return textures[idx];
}

void buildSpriteTexture(const void* owner, uint16_t* outTexture, uint32_t nowMs) {
  (void)nowMs;
  const uint8_t tile = static_cast<uint8_t>(reinterpret_cast<uintptr_t>(owner));
  const uint16_t* tex = texture(tile);
  if (tex == nullptr) {
    memset(outTexture, 0, TEX_SIZE * TEX_SIZE * sizeof(uint16_t));
    return;
  }
  memcpy(outTexture, tex, TEX_SIZE * TEX_SIZE * sizeof(uint16_t));
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
  const uint16_t* tex = texture(tile);
  if (tex == nullptr) {
    return 0;
  }
  return tex[texY * TEX_SIZE + texX];
}

const uint16_t* texture(uint8_t tile) {
  return textureForColor(colorForPickup(tile));
}

void initSprite(WolfRender::Sprite& sprite, uint8_t tile, float x, float y) {
  sprite.configure(
    x,
    y,
    TEX_SIZE,
    1,
    1,
    3,
    8.0f,
    reinterpret_cast<const void*>(static_cast<uintptr_t>(tile)),
    buildSpriteTexture);
}

}  // namespace Keys
