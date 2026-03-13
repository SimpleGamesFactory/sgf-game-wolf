#include "WeaponRenderer.h"

#include "SGF/Color565.h"
#include "Textures.h"

namespace WeaponRenderer {
namespace {

constexpr const char* WEAPON_TEXTURE_NAME = "weapon_pistol";
constexpr int TEX_SIZE = Textures::TEX_SIZE;
constexpr int SCALE = 2;

void putPixel(uint16_t* frameBuffer, int width, int height, int x, int y, uint16_t color565) {
  if (x < 0 || x >= width || y < 0 || y >= height) {
    return;
  }
  frameBuffer[y * width + x] = color565;
}

void fillRect(uint16_t* frameBuffer, int width, int height, int x0, int y0, int w, int h, uint16_t color565) {
  if (w <= 0 || h <= 0) {
    return;
  }
  for (int y = y0; y < y0 + h; y++) {
    for (int x = x0; x < x0 + w; x++) {
      putPixel(frameBuffer, width, height, x, y, color565);
    }
  }
}

void renderFallback(uint16_t* frameBuffer, int width, int height, int bobOffsetY, bool muzzleFlash) {
  const uint16_t metal = Color565::rgb(124, 130, 142);
  const uint16_t darkMetal = Color565::rgb(76, 82, 92);
  const uint16_t grip = Color565::rgb(78, 54, 38);
  const uint16_t muzzle = Color565::rgb(240, 188, 84);
  const uint16_t muzzleHot = Color565::rgb(255, 246, 210);
  const int gunW = 28;
  const int gunH = 18;
  int baseX = (width - gunW) / 2;
  int baseY = height - gunH - 2 + bobOffsetY;

  fillRect(frameBuffer, width, height, baseX + 10, baseY + 2, 12, 5, metal);
  fillRect(frameBuffer, width, height, baseX + 12, baseY + 1, 8, 2, darkMetal);
  fillRect(frameBuffer, width, height, baseX + 16, baseY + 7, 4, 7, darkMetal);
  fillRect(frameBuffer, width, height, baseX + 9, baseY + 8, 7, 9, grip);
  fillRect(frameBuffer, width, height, baseX + 8, baseY + 13, 5, 4, darkMetal);
  fillRect(frameBuffer, width, height, baseX + 20, baseY + 7, 2, 2, metal);

  if (muzzleFlash) {
    fillRect(frameBuffer, width, height, baseX + 22, baseY + 5, 3, 4, muzzle);
    putPixel(frameBuffer, width, height, baseX + 25, baseY + 6, muzzleHot);
    putPixel(frameBuffer, width, height, baseX + 25, baseY + 7, muzzle);
    putPixel(frameBuffer, width, height, baseX + 24, baseY + 4, muzzleHot);
    putPixel(frameBuffer, width, height, baseX + 24, baseY + 9, muzzle);
  }
}

}  // namespace

void render(
  uint16_t* frameBuffer,
  int width,
  int height,
  int bobOffsetY,
  bool muzzleFlash
) {
  const uint16_t* texture = Textures::pixels(WEAPON_TEXTURE_NAME);
  if (texture == nullptr) {
    renderFallback(frameBuffer, width, height, bobOffsetY, muzzleFlash);
    return;
  }

  int weaponW = TEX_SIZE * SCALE;
  int weaponH = TEX_SIZE * SCALE;
  int baseX = (width - weaponW) / 2;
  int baseY = height - weaponH + bobOffsetY;

  for (int texY = 0; texY < TEX_SIZE; texY++) {
    for (int texX = 0; texX < TEX_SIZE; texX++) {
      uint16_t color565 = texture[texY * TEX_SIZE + texX];
      if (color565 == 0) {
        continue;
      }
      fillRect(
        frameBuffer,
        width,
        height,
        baseX + texX * SCALE,
        baseY + texY * SCALE,
        SCALE,
        SCALE,
        color565);
    }
  }

  if (muzzleFlash) {
    const uint16_t muzzle = Color565::rgb(240, 188, 84);
    const uint16_t muzzleHot = Color565::rgb(255, 246, 210);
    int flashX = baseX + weaponW - 6;
    int flashY = baseY + 8;
    fillRect(frameBuffer, width, height, flashX, flashY, 4, 5, muzzle);
    putPixel(frameBuffer, width, height, flashX + 4, flashY + 1, muzzleHot);
    putPixel(frameBuffer, width, height, flashX + 4, flashY + 2, muzzle);
    putPixel(frameBuffer, width, height, flashX + 3, flashY - 1, muzzleHot);
    putPixel(frameBuffer, width, height, flashX + 3, flashY + 5, muzzle);
  }
}

}  // namespace WeaponRenderer
