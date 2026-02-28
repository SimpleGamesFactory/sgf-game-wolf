#include "FloorRenderer.h"

#include <math.h>

#include "SGF/Color565.h"

namespace FloorRenderer {
namespace {

constexpr int TEX_SIZE = 16;

uint16_t shadeColor(uint16_t color565, float distance) {
  float brightness = 1.0f / (1.0f + distance * 0.16f);
  if (brightness < 0.20f) {
    brightness = 0.20f;
  }

  uint8_t r5 = (color565 >> 11) & 0x1F;
  uint8_t g6 = (color565 >> 5) & 0x3F;
  uint8_t b5 = color565 & 0x1F;
  uint8_t r = static_cast<uint8_t>((static_cast<float>(r5) * 255.0f / 31.0f) * brightness);
  uint8_t g = static_cast<uint8_t>((static_cast<float>(g6) * 255.0f / 63.0f) * brightness);
  uint8_t b = static_cast<uint8_t>((static_cast<float>(b5) * 255.0f / 31.0f) * brightness);
  return Color565::rgb(r, g, b);
}

uint16_t floorTexel(int texX, int texY, int cellX, int cellY) {
  bool warmTile = ((cellX + cellY) & 1) == 0;
  uint16_t base = warmTile ? Color565::rgb(92, 84, 70) : Color565::rgb(78, 72, 60);
  uint16_t light = Color565::lighten(base);
  uint16_t dark = Color565::darken(base);
  uint16_t seam = Color565::rgb(42, 36, 32);

  bool edge = texX == 0 || texY == 0 || texX == TEX_SIZE - 1 || texY == TEX_SIZE - 1;
  bool grout = (texX == 7) || (texY == 7);
  bool speckle = ((texX * 3 + texY * 5 + cellX * 7 + cellY * 11) & 0x3) == 0;
  bool crack =
    ((texX == texY) && texX > 2 && texX < 7) ||
    ((texX + texY) == 11 && texX > 8 && texY > 2) ||
    (texX > 10 && texX < 13 && texY == 4);

  if (edge || grout) {
    return seam;
  }
  if (crack) {
    return dark;
  }
  if (speckle) {
    return light;
  }
  if (((texX + texY) & 1) == 0) {
    return base;
  }
  return warmTile ? dark : light;
}

}  // namespace

void render(
  uint16_t* frameBuffer,
  int width,
  int height,
  float playerX,
  float playerY,
  float dirX,
  float dirY,
  float planeX,
  float planeY
) {
  if (frameBuffer == 0 || width <= 0 || height <= 0) {
    return;
  }

  const int halfH = height / 2;
  const float rayDirX0 = dirX - planeX;
  const float rayDirY0 = dirY - planeY;
  const float rayDirX1 = dirX + planeX;
  const float rayDirY1 = dirY + planeY;
  const float cameraZ = static_cast<float>(height) * 0.5f;

  for (int y = halfH + 1; y < height; y++) {
    int rowOffset = y - halfH;
    if (rowOffset <= 0) {
      continue;
    }

    float rowDistance = cameraZ / static_cast<float>(rowOffset);
    float floorStepX = rowDistance * (rayDirX1 - rayDirX0) / static_cast<float>(width);
    float floorStepY = rowDistance * (rayDirY1 - rayDirY0) / static_cast<float>(width);
    float floorX = playerX + rowDistance * rayDirX0;
    float floorY = playerY + rowDistance * rayDirY0;

    uint16_t* row = &frameBuffer[y * width];
    for (int x = 0; x < width; x++) {
      int cellX = static_cast<int>(floorf(floorX));
      int cellY = static_cast<int>(floorf(floorY));
      float fracX = floorX - static_cast<float>(cellX);
      float fracY = floorY - static_cast<float>(cellY);
      int texX = static_cast<int>(fracX * static_cast<float>(TEX_SIZE)) & (TEX_SIZE - 1);
      int texY = static_cast<int>(fracY * static_cast<float>(TEX_SIZE)) & (TEX_SIZE - 1);

      row[x] = shadeColor(floorTexel(texX, texY, cellX, cellY), rowDistance);
      floorX += floorStepX;
      floorY += floorStepY;
    }
  }
}

}  // namespace FloorRenderer
