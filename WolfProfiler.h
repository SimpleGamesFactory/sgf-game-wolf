#pragma once

#include <stdint.h>

class WolfProfiler {
public:
  enum class Stage : uint8_t {
    Physics,
    Input,
    ZombieUpdate,
    HudAnim,
    Floor,
    World,
    Keys,
    ZombieRender,
    Weapon,
    Minimap,
    FpsOverlay,
    Present,
    HudRender,
    HudFlush,
    Count
  };

  void begin(uint32_t baudRate = 115200u);
  void add(Stage stage, uint32_t elapsedUs);
  void frame();
  void emitIfReady(uint32_t nowMs, uint16_t fpsValue);

private:
  static constexpr uint32_t REPORT_INTERVAL_MS = 1000u;

  bool serialReady = false;
  uint32_t windowStartMs = 0;
  uint32_t frameCount = 0;
  uint32_t accumUs[static_cast<int>(Stage::Count)]{};

  void resetWindow(uint32_t nowMs);
  static const char* stageLabel(Stage stage);
};
