#include "Hud.h"

#include <stdlib.h>
#include <stdio.h>
#if defined(ARDUINO_ARCH_ESP32)
#include <esp_heap_caps.h>
#endif

#include "Keys.h"
#include "SGF/Color565.h"
#include "SGF/Font5x7.h"
#include "SGF/FontRenderer.h"
#include "SGF/IFillRect.h"

namespace {

class HudBufferFillRect : public IFillRect {
public:
  HudBufferFillRect(uint8_t* pixelsRef, int widthRef, int heightRef)
    : pixels(pixelsRef), width(widthRef), height(heightRef) {}

  void fillRect565(int x0, int y0, int w, int h, uint16_t color565) override {
    if (w <= 0 || h <= 0) {
      return;
    }
    uint8_t packed = packRgb332(color565);
    for (int yy = y0; yy < y0 + h; yy++) {
      if (yy < 0 || yy >= height) {
        continue;
      }
      uint8_t* row = &pixels[yy * width];
      for (int xx = x0; xx < x0 + w; xx++) {
        if (xx < 0 || xx >= width) {
          continue;
        }
        row[xx] = packed;
      }
    }
  }

private:
  static uint8_t packRgb332(uint16_t color565) {
    uint8_t r = static_cast<uint8_t>(((color565 >> 11) & 0x1Fu) * 255u / 31u);
    uint8_t g = static_cast<uint8_t>(((color565 >> 5) & 0x3Fu) * 255u / 63u);
    uint8_t b = static_cast<uint8_t>((color565 & 0x1Fu) * 255u / 31u);
    return static_cast<uint8_t>((r & 0xE0u) | ((g >> 3) & 0x1Cu) | (b >> 6));
  }

  uint8_t* pixels = nullptr;
  int width = 0;
  int height = 0;
};

uint16_t unpackRgb332(uint8_t packed) {
  uint8_t r = packed & 0xE0u;
  uint8_t g = (packed & 0x1Cu) << 3;
  uint8_t b = (packed & 0x03u) << 6;
  r |= static_cast<uint8_t>(r >> 3);
  g |= static_cast<uint8_t>(g >> 3);
  b |= static_cast<uint8_t>(b >> 2);
  b |= static_cast<uint8_t>(b >> 4);
  return Color565::rgb(r, g, b);
}

}  // namespace

void Hud::begin(int width, int worldHeight) {
  screenW = width;
  worldScreenH = worldHeight;
#if WOLF_HEAP_COLD_BUFFERS
  if (buffer == nullptr) {
    const size_t bufferBytes = static_cast<size_t>(screenW) * HUD_H;
#if defined(ARDUINO_ARCH_ESP32)
    buffer = static_cast<uint8_t*>(
      heap_caps_malloc(bufferBytes, MALLOC_CAP_INTERNAL | MALLOC_CAP_8BIT));
#else
    buffer = static_cast<uint8_t*>(malloc(bufferBytes));
#endif
    if (buffer == nullptr) {
      abort();
    }
  }
#endif
  invalidate();
}

void Hud::setLives(int value) {
  if (lives == value) {
    return;
  }
  lives = value;
  markDirtyRect(HUD_LIVES_X, HUD_LIVES_Y, HUD_LIVES_W, HUD_LIVES_H);
}

void Hud::setAmmo(int value) {
  if (ammo == value) {
    return;
  }
  ammo = value;
  markDirtyRect(HUD_STATS_X, HUD_AMMO_Y, HUD_STATS_W, HUD_AMMO_H);
}

void Hud::setEnergy(int value) {
  if (energy == value) {
    return;
  }
  energy = value;
  markDirtyRect(HUD_STATS_X, HUD_ENERGY_Y, HUD_STATS_W, HUD_ENERGY_H);
}

void Hud::setKeys(uint8_t value) {
  if (keysMask == value) {
    return;
  }
  keysMask = value;
  markDirtyRect(HUD_FACE_X + HUD_FACE_W - 34, HUD_FACE_Y + 4, 28, 10);
}

void Hud::setFaceMood(FaceMood value) {
  if (faceMood == value) {
    return;
  }
  faceMood = value;
  markDirtyRect(HUD_FACE_X, HUD_FACE_Y, HUD_FACE_W, HUD_FACE_H);
}

void Hud::invalidate() {
  dirty.clear();
  dirty.add(0, 0, screenW - 1, HUD_H - 1);
}

void Hud::markDirtyRect(int x, int y, int w, int h) {
  dirty.add(x, y, x + w - 1, y + h - 1);
  dirty.clip(screenW, HUD_H);
}

void Hud::fillRect(int x0, int y0, int w, int h, uint16_t color565) {
  HudBufferFillRect fillRect(buffer, screenW, HUD_H);
  fillRect.fillRect565(x0, y0, w, h, color565);
}

void Hud::drawFace(int x, int y, int w, int h) {
  const uint16_t skin = Color565::rgb(232, 188, 140);
  const uint16_t skinDark = Color565::rgb(186, 132, 96);
  const uint16_t hair = Color565::rgb(90, 54, 28);
  const uint16_t eye = Color565::rgb(18, 18, 24);
  const uint16_t mouth = Color565::rgb(124, 20, 18);
  const uint16_t bruised = Color565::rgb(170, 64, 70);

  int headW = 24;
  int headH = 24;
  int headX = x + (w - headW) / 2;
  int headY = y + (h - headH) / 2;
  fillRect(headX, headY, headW, headH, skin);
  fillRect(headX, headY, headW, 4, hair);
  fillRect(headX, headY + headH - 2, headW, 2, skinDark);
  fillRect(headX, headY, 2, headH, skinDark);
  fillRect(headX + headW - 2, headY, 2, headH, skinDark);

  int leftEyeX = headX + 6;
  int rightEyeX = headX + 16;
  int eyeY = headY + 8;

  switch (faceMood) {
    case FaceMood::Blink:
      fillRect(leftEyeX, eyeY + 1, 4, 1, eye);
      fillRect(rightEyeX - 1, eyeY + 1, 4, 1, eye);
      break;
    case FaceMood::Hurt:
      fillRect(leftEyeX, eyeY + 1, 3, 1, eye);
      fillRect(rightEyeX, eyeY, 2, 3, eye);
      fillRect(headX + 3, headY + 4, 4, 4, bruised);
      break;
    case FaceMood::Low:
      fillRect(leftEyeX, eyeY + 1, 2, 2, eye);
      fillRect(rightEyeX, eyeY + 1, 2, 2, eye);
      fillRect(leftEyeX - 1, eyeY - 1, 4, 1, eye);
      fillRect(rightEyeX - 1, eyeY - 1, 4, 1, eye);
      break;
    default:
      fillRect(leftEyeX, eyeY, 2, 3, eye);
      fillRect(rightEyeX, eyeY, 2, 3, eye);
      break;
  }

  fillRect(headX + 11, headY + 11, 2, 5, skinDark);

  switch (faceMood) {
    case FaceMood::Shoot:
      fillRect(headX + 7, headY + 18, 10, 2, mouth);
      fillRect(headX + 9, headY + 16, 6, 2, mouth);
      break;
    case FaceMood::Hurt:
      fillRect(headX + 8, headY + 19, 8, 1, mouth);
      fillRect(headX + 7, headY + 20, 2, 1, mouth);
      fillRect(headX + 15, headY + 18, 2, 1, mouth);
      break;
    case FaceMood::Low:
      fillRect(headX + 8, headY + 20, 8, 1, mouth);
      fillRect(headX + 7, headY + 19, 2, 1, mouth);
      fillRect(headX + 15, headY + 19, 2, 1, mouth);
      break;
    default:
      fillRect(headX + 8, headY + 18, 8, 1, mouth);
      fillRect(headX + 7, headY + 17, 2, 1, mouth);
      fillRect(headX + 15, headY + 17, 2, 1, mouth);
      break;
  }
}

void Hud::render() {
  if (dirty.count() == 0) {
    return;
  }

  const uint16_t panelBg = Color565::rgb(46, 28, 18);
  const uint16_t panelDark = Color565::rgb(24, 12, 8);
  const uint16_t frame = Color565::rgb(164, 118, 64);
  const uint16_t frameHi = Color565::rgb(212, 170, 100);
  const uint16_t numberBg = Color565::rgb(8, 6, 6);
  const uint16_t text = Color565::rgb(236, 220, 180);
  const uint16_t accent = Color565::rgb(228, 188, 84);
  const uint16_t green = Color565::rgb(72, 188, 92);
  const uint16_t red = Color565::rgb(196, 72, 58);
  HudBufferFillRect fillRectTarget(buffer, screenW, HUD_H);
  char valueBuf[16];

  fillRect(0, 0, screenW, HUD_H, panelBg);
  fillRect(0, 0, screenW, 2, frameHi);
  fillRect(0, 2, screenW, 2, frame);
  fillRect(0, HUD_H - 2, screenW, 2, panelDark);

  fillRect(HUD_LIVES_X, HUD_LIVES_Y, HUD_LIVES_W, HUD_LIVES_H, panelDark);
  fillRect(HUD_FACE_X, HUD_FACE_Y, HUD_FACE_W, HUD_FACE_H, panelDark);
  fillRect(HUD_STATS_X, HUD_AMMO_Y, HUD_STATS_W, HUD_AMMO_H, panelDark);
  fillRect(HUD_STATS_X, HUD_ENERGY_Y, HUD_STATS_W, HUD_ENERGY_H, panelDark);

  fillRect(HUD_LIVES_X, HUD_LIVES_Y, HUD_LIVES_W, 2, frameHi);
  fillRect(HUD_LIVES_X, HUD_LIVES_Y, 2, HUD_LIVES_H, frameHi);
  fillRect(HUD_LIVES_X + HUD_LIVES_W - 2, HUD_LIVES_Y, 2, HUD_LIVES_H, frame);
  fillRect(HUD_LIVES_X, HUD_LIVES_Y + HUD_LIVES_H - 2, HUD_LIVES_W, 2, frame);

  fillRect(HUD_FACE_X, HUD_FACE_Y, HUD_FACE_W, 2, frameHi);
  fillRect(HUD_FACE_X, HUD_FACE_Y, 2, HUD_FACE_H, frameHi);
  fillRect(HUD_FACE_X + HUD_FACE_W - 2, HUD_FACE_Y, 2, HUD_FACE_H, frame);
  fillRect(HUD_FACE_X, HUD_FACE_Y + HUD_FACE_H - 2, HUD_FACE_W, 2, frame);

  fillRect(HUD_STATS_X, HUD_AMMO_Y, HUD_STATS_W, 2, frameHi);
  fillRect(HUD_STATS_X, HUD_AMMO_Y, 2, HUD_AMMO_H, frameHi);
  fillRect(HUD_STATS_X + HUD_STATS_W - 2, HUD_AMMO_Y, 2, HUD_AMMO_H, frame);
  fillRect(HUD_STATS_X, HUD_AMMO_Y + HUD_AMMO_H - 2, HUD_STATS_W, 2, frame);

  fillRect(HUD_STATS_X, HUD_ENERGY_Y, HUD_STATS_W, 2, frameHi);
  fillRect(HUD_STATS_X, HUD_ENERGY_Y, 2, HUD_ENERGY_H, frameHi);
  fillRect(HUD_STATS_X + HUD_STATS_W - 2, HUD_ENERGY_Y, 2, HUD_ENERGY_H, frame);
  fillRect(HUD_STATS_X, HUD_ENERGY_Y + HUD_ENERGY_H - 2, HUD_STATS_W, 2, frame);

  FontRenderer::drawText(FONT_5X7, fillRectTarget, HUD_LIVES_X + 6, HUD_LIVES_Y + 4, "LIVES", 1, text);
  snprintf(valueBuf, sizeof(valueBuf), "%d", lives);
  int livesTextW = FontRenderer::textWidth(FONT_5X7, valueBuf, 2);
  FontRenderer::drawText(
    FONT_5X7,
    fillRectTarget,
    HUD_LIVES_X + (HUD_LIVES_W - livesTextW) / 2,
    HUD_LIVES_Y + 14,
    valueBuf,
    2,
    accent);

  FontRenderer::drawText(FONT_5X7, fillRectTarget, HUD_STATS_X + 6, HUD_AMMO_Y + 3, "AMMO", 1, text);
  snprintf(valueBuf, sizeof(valueBuf), "%d", ammo);
  FontRenderer::drawText(FONT_5X7, fillRectTarget, HUD_STATS_X + 34, HUD_AMMO_Y + 3, valueBuf, 1, accent);

  FontRenderer::drawText(FONT_5X7, fillRectTarget, HUD_STATS_X + 6, HUD_ENERGY_Y + 2, "NRG", 1, text);
  snprintf(valueBuf, sizeof(valueBuf), "%d", energy);
  FontRenderer::drawText(FONT_5X7, fillRectTarget, HUD_STATS_X + 34, HUD_ENERGY_Y + 2, valueBuf, 1, accent);

  fillRect(HUD_STATS_X + 6, HUD_ENERGY_Y + 8, 48, 4, numberBg);
  int barW = (48 * energy) / 100;
  uint16_t barColor = (energy > 35) ? green : red;
  fillRect(HUD_STATS_X + 6, HUD_ENERGY_Y + 8, barW, 4, barColor);
  if (barW > 2) {
    fillRect(HUD_STATS_X + 6, HUD_ENERGY_Y + 8, barW, 1, Color565::lighten(barColor));
  }

  drawFace(HUD_FACE_X + 6, HUD_FACE_Y + 2, HUD_FACE_W - 12, HUD_FACE_H - 4);

  const int keySlotY = HUD_FACE_Y + 5;
  const int keySlotW = 8;
  const int keySlotH = 8;
  const int keySlotGap = 2;
  const int keyStartX = HUD_FACE_X + HUD_FACE_W - 32;
  const uint16_t keySlotBg = Color565::rgb(22, 16, 12);
  const uint16_t keySlotFrame = Color565::rgb(120, 92, 48);
  const KeyColor keyOrder[3] = {KeyColor::Red, KeyColor::Green, KeyColor::Blue};
  for (int i = 0; i < 3; i++) {
    int slotX = keyStartX + i * (keySlotW + keySlotGap);
    KeyColor keyColor = keyOrder[i];
    uint16_t color565 = Keys::color565(keyColor);
    bool owned = (keysMask & Keys::bitFor(keyColor)) != 0;
    fillRect(slotX, keySlotY, keySlotW, keySlotH, keySlotBg);
    fillRect(slotX, keySlotY, keySlotW, 1, keySlotFrame);
    fillRect(slotX, keySlotY + keySlotH - 1, keySlotW, 1, keySlotFrame);
    fillRect(slotX, keySlotY, 1, keySlotH, keySlotFrame);
    fillRect(slotX + keySlotW - 1, keySlotY, 1, keySlotH, keySlotFrame);
    if (owned) {
      fillRect(slotX + 2, keySlotY + 2, 4, 4, color565);
      fillRect(slotX + 2, keySlotY + 1, 2, 1, Color565::lighten(color565));
    } else {
      fillRect(slotX + 2, keySlotY + 2, 4, 4, Color565::darken(color565));
    }
  }
}

void Hud::flush(IRenderTarget& target) {
  static constexpr int FLUSH_ROWS_PER_BATCH = 2;
  static uint16_t flushBuf[MAX_SCREEN_W * FLUSH_ROWS_PER_BATCH];
  if (dirty.count() == 0) {
    return;
  }

  dirty.mergeAll();
  for (int i = 0; i < dirty.count(); i++) {
    const Rect& rect = dirty[i];
    int w = rect.x1 - rect.x0 + 1;
    int h = rect.y1 - rect.y0 + 1;
    for (int y0 = 0; y0 < h; y0 += FLUSH_ROWS_PER_BATCH) {
      int batchRows = min(FLUSH_ROWS_PER_BATCH, h - y0);
      for (int row = 0; row < batchRows; row++) {
        const uint8_t* src = &buffer[(rect.y0 + y0 + row) * screenW + rect.x0];
        uint16_t* dst = &flushBuf[row * w];
        for (int x = 0; x < w; x++) {
          dst[x] = unpackRgb332(src[x]);
        }
      }
      target.blit565(rect.x0, worldScreenH + rect.y0 + y0, w, batchRows, flushBuf);
    }
  }
  dirty.clear();
}
