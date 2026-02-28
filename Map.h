#pragma once

#include <stdint.h>

namespace MapLayout {

inline constexpr char E1M1[] =
#include "../../../maps/e1m1.txt"
;

constexpr int computeWidth(const char* text) {
  int width = 0;
  int current = 0;
  for (int i = 0; text[i] != '\0'; i++) {
    char symbol = text[i];
    if (symbol == '\r') {
      continue;
    }
    if (symbol == '\n') {
      if (current > width) {
        width = current;
      }
      current = 0;
      continue;
    }
    current++;
  }
  if (current > width) {
    width = current;
  }
  return width;
}

constexpr int computeHeight(const char* text) {
  int height = 0;
  int current = 0;
  for (int i = 0; text[i] != '\0'; i++) {
    char symbol = text[i];
    if (symbol == '\r') {
      continue;
    }
    if (symbol == '\n') {
      if (current > 0) {
        height++;
      }
      current = 0;
      continue;
    }
    current++;
  }
  if (current > 0) {
    height++;
  }
  return height;
}

}  // namespace MapLayout

class Map {
public:
  static constexpr int MAX_WIDTH = 64;
  static constexpr int MAX_HEIGHT = 64;
  static constexpr int WIDTH = MapLayout::computeWidth(MapLayout::E1M1);
  static constexpr int HEIGHT = MapLayout::computeHeight(MapLayout::E1M1);
  static_assert(WIDTH > 0, "Map width must be greater than zero");
  static_assert(HEIGHT > 0, "Map height must be greater than zero");
  static_assert(WIDTH <= MAX_WIDTH, "Map width exceeds maximum");
  static_assert(HEIGHT <= MAX_HEIGHT, "Map height exceeds maximum");

  struct Spawn {
    float x = 3.5f;
    float y = 3.5f;
    float dirX = 1.0f;
    float dirY = 0.0f;
  };

  static void load(
    uint8_t (&tiles)[MAX_HEIGHT][MAX_WIDTH],
    bool (&doorOpen)[MAX_HEIGHT][MAX_WIDTH],
    int& width,
    int& height,
    Spawn& spawn);

private:
  static uint8_t decodeTile(char symbol);
  static bool decodeSpawn(char symbol, Spawn& spawn, int x, int y);
};
