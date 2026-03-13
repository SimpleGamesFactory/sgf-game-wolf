#include "Minimap.h"

#include <stdlib.h>

#include "Door.h"
#include "Keys.h"
#include "SGF/Color565.h"

namespace {

static constexpr int MAX_CELL = 5;
static constexpr int MIN_CELL = 2;
static constexpr int BASE_MARGIN = 6;

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

uint16_t tileColor(uint8_t tile, uint8_t openAmount) {
  if (Door::isTile(tile)) {
    return Door::minimapColor(tile, openAmount);
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
  const uint8_t* doorOpenAmounts,
  int mapStride,
  int mapW,
  int mapH,
  float playerX,
  float playerY,
  float dirX,
  float dirY
) {
  int margin = BASE_MARGIN;
  int cellW = (width - (margin * 2)) / (mapW > 0 ? mapW : 1);
  int cellH = (height - (margin * 2)) / (mapH > 0 ? mapH : 1);
  int cell = cellW < cellH ? cellW : cellH;
  if (cell > MAX_CELL) {
    cell = MAX_CELL;
  }
  if (cell < MIN_CELL) {
    cell = MIN_CELL;
    margin = 2;
  }

  int mapPixelW = mapW * cell;
  int mapPixelH = mapH * cell;
  int boxX = margin - 2;
  int boxY = margin - 2;

  if (boxX + mapPixelW + 4 > width) {
    boxX = width - (mapPixelW + 4);
  }
  if (boxY + mapPixelH + 4 > height) {
    boxY = height - (mapPixelH + 4);
  }
  if (boxX < 0) {
    boxX = 0;
  }
  if (boxY < 0) {
    boxY = 0;
  }

  int mapX0 = boxX + 2;
  int mapY0 = boxY + 2;
  drawRect(
    buffer,
    width,
    height,
    boxX,
    boxY,
    mapPixelW + 4,
    mapPixelH + 4,
    Color565::rgb(12, 16, 20));

  for (int y = 0; y < mapH; y++) {
    for (int x = 0; x < mapW; x++) {
      uint8_t tile = map[y * mapStride + x];
      uint8_t openAmount = 0;
      if (doorOpenAmounts != nullptr) {
        openAmount = doorOpenAmounts[y * mapStride + x];
      }
      bool isKey = Keys::isPickup(tile);
      uint16_t color565 =
        (tile && !isKey) ? tileColor(tile, openAmount) : Color565::rgb(18, 22, 26);
      drawRect(
        buffer,
        width,
        height,
        mapX0 + x * cell,
        mapY0 + y * cell,
        cell - 1,
        cell - 1,
        color565);
      if (isKey) {
        drawRect(
          buffer,
          width,
          height,
          mapX0 + x * cell + 1,
          mapY0 + y * cell + 1,
          cell - 3,
          cell - 3,
          Keys::minimapColor(tile));
      }
    }
  }

  int px = mapX0 + static_cast<int>(playerX * static_cast<float>(cell));
  int py = mapY0 + static_cast<int>(playerY * static_cast<float>(cell));
  drawRect(buffer, width, height, px - 1, py - 1, 3, 3, Color565::rgb(255, 255, 255));

  int lookX = px + static_cast<int>(dirX * static_cast<float>(cell * 2));
  int lookY = py + static_cast<int>(dirY * static_cast<float>(cell * 2));
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
