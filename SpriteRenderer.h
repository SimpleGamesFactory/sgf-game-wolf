#pragma once

#include <stdint.h>

#include "Sprite.h"

namespace SpriteRenderer {

constexpr int MAX_SPRITES = 64;
constexpr int MAX_TEXELS = 16 * 16;

struct RenderView {
  uint16_t* frameBuffer = nullptr;
  int width = 0;
  int height = 0;
  const float* wallDepth = nullptr;
  float playerX = 0.0f;
  float playerY = 0.0f;
  float dirX = 0.0f;
  float dirY = 0.0f;
  float planeX = 0.0f;
  float planeY = 0.0f;
  int cameraYOffset = 0;
  uint32_t nowMs = 0;
};

void render(const RenderView& view, const WolfRender::ISprite* const* sprites, int spriteCount);

}  // namespace SpriteRenderer
