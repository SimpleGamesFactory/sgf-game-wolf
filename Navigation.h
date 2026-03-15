#pragma once

#include <stdint.h>

namespace Navigation {

struct GridWorldView {
  const uint8_t* map = nullptr;
  const uint8_t* doorOpenAmounts = nullptr;
  int mapStride = 0;
  int mapWidth = 0;
  int mapHeight = 0;
};

struct TargetRequest {
  GridWorldView world{};
  float actorX = 0.0f;
  float actorY = 0.0f;
  float goalX = 0.0f;
  float goalY = 0.0f;
  uint32_t nowMs = 0u;
  uint32_t rebuildMs = 0u;
};

struct PathState {
  static constexpr uint8_t POINT_CAP = 24;

  uint32_t nextRebuildMs = 0u;
  int goalCellX = -1;
  int goalCellY = -1;
  uint8_t pointCount = 0u;
  uint8_t pointIndex = 0u;
  uint8_t pointsX[POINT_CAP]{};
  uint8_t pointsY[POINT_CAP]{};

  void clear();
};

bool isDoorOpen(const GridWorldView& world, int cellX, int cellY);
bool isCellBlocked(const GridWorldView& world, int cellX, int cellY);
bool lineBlocked(const GridWorldView& world, float fromX, float fromY, float toX, float toY);
bool nextTarget(const TargetRequest& request, PathState& state, float& outX, float& outY);

}  // namespace Navigation
