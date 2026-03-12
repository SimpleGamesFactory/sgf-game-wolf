#pragma once

#include <stdint.h>

class Minimap {
public:
  static void render(
    uint16_t* buffer,
    int width,
    int height,
    const uint8_t* map,
    const uint8_t* doorOpenBits,
    int mapStride,
    int mapW,
    int mapH,
    float playerX,
    float playerY,
    float dirX,
    float dirY);
};
