#include "WolfProfiler.h"

#include <Arduino.h>

namespace {

constexpr int STAGE_COUNT = static_cast<int>(WolfProfiler::Stage::Count);

}  // namespace

void WolfProfiler::begin(uint32_t baudRate) {
  if (!serialReady) {
    Serial.begin(baudRate);
    serialReady = true;
  }
  resetWindow(millis());
}

void WolfProfiler::add(Stage stage, uint32_t elapsedUs) {
  accumUs[static_cast<int>(stage)] += elapsedUs;
}

void WolfProfiler::frame() {
  frameCount++;
}

void WolfProfiler::emitIfReady(uint32_t nowMs, uint16_t fpsValue) {
  if (!serialReady) {
    return;
  }
  if (windowStartMs == 0) {
    resetWindow(nowMs);
  }

  uint32_t elapsedMs = nowMs - windowStartMs;
  if (elapsedMs < REPORT_INTERVAL_MS || frameCount == 0) {
    return;
  }

  Serial.print("[prof] fps=");
  Serial.print(fpsValue);
  Serial.print(" frames=");
  Serial.print(frameCount);
  for (int i = 0; i < STAGE_COUNT; i++) {
    uint32_t avgUs = accumUs[i] / frameCount;
    Serial.print(' ');
    Serial.print(stageLabel(static_cast<Stage>(i)));
    Serial.print('=');
    Serial.print(avgUs / 1000u);
    Serial.print('.');
    Serial.print((avgUs % 1000u) / 100u);
    Serial.print("ms");
  }
  Serial.println();

  resetWindow(nowMs);
}

void WolfProfiler::resetWindow(uint32_t nowMs) {
  windowStartMs = nowMs;
  frameCount = 0;
  for (int i = 0; i < STAGE_COUNT; i++) {
    accumUs[i] = 0;
  }
}

const char* WolfProfiler::stageLabel(Stage stage) {
  switch (stage) {
    case Stage::Physics:
      return "physics";
    case Stage::Input:
      return "input";
    case Stage::ZombieUpdate:
      return "zupd";
    case Stage::HudAnim:
      return "hudanim";
    case Stage::Floor:
      return "floor";
    case Stage::World:
      return "world";
    case Stage::Keys:
      return "keys";
    case Stage::ZombieRender:
      return "zrend";
    case Stage::Weapon:
      return "weapon";
    case Stage::Minimap:
      return "minimap";
    case Stage::FpsOverlay:
      return "fpsov";
    case Stage::Present:
      return "present";
    case Stage::HudRender:
      return "hudr";
    case Stage::HudFlush:
      return "hudf";
    case Stage::Count:
      return "count";
  }
  return "?";
}
