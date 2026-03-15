#include "Pickups.h"

#include <stdlib.h>
#include <string.h>

#include "Keys.h"
#include "SGF/Color565.h"
#include "Textures.h"

namespace Pickups {
namespace {

constexpr int EXTRA_PICKUP_COUNT = 2;
constexpr uint8_t PICKUP_OWNER_RED = 'r';
constexpr uint8_t PICKUP_OWNER_GREEN = 'g';
constexpr uint8_t PICKUP_OWNER_BLUE = 'b';
constexpr uint8_t PICKUP_OWNER_AMMO = 'a';
constexpr uint8_t PICKUP_OWNER_MEDKIT = 'h';

const char* textureName(uint8_t tile) {
  switch (tile) {
    case 'r': return "sprite_key_red";
    case 'g': return "sprite_key_green";
    case 'b': return "sprite_key_blue";
    case 'a': return "sprite_ammo";
    case 'h': return "sprite_medkit";
    default: return nullptr;
  }
}

const uint8_t* ownerForTile(uint8_t tile) {
  switch (tile) {
    case 'r': return &PICKUP_OWNER_RED;
    case 'g': return &PICKUP_OWNER_GREEN;
    case 'b': return &PICKUP_OWNER_BLUE;
    case 'a': return &PICKUP_OWNER_AMMO;
    case 'h': return &PICKUP_OWNER_MEDKIT;
    default: return nullptr;
  }
}

int extraPickupIndex(uint8_t tile) {
  switch (tile) {
    case 'a': return 0;
    case 'h': return 1;
    default: return -1;
  }
}

uint16_t buildAmmoTexel(int texX, int texY) {
  const uint16_t brass = Color565::rgb(202, 164, 76);
  const uint16_t brassLight = Color565::lighten(brass);
  const uint16_t brassDark = Color565::darken(brass);
  const uint16_t crate = Color565::rgb(96, 70, 44);
  const uint16_t crateDark = Color565::darken(crate);
  const uint16_t band = Color565::rgb(128, 136, 142);

  bool box = texX >= 2 && texX <= 13 && texY >= 4 && texY <= 12;
  bool edge = texX == 2 || texX == 13 || texY == 4 || texY == 12;
  bool rounds = texY >= 6 && texY <= 10 && (texX == 4 || texX == 7 || texX == 10);
  bool roundsTip = texY == 6 && (texX == 4 || texX == 7 || texX == 10);
  bool banding = texY == 8 && texX >= 3 && texX <= 12;

  if (!box) {
    return 0;
  }
  if (edge) {
    return crateDark;
  }
  if (banding) {
    return band;
  }
  if (roundsTip) {
    return brassLight;
  }
  if (rounds) {
    return brass;
  }
  return crate;
}

uint16_t buildMedkitTexel(int texX, int texY) {
  const uint16_t white = Color565::rgb(232, 232, 232);
  const uint16_t whiteDark = Color565::darken(white);
  const uint16_t red = Color565::rgb(212, 52, 52);
  const uint16_t redDark = Color565::darken(red);

  bool bag = texX >= 2 && texX <= 13 && texY >= 3 && texY <= 13;
  bool edge = texX == 2 || texX == 13 || texY == 3 || texY == 13;
  bool handle = texY >= 1 && texY <= 3 && texX >= 5 && texX <= 10;
  bool handleHole = texY == 2 && texX >= 6 && texX <= 9;
  bool crossH = texY >= 7 && texY <= 9 && texX >= 4 && texX <= 11;
  bool crossV = texX >= 6 && texX <= 8 && texY >= 5 && texY <= 11;

  if (handle && !handleHole) {
    return whiteDark;
  }
  if (!bag) {
    return 0;
  }
  if (edge) {
    return whiteDark;
  }
  if (crossH || crossV) {
    return (texX == 6 || texX == 8 || texY == 7 || texY == 9) ? redDark : red;
  }
  return white;
}

const uint16_t* fallbackTexture(uint8_t tile) {
  static bool initialized = false;
  static uint16_t* textures[EXTRA_PICKUP_COUNT]{};
  if (!initialized) {
    for (int i = 0; i < EXTRA_PICKUP_COUNT; i++) {
      textures[i] = new uint16_t[TEX_SIZE * TEX_SIZE];
      if (textures[i] == nullptr) {
        continue;
      }
      for (int texY = 0; texY < TEX_SIZE; texY++) {
        for (int texX = 0; texX < TEX_SIZE; texX++) {
          uint16_t color565 = (i == 0) ? buildAmmoTexel(texX, texY) : buildMedkitTexel(texX, texY);
          textures[i][texY * TEX_SIZE + texX] = color565;
        }
      }
    }
    initialized = true;
  }

  int index = extraPickupIndex(tile);
  if (index < 0) {
    return nullptr;
  }
  return textures[index];
}

void buildSpriteTexture(const void* owner, uint16_t* outTexture, uint32_t nowMs) {
  (void)nowMs;
  const uint8_t* tileRef = owner == &PICKUP_OWNER_RED ? &PICKUP_OWNER_RED
                         : owner == &PICKUP_OWNER_GREEN ? &PICKUP_OWNER_GREEN
                         : owner == &PICKUP_OWNER_BLUE ? &PICKUP_OWNER_BLUE
                         : owner == &PICKUP_OWNER_AMMO ? &PICKUP_OWNER_AMMO
                         : owner == &PICKUP_OWNER_MEDKIT ? &PICKUP_OWNER_MEDKIT
                         : nullptr;
  uint8_t tile = tileRef == nullptr ? 0 : *tileRef;
  const uint16_t* tex = texture(tile);
  if (tex == nullptr) {
    memset(outTexture, 0, TEX_SIZE * TEX_SIZE * sizeof(uint16_t));
    return;
  }
  memcpy(outTexture, tex, TEX_SIZE * TEX_SIZE * sizeof(uint16_t));
}

}  // namespace

Type typeForTile(uint8_t tile) {
  switch (tile) {
    case 'r': return Type::KeyRed;
    case 'g': return Type::KeyGreen;
    case 'b': return Type::KeyBlue;
    case 'a': return Type::Ammo;
    case 'h': return Type::Medkit;
    default: return Type::None;
  }
}

bool isPickup(uint8_t tile) {
  return typeForTile(tile) != Type::None;
}

bool isKey(uint8_t tile) {
  Type type = typeForTile(tile);
  return type == Type::KeyRed || type == Type::KeyGreen || type == Type::KeyBlue;
}

bool isAmmo(uint8_t tile) {
  return typeForTile(tile) == Type::Ammo;
}

bool isMedkit(uint8_t tile) {
  return typeForTile(tile) == Type::Medkit;
}

uint16_t minimapColor(uint8_t tile) {
  switch (typeForTile(tile)) {
    case Type::KeyRed:
    case Type::KeyGreen:
    case Type::KeyBlue:
      return Keys::color565(Keys::colorForPickup(tile));
    case Type::Ammo:
      return Color565::rgb(202, 164, 76);
    case Type::Medkit:
      return Color565::rgb(232, 232, 232);
    default:
      return Color565::rgb(255, 255, 255);
  }
}

const uint16_t* texture(uint8_t tile) {
  const uint16_t* bmpTexture = Textures::pixels(textureName(tile));
  if (bmpTexture != nullptr) {
    return bmpTexture;
  }
  if (isKey(tile)) {
    return Keys::texture(tile);
  }
  return fallbackTexture(tile);
}

void initSprite(WolfRender::Sprite& sprite, uint8_t tile, float x, float y) {
  const uint8_t* owner = ownerForTile(tile);
  sprite.configure(
    x,
    y,
    TEX_SIZE,
    1,
    1,
    3,
    8.0f,
    owner,
    buildSpriteTexture);
}

}  // namespace Pickups
