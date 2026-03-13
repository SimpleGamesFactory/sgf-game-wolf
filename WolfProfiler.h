#pragma once

#include <stdint.h>

#include "SGF/Profiler.h"

class WolfProfiler {
public:
  enum class Slot : uint8_t {
    Fps,
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

  WolfProfiler();

  void begin();
  void add(Slot slot, uint32_t elapsedUs);
  void frame();
  Profiler& profiler() { return stageProfiler; }
  const Profiler& profiler() const { return stageProfiler; }

private:
  Profiler::Slot profilerSlots[static_cast<int>(Slot::Count)]{};
  Profiler stageProfiler;
  bool initialized = false;

  static const char* slotLabel(Slot slot);
};
