#pragma once

#include <stdint.h>

#include "Sprite.h"

namespace Pickups {

enum class Type : uint8_t {
  None = 0,
  KeyRed,
  KeyGreen,
  KeyBlue,
  Ammo,
  Medkit
};

constexpr int TEX_SIZE = 16;
constexpr int MAX_AMMO = 99;
constexpr int AMMO_PICKUP_AMOUNT = 12;
constexpr int MEDKIT_PICKUP_AMOUNT = 25;

Type typeForTile(uint8_t tile);
bool isPickup(uint8_t tile);
bool isKey(uint8_t tile);
bool isAmmo(uint8_t tile);
bool isMedkit(uint8_t tile);
uint16_t minimapColor(uint8_t tile);
const uint16_t* texture(uint8_t tile);
void initSprite(WolfRender::Sprite& sprite, uint8_t tile, float x, float y);

}  // namespace Pickups
