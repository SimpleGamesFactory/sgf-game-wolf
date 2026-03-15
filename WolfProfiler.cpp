#include "WolfProfiler.h"

namespace {

constexpr int SLOT_COUNT = WolfProfiler::Count;
constexpr const char* SLOT_LABELS[SLOT_COUNT] = {
  "fps",
  "physics",
  "input",
  "zupd",
  "hudanim",
  "floor",
  "world",
  "sprites",
  "weapon",
  "minimap",
  "fpsov",
  "present",
  "hudr",
  "hudf",
};

}  // namespace

WolfProfiler::WolfProfiler()
  : stageProfiler("wolf", profilerSlots, Count) {}

void WolfProfiler::begin() {
  if (initialized) {
    return;
  }
  for (int i = 0; i < SLOT_COUNT; i++) {
    stageProfiler.setLabel(i, SLOT_LABELS[i]);
  }
  stageProfiler.setMode(Fps, Profiler::CounterSlot);
  for (int i = 1; i < SLOT_COUNT; i++) {
    stageProfiler.setMode(i, Profiler::SampleSlot);
  }
  initialized = true;
}

void WolfProfiler::add(Slot slot, uint32_t elapsedUs) {
  stageProfiler.probe(slot, elapsedUs);
}

void WolfProfiler::frame() {
  stageProfiler.increment(Fps);
}

const char* WolfProfiler::slotLabel(Slot slot) {
  switch (slot) {
    case Slot::Fps:
      return "fps";
    case Slot::Physics:
      return "physics";
    case Slot::Input:
      return "input";
    case Slot::ZombieUpdate:
      return "zupd";
    case Slot::HudAnim:
      return "hudanim";
    case Slot::Floor:
      return "floor";
    case Slot::World:
      return "world";
    case Slot::Sprites:
      return "sprites";
    case Slot::Weapon:
      return "weapon";
    case Slot::Minimap:
      return "minimap";
    case Slot::FpsOverlay:
      return "fpsov";
    case Slot::Present:
      return "present";
    case Slot::HudRender:
      return "hudr";
    case Slot::HudFlush:
      return "hudf";
    case Slot::Count:
      return "count";
  }
  return "?";
}
