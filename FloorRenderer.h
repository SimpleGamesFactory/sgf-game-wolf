#pragma once

#include <stdint.h>

namespace FloorRenderer {

void render(
  uint16_t* frameBuffer,
  int width,
  int height,
  float playerX,
  float playerY,
  float dirX,
  float dirY,
  float planeX,
  float planeY);

}  // namespace FloorRenderer
