#include "Navigation.h"

#include <Arduino.h>
#include <stdlib.h>
#include <string.h>

#include "Door.h"

namespace Navigation {

namespace {

constexpr uint16_t INVALID_NODE = 0xFFFFu;
constexpr int MAX_MAP_W = 64;
constexpr int MAX_MAP_H = 64;
constexpr int MAX_CELLS = MAX_MAP_W * MAX_MAP_H;
constexpr int BITSET_BYTES = (MAX_CELLS + 7) / 8;
constexpr float WAYPOINT_REACHED_RADIUS = 0.18f;

struct Scratch {
  uint16_t* gScore = nullptr;
  uint16_t* parent = nullptr;
  uint16_t* openList = nullptr;
  uint8_t* openBits = nullptr;
  uint8_t* closedBits = nullptr;
  bool ready = false;
};

template <typename T>
T* allocBuffer(size_t count) {
  return static_cast<T*>(malloc(sizeof(T) * count));
}

Scratch& scratch() {
  static Scratch s;
  if (s.ready) {
    return s;
  }

  s.gScore = allocBuffer<uint16_t>(MAX_CELLS);
  s.parent = allocBuffer<uint16_t>(MAX_CELLS);
  s.openList = allocBuffer<uint16_t>(MAX_CELLS);
  s.openBits = allocBuffer<uint8_t>(BITSET_BYTES);
  s.closedBits = allocBuffer<uint8_t>(BITSET_BYTES);
  s.ready = s.gScore != nullptr &&
            s.parent != nullptr &&
            s.openList != nullptr &&
            s.openBits != nullptr &&
            s.closedBits != nullptr;
  return s;
}

int compactIndex(int x, int y, int width) {
  return y * width + x;
}

void bitsetClear(uint8_t* bits, int bitCount) {
  if (bits == nullptr || bitCount <= 0) {
    return;
  }
  memset(bits, 0, (bitCount + 7) / 8);
}

bool bitsetGet(const uint8_t* bits, int index) {
  return bits != nullptr && (bits[index >> 3] & (1u << (index & 7))) != 0u;
}

void bitsetSet(uint8_t* bits, int index, bool value) {
  if (bits == nullptr) {
    return;
  }
  uint8_t mask = 1u << (index & 7);
  if (value) {
    bits[index >> 3] |= mask;
  } else {
    bits[index >> 3] &= ~mask;
  }
}

}  // namespace

bool isDoorOpen(const GridWorldView& world, int cellX, int cellY) {
  if (world.doorOpenAmounts == nullptr) {
    return false;
  }
  return Door::isPassable(world.doorOpenAmounts[cellY * world.mapStride + cellX]);
}

bool isCellBlocked(const GridWorldView& world, int cellX, int cellY) {
  if (cellX < 0 || cellX >= world.mapWidth || cellY < 0 || cellY >= world.mapHeight) {
    return true;
  }
  uint8_t tile = world.map[cellY * world.mapStride + cellX];
  if (tile == 0) {
    return false;
  }
  if (Door::isTile(tile)) {
    return !isDoorOpen(world, cellX, cellY);
  }
  return true;
}

bool lineBlocked(const GridWorldView& world, float fromX, float fromY, float toX, float toY) {
  float dx = toX - fromX;
  float dy = toY - fromY;
  float distance = sqrtf(dx * dx + dy * dy);
  if (distance <= 0.001f) {
    return false;
  }

  int steps = constrain(distance * 12.0f, 1, 128);
  for (int i = 1; i < steps; i++) {
    float t = i / float(steps);
    float sampleX = fromX + dx * t;
    float sampleY = fromY + dy * t;
    if (isCellBlocked(world, sampleX, sampleY)) {
      return true;
    }
  }
  return false;
}

namespace {

uint16_t heuristic(int x0, int y0, int x1, int y1) {
  int dx = x0 > x1 ? x0 - x1 : x1 - x0;
  int dy = y0 > y1 ? y0 - y1 : y1 - y0;
  return dx + dy;
}

bool buildPath(
  const TargetRequest& request,
  PathState& state,
  int startCellX,
  int startCellY,
  int goalCellX,
  int goalCellY
) {
  Scratch& s = scratch();
  if (!s.ready) {
    state.clear();
    return false;
  }

  int cellCount = request.world.mapWidth * request.world.mapHeight;
  for (int i = 0; i < cellCount; ++i) {
    s.gScore[i] = INVALID_NODE;
    s.parent[i] = INVALID_NODE;
  }
  bitsetClear(s.openBits, cellCount);
  bitsetClear(s.closedBits, cellCount);

  int startIndex = compactIndex(startCellX, startCellY, request.world.mapWidth);
  int goalIndex = compactIndex(goalCellX, goalCellY, request.world.mapWidth);
  uint16_t openCount = 0u;
  s.gScore[startIndex] = 0u;
  s.parent[startIndex] = startIndex;
  s.openList[openCount++] = startIndex;
  bitsetSet(s.openBits, startIndex, true);

  while (openCount > 0u) {
    uint16_t bestOpenSlot = 0u;
    uint16_t bestNode = s.openList[0];
    uint16_t bestScore = INVALID_NODE;
    for (uint16_t i = 0u; i < openCount; ++i) {
      uint16_t node = s.openList[i];
      int nodeX = node % request.world.mapWidth;
      int nodeY = node / request.world.mapWidth;
      uint16_t fScore =
        s.gScore[node] + heuristic(nodeX, nodeY, goalCellX, goalCellY);
      if (fScore < bestScore) {
        bestScore = fScore;
        bestNode = node;
        bestOpenSlot = i;
      }
    }

    s.openList[bestOpenSlot] = s.openList[--openCount];
    bitsetSet(s.openBits, bestNode, false);
    bitsetSet(s.closedBits, bestNode, true);

    if (bestNode == goalIndex) {
      break;
    }

    int cellX = bestNode % request.world.mapWidth;
    int cellY = bestNode / request.world.mapWidth;
    static constexpr int OFFSETS[4][2] = {
      {1, 0},
      {-1, 0},
      {0, 1},
      {0, -1},
    };

    for (int i = 0; i < 4; ++i) {
      int nextX = cellX + OFFSETS[i][0];
      int nextY = cellY + OFFSETS[i][1];
      if (isCellBlocked(request.world, nextX, nextY)) {
        continue;
      }
      int nextIndex = compactIndex(nextX, nextY, request.world.mapWidth);
      if (bitsetGet(s.closedBits, nextIndex)) {
        continue;
      }

      uint16_t tentative = s.gScore[bestNode] + 1u;
      if (tentative >= s.gScore[nextIndex]) {
        continue;
      }

      s.gScore[nextIndex] = tentative;
      s.parent[nextIndex] = bestNode;
      if (!bitsetGet(s.openBits, nextIndex)) {
        s.openList[openCount++] = nextIndex;
        bitsetSet(s.openBits, nextIndex, true);
      }
    }
  }

  if (s.parent[goalIndex] == INVALID_NODE) {
    state.clear();
    return false;
  }

  uint16_t reverseCount = 0u;
  uint16_t node = goalIndex;
  while (node != startIndex && reverseCount < MAX_CELLS) {
    s.openList[reverseCount++] = node;
    node = s.parent[node];
    if (node == INVALID_NODE) {
      state.clear();
      return false;
    }
  }

  state.pointCount = 0u;
  state.pointIndex = 0u;
  for (int i = reverseCount - 1;
       i >= 0 && state.pointCount < PathState::POINT_CAP;
       --i) {
    uint16_t pathNode = s.openList[i];
    state.pointsX[state.pointCount] = pathNode % request.world.mapWidth;
    state.pointsY[state.pointCount] = pathNode / request.world.mapWidth;
    state.pointCount++;
  }
  state.goalCellX = goalCellX;
  state.goalCellY = goalCellY;
  state.nextRebuildMs = request.nowMs + request.rebuildMs;
  return state.pointCount > 0u;
}

}  // namespace

void PathState::clear() {
  nextRebuildMs = 0u;
  goalCellX = -1;
  goalCellY = -1;
  pointCount = 0u;
  pointIndex = 0u;
}

bool nextTarget(const TargetRequest& request, PathState& state, float& outX, float& outY) {
  if (request.world.map == nullptr || request.world.mapWidth <= 0 || request.world.mapHeight <= 0) {
    state.clear();
    return false;
  }

  int startCellX = request.actorX;
  int startCellY = request.actorY;
  int goalCellX = request.goalX;
  int goalCellY = request.goalY;
  if (startCellX == goalCellX && startCellY == goalCellY) {
    state.clear();
    return false;
  }

  bool needRebuild =
    state.pointCount == 0u ||
    state.pointIndex >= state.pointCount ||
    state.goalCellX != goalCellX ||
    state.goalCellY != goalCellY ||
    request.nowMs >= state.nextRebuildMs;
  if (needRebuild &&
      !buildPath(request, state, startCellX, startCellY, goalCellX, goalCellY)) {
    return false;
  }

  while (state.pointIndex < state.pointCount) {
    float targetX = state.pointsX[state.pointIndex] + 0.5f;
    float targetY = state.pointsY[state.pointIndex] + 0.5f;
    float dx = targetX - request.actorX;
    float dy = targetY - request.actorY;
    if ((dx * dx + dy * dy) <= (WAYPOINT_REACHED_RADIUS * WAYPOINT_REACHED_RADIUS)) {
      state.pointIndex++;
      continue;
    }
    outX = targetX;
    outY = targetY;
    return true;
  }

  return false;
}

}  // namespace Navigation
