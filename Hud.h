#pragma once

#include <stdint.h>

#include "SGF/DirtyRects.h"
#include "SGF/IRenderTarget.h"

class Hud {
public:
  static constexpr int MAX_SCREEN_W = 240;
  static constexpr int HUD_H = 40;
  static constexpr int HUD_LIVES_X = 4;
  static constexpr int HUD_LIVES_Y = 4;
  static constexpr int HUD_LIVES_W = 58;
  static constexpr int HUD_LIVES_H = 32;
  static constexpr int HUD_FACE_X = 74;
  static constexpr int HUD_FACE_Y = 4;
  static constexpr int HUD_FACE_W = 92;
  static constexpr int HUD_FACE_H = 32;
  static constexpr int HUD_STATS_X = 176;
  static constexpr int HUD_AMMO_Y = 4;
  static constexpr int HUD_AMMO_H = 14;
  static constexpr int HUD_ENERGY_Y = 21;
  static constexpr int HUD_ENERGY_H = 14;
  static constexpr int HUD_STATS_W = 60;

  enum class FaceMood : uint8_t {
    Normal,
    Blink,
    Shoot,
    Hurt,
    Low
  };

  void begin(int screenW, int worldScreenH);
  void setLives(int value);
  void setAmmo(int value);
  void setEnergy(int value);
  void setKeys(uint8_t value);
  void setFaceMood(FaceMood value);
  void invalidate();
  void render();
  void flush(IRenderTarget& target);

private:
  DirtyRects dirty;
  uint16_t buffer[MAX_SCREEN_W * HUD_H]{};
  int screenW = 0;
  int worldScreenH = 0;
  int lives = 3;
  int ammo = 99;
  int energy = 100;
  uint8_t keysMask = 0;
  FaceMood faceMood = FaceMood::Normal;

  void markDirtyRect(int x, int y, int w, int h);
  void fillRect(int x0, int y0, int w, int h, uint16_t color565);
  void drawFace(int x, int y, int w, int h);
};
