#include "Map.h"

#include "Zombie.h"

void Map::load(
  uint8_t (*tiles)[MAX_WIDTH],
  uint8_t* doorOpenBits,
  int& width,
  int& height,
  Spawn& spawn
) {
  spawn = Spawn();
  width = WIDTH;
  height = HEIGHT;

  for (int i = 0; i < DOOR_OPEN_BYTES; i++) {
    doorOpenBits[i] = 0;
  }
  for (int y = 0; y < MAX_HEIGHT; y++) {
    for (int x = 0; x < MAX_WIDTH; x++) {
      tiles[y][x] = 0;
    }
  }

  int x = 0;
  int y = 0;
  for (const char* cursor = MapLayout::E1M1; *cursor != '\0' && y < height; cursor++) {
    char symbol = *cursor;
    if (symbol == '\r') {
      continue;
    }
    if (symbol == '\n') {
      if (x == 0) {
        continue;
      }
      x = 0;
      y++;
      continue;
    }
    if (x >= width) {
      continue;
    }

    if (decodeSpawn(symbol, spawn, x, y)) {
      tiles[y][x] = 0;
    } else {
      tiles[y][x] = decodeTile(symbol);
    }
    x++;
  }
}

uint8_t Map::decodeTile(char symbol) {
  switch (symbol) {
    case '.': return 0;
    case Zombie::MAP_SYMBOL: return 0;
    case '#': return 1;
    case 'A': return 2;
    case 'B': return 3;
    case 'C': return 4;
    case 'E': return 5;
    case 'F': return 6;
    case 'G': return 7;
    case 'H': return 8;
    case 'I': return 9;
    case 'D': return 'D';
    case '1': return '1';
    case '2': return '2';
    case '3': return '3';
    case 'r': return 'r';
    case 'g': return 'g';
    case 'b': return 'b';
    case 'a': return 'a';
    case 'h': return 'h';
    default: return 0;
  }
}

bool Map::decodeSpawn(char symbol, Spawn& spawn, int x, int y) {
  switch (symbol) {
    case '>':
      spawn.x = static_cast<float>(x) + 0.5f;
      spawn.y = static_cast<float>(y) + 0.5f;
      spawn.dirX = 1.0f;
      spawn.dirY = 0.0f;
      return true;
    case '<':
      spawn.x = static_cast<float>(x) + 0.5f;
      spawn.y = static_cast<float>(y) + 0.5f;
      spawn.dirX = -1.0f;
      spawn.dirY = 0.0f;
      return true;
    case '^':
      spawn.x = static_cast<float>(x) + 0.5f;
      spawn.y = static_cast<float>(y) + 0.5f;
      spawn.dirX = 0.0f;
      spawn.dirY = -1.0f;
      return true;
    case 'v':
      spawn.x = static_cast<float>(x) + 0.5f;
      spawn.y = static_cast<float>(y) + 0.5f;
      spawn.dirX = 0.0f;
      spawn.dirY = 1.0f;
      return true;
    default:
      return false;
  }
}
