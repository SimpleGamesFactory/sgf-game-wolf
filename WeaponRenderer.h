#pragma once

#include <stdint.h>

namespace WeaponRenderer {

void render(
  uint16_t* frameBuffer,
  int width,
  int height,
  int bobOffsetY,
  bool muzzleFlash
);

}  // namespace WeaponRenderer
