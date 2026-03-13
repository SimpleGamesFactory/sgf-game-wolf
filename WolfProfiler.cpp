#include "WolfProfiler.h"

namespace {

constexpr int SLOT_COUNT = static_cast<int>(WolfProfiler::Slot::Count);

}  // namespace

WolfProfiler::WolfProfiler()
  : stageProfiler("prof", profilerSlots, static_cast<uint8_t>(Slot::Count)) {}

void WolfProfiler::begin() {
  if (initialized) {
    return;
  }
  for (int i = 0; i < SLOT_COUNT; i++) {
    stageProfiler.setLabel(i, slotLabel(static_cast<Slot>(i)));
  }
  stageProfiler.setMode(static_cast<int>(Slot::Fps), Profiler::CounterSlot);
  for (int i = 1; i < SLOT_COUNT; i++) {
    stageProfiler.setMode(i, Profiler::SampleSlot);
  }
  initialized = true;
}

void WolfProfiler::add(Slot slot, uint32_t elapsedUs) {
  stageProfiler.probe(static_cast<int>(slot), elapsedUs);
}

void WolfProfiler::frame() {
  stageProfiler.increment(static_cast<int>(Slot::Fps));
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
    case Slot::Keys:
      return "keys";
    case Slot::ZombieRender:
      return "zrend";
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
