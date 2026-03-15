#pragma once

#include <stdint.h>

#include "Navigation.h"
#include "Sprite.h"

class Enemy {
public:
  static constexpr int MAX_COUNT = 16;
  static constexpr int TEX_SIZE = 16;

  enum class RespawnMode : uint8_t {
    None = 0,
    Auto,
  };

  enum class AttackMode : uint8_t {
    Ranged = 0,
    Touch,
  };

  struct WorldView {
    Navigation::GridWorldView grid{};
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

  RespawnMode respawnMode = RespawnMode::None;
  AttackMode attackMode = AttackMode::Ranged;
  float radius = 0.22f;
  float detectRange = 6.0f;
  float stopRange = 0.0f;
  float moveSpeed = 0.0f;
  float attackRange = 0.0f;
  int damage = 7;
  int maxHp = 0;
  bool leavesCorpse = false;
  uint32_t shotCooldownMs = 900u;
  uint32_t attackFlashMs = 140u;
  uint32_t pathRebuildMs = 280u;
  uint32_t respawnDelayMs = 0u;
  const char* aliveTextureName = nullptr;
  const char* deadTextureName = nullptr;
  const char* attackSampleId = nullptr;
  const char* dieSampleId = nullptr;

  void clear();
  void spawn(float x, float y);
  bool isAlive() const;
  bool hasCorpse() const;
  float getX() const;
  float getY() const;
  bool isAttacking(uint32_t nowMs) const;
  void kill();
  void applyDamage(int amount);
  void update(const WorldView& world, int& damageOut, const char*& attackSampleIdOut);
  bool tryHit(const AimView& aim, float& hitDistance) const;
  uint16_t texel(int texX, int texY, uint32_t nowMs) const;
  const WolfRender::ISprite& sprite() const { return renderSprite; }

  static void loadSpawns(Enemy* enemies, int& enemyCount, const char* layout);

private:
  float x = 0.0f;
  float y = 0.0f;
  int hp = 0;
  bool alive = false;
  bool corpse = false;
  uint32_t nextShotMs = 0;
  uint32_t attackUntilMs = 0;
  uint32_t respawnAtMs = 0;
  Navigation::PathState navigation{};
  WolfRender::Sprite renderSprite;

  static bool lineBlocked(const WorldView& world, float fromX, float fromY, float toX, float toY);
  static void buildSpriteTexture(const void* owner, uint16_t* outTexture, uint32_t nowMs);
  void tryAutoRespawn(const WorldView& world);
  bool blockedAt(const WorldView& world, float nextX, float nextY) const;
  bool tryMove(const WorldView& world, float moveX, float moveY);
};
