#pragma once

#include <stdint.h>

#include "Sprite.h"

class Zombie {
public:
  static constexpr int MAX_COUNT = 16;
  static constexpr char MAP_SYMBOL = '@';
  static constexpr int TEX_SIZE = 16;

  struct WorldView {
    const uint8_t* map = nullptr;
    const uint8_t* doorOpenBits = nullptr;
    int mapStride = 0;
    int mapWidth = 0;
    int mapHeight = 0;
    float playerX = 0.0f;
    float playerY = 0.0f;
    uint32_t nowMs = 0;
    float delta = 0.0f;
  };

  struct AimView {
    const WorldView& world;
    float dirX = 0.0f;
    float dirY = 0.0f;
  };

  void clear();
  void spawn(float x, float y);
  bool isAlive() const;
  float getX() const;
  float getY() const;
  bool isAttacking(uint32_t nowMs) const;
  void kill();
  void update(const WorldView& world, int& damageOut);
  bool tryHit(const AimView& aim, float& hitDistance) const;
  uint16_t texel(int texX, int texY, uint32_t nowMs) const;
  const WolfRender::ISprite& sprite() const { return renderSprite; }

  static void loadSpawns(Zombie* zombies, int& zombieCount, const char* layout);

private:
  static constexpr float RADIUS = 0.22f;
  static constexpr float DETECT_RANGE = 6.0f;
  static constexpr float STOP_RANGE = 1.45f;
  static constexpr float SHOOT_RANGE = 4.5f;
  static constexpr float MOVE_SPEED = 0.72f;
  static constexpr int DAMAGE = 7;
  static constexpr uint32_t SHOT_COOLDOWN_MS = 900;
  static constexpr uint32_t ATTACK_FLASH_MS = 140;

  float x = 0.0f;
  float y = 0.0f;
  bool alive = false;
  uint32_t nextShotMs = 0;
  uint32_t attackUntilMs = 0;
  WolfRender::Sprite renderSprite;

  static bool lineBlocked(const WorldView& world, float fromX, float fromY, float toX, float toY);
  static void buildSpriteTexture(const void* owner, uint16_t* outTexture, uint32_t nowMs);
  bool blockedAt(const WorldView& world, float nextX, float nextY) const;
};
