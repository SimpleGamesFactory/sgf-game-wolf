#include "Minimap.h"

#include <stdlib.h>

#include "Door.h"
#include "SGF/Color565.h"

namespace {

static constexpr int CELL = 5;
static constexpr int MARGIN = 6;

void putPixel(uint16_t* buffer, int width, int height, int x, int y, uint16_t color565) {
  if (x < 0 || x >= width || y < 0 || y >= height) {
    return;
  }
  buffer[y * width + x] = color565;
}

void drawRect(
  uint16_t* buffer,
  int width,
  int height,
  int x0,
  int y0,
  int w,
  int h,
  uint16_t color565
) {
  if (w <= 0 || h <= 0) {
    return;
  }
  for (int y = y0; y < y0 + h; y++) {
    if (y < 0 || y >= height) {
      continue;
    }
    for (int x = x0; x < x0 + w; x++) {
      if (x < 0 || x >= width) {
        continue;
      }
      buffer[y * width + x] = color565;
    }
  }
}

uint16_t tileColor(uint8_t tile, bool isOpen) {
  if (Door::isTile(tile)) {
    return Door::minimapColor(isOpen);
  }
  switch (tile) {
    case 1: return Color565::rgb(140, 66, 58);
    case 2: return Color565::rgb(70, 138, 82);
    case 3: return Color565::rgb(66, 96, 164);
    case 4: return Color565::rgb(164, 136, 68);
    case 5: return Color565::rgb(130, 76, 146);
    case 6: return Color565::rgb(58, 148, 164);
    case 7: return Color565::rgb(156, 98, 66);
    case 8: return Color565::rgb(172, 70, 112);
    case 9: return Color565::rgb(196, 196, 196);
    default: return Color565::rgb(255, 255, 255);
  }
}

}  // namespace

void Minimap::render(
  uint16_t* buffer,
  int width,
  int height,
  const uint8_t* map,
  const bool* doorOpen,
  int mapW,
  int mapH,
  float playerX,
  float playerY,
  float dirX,
  float dirY
) {
  int mapPixelW = mapW * CELL;
  int mapPixelH = mapH * CELL;
  drawRect(
    buffer,
    width,
    height,
    MARGIN - 2,
    MARGIN - 2,
    mapPixelW + 4,
    mapPixelH + 4,
    Color565::rgb(12, 16, 20));

  for (int y = 0; y < mapH; y++) {
    for (int x = 0; x < mapW; x++) {
      uint8_t tile = map[y * mapW + x];
      bool isOpen = doorOpen && doorOpen[y * mapW + x];
      uint16_t color565 = tile ? tileColor(tile, isOpen) : Color565::rgb(18, 22, 26);
      drawRect(
        buffer,
        width,
        height,
        MARGIN + x * CELL,
        MARGIN + y * CELL,
        CELL - 1,
        CELL - 1,
        color565);
    }
  }

  int px = MARGIN + static_cast<int>(playerX * static_cast<float>(CELL));
  int py = MARGIN + static_cast<int>(playerY * static_cast<float>(CELL));
  drawRect(buffer, width, height, px - 1, py - 1, 3, 3, Color565::rgb(255, 255, 255));

  int lookX = px + static_cast<int>(dirX * static_cast<float>(CELL * 2));
  int lookY = py + static_cast<int>(dirY * static_cast<float>(CELL * 2));
  int dx = abs(lookX - px);
  int sx = (px < lookX) ? 1 : -1;
  int dy = -abs(lookY - py);
  int sy = (py < lookY) ? 1 : -1;
  int err = dx + dy;
  while (true) {
    putPixel(buffer, width, height, px, py, Color565::rgb(255, 220, 120));
    if (px == lookX && py == lookY) {
      break;
    }
    int err2 = err * 2;
    if (err2 >= dy) {
      err += dy;
      px += sx;
    }
    if (err2 <= dx) {
      err += dx;
      py += sy;
    }
  }
}
