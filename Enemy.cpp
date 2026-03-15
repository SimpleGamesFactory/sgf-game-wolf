#include "Enemy.h"

#include <Arduino.h>
#include <math.h>
#include <string.h>

#include "Navigation.h"
#include "SGF/Color565.h"
#include "EnemyCatalog.h"
#include "EnemyTextures.h"

void Enemy::clear() {
  x = 0.0f;
  y = 0.0f;
  hp = 0;
  alive = false;
  corpse = false;
  nextShotMs = 0;
  attackUntilMs = 0;
  respawnAtMs = 0;
  respawnMode = RespawnMode::None;
  attackMode = AttackMode::Ranged;
  radius = 0.22f;
  detectRange = 6.0f;
  stopRange = 0.0f;
  moveSpeed = 0.0f;
  attackRange = 0.0f;
  damage = 7;
  maxHp = 0;
  leavesCorpse = false;
  shotCooldownMs = 900u;
  attackFlashMs = 140u;
  pathRebuildMs = 280u;
  respawnDelayMs = 0u;
  aliveTextureName = nullptr;
  deadTextureName = nullptr;
  attackSampleId = nullptr;
  dieSampleId = nullptr;
  navigation.clear();
  renderSprite.clear();
}

void Enemy::spawn(float spawnX, float spawnY) {
  x = spawnX;
  y = spawnY;
  hp = maxHp;
  alive = true;
  corpse = false;
  nextShotMs = 0;
  attackUntilMs = 0;
  respawnAtMs = 0;
  navigation.clear();
  renderSprite.configure(x, y, TEX_SIZE, 3, 4, 8, 12.0f, this, buildSpriteTexture);
}

bool Enemy::isAlive() const {
  return alive;
}

bool Enemy::hasCorpse() const {
  return corpse;
}

float Enemy::getX() const {
  return x;
}

float Enemy::getY() const {
  return y;
}

bool Enemy::isAttacking(uint32_t nowMs) const {
  return alive && nowMs < attackUntilMs;
}

void Enemy::kill() {
  hp = 0;
  alive = false;
  corpse = leavesCorpse;
  respawnAtMs =
    (respawnMode == RespawnMode::Auto)
      ? (millis() + respawnDelayMs)
      : 0u;
  navigation.clear();
  if (!corpse) {
    renderSprite.clear();
  }
}

void Enemy::applyDamage(int amount) {
  if (!alive || amount <= 0) {
    return;
  }
  hp -= amount;
  if (hp <= 0) {
    kill();
  }
}

void Enemy::update(const WorldView& world, int& damageOut, const char*& attackSampleIdOut) {
  if (!alive) {
    tryAutoRespawn(world);
    return;
  }

  float dx = world.playerX - x;
  float dy = world.playerY - y;
  float distanceSq = dx * dx + dy * dy;
  if (distanceSq > detectRange * detectRange) {
    return;
  }

  float distance = sqrtf(distanceSq);
  if (distance > 0.001f && distance > stopRange) {
    float targetX = world.playerX;
    float targetY = world.playerY;
    if (attackMode == AttackMode::Ranged) {
      Navigation::TargetRequest navRequest{
        .world = world.grid,
        .actorX = x,
        .actorY = y,
        .goalX = world.playerX,
        .goalY = world.playerY,
        .nowMs = world.nowMs,
        .rebuildMs = pathRebuildMs,
      };
      Navigation::nextTarget(navRequest, navigation, targetX, targetY);
    }

    float navDx = targetX - x;
    float navDy = targetY - y;
    float navDistance = sqrtf(navDx * navDx + navDy * navDy);
    if (navDistance > 0.001f) {
      float step = moveSpeed * world.delta;
      float dirX = navDx / navDistance;
      float dirY = navDy / navDistance;
      if (!tryMove(world, dirX * step, dirY * step)) {
        float tangentX = -dirY * step;
        float tangentY = dirX * step;
        float leftDistSq =
          ((x + tangentX) - targetX) * ((x + tangentX) - targetX) +
          ((y + tangentY) - targetY) * ((y + tangentY) - targetY);
        float rightDistSq =
          ((x - tangentX) - targetX) * ((x - tangentX) - targetX) +
          ((y - tangentY) - targetY) * ((y - tangentY) - targetY);

        if (leftDistSq <= rightDistSq) {
          if (!tryMove(world, tangentX, tangentY)) {
            tryMove(world, -tangentX, -tangentY);
          }
        } else {
          if (!tryMove(world, -tangentX, -tangentY)) {
            tryMove(world, tangentX, tangentY);
          }
        }
      }
    }
  }

  bool canAttack = distance <= attackRange && world.nowMs >= nextShotMs;
  if (attackMode == AttackMode::Ranged) {
    canAttack = canAttack && !Navigation::lineBlocked(world.grid, x, y, world.playerX, world.playerY);
  }
  if (!canAttack) {
    return;
  }

  damageOut += damage;
  attackUntilMs = world.nowMs + attackFlashMs;
  nextShotMs = world.nowMs + shotCooldownMs;
  attackSampleIdOut = attackSampleId;
}

bool Enemy::tryHit(const AimView& aim, float& hitDistance) const {
  if (!alive) {
    return false;
  }

  float relX = x - aim.world.playerX;
  float relY = y - aim.world.playerY;
  float forward = relX * aim.dirX + relY * aim.dirY;
  if (forward <= 0.1f) {
    return false;
  }

  float side = fabsf(relX * aim.dirY - relY * aim.dirX);
  float hitRadius = 0.20f + forward * 0.08f;
  if (side > hitRadius) {
    return false;
  }

  if (Navigation::lineBlocked(
        aim.world.grid, aim.world.playerX, aim.world.playerY, x, y)) {
    return false;
  }

  hitDistance = forward;
  return true;
}

uint16_t Enemy::texel(int texX, int texY, uint32_t nowMs) const {
  bool attackFlash = isAttacking(nowMs);
  int centerX = TEX_SIZE / 2;
  int headX = texX - centerX;
  int headY = texY - 4;

  bool head = (headX * headX + headY * headY) <= 18;
  bool torso = (texX >= 4 && texX <= 11 && texY >= 7 && texY <= 13);
  bool leftArm = (texX >= 2 && texX <= 4 && texY >= 8 && texY <= 13);
  bool rightArm = (texX >= 11 && texX <= 13 && texY >= 8 && texY <= 13);
  bool leftLeg = (texX >= 5 && texX <= 7 && texY >= 13);
  bool rightLeg = (texX >= 8 && texX <= 10 && texY >= 13);
  bool eye = (texY == 4 && (texX == 5 || texX == 10));

  if (eye && head) {
    return attackFlash ? Color565::rgb(255, 92, 72) : Color565::rgb(28, 22, 22);
  }
  if (head) {
    return Color565::rgb(118, 152, 98);
  }
  if (torso) {
    return Color565::rgb(74, 92, 122);
  }
  if (leftArm || rightArm) {
    return Color565::rgb(94, 112, 138);
  }
  if (leftLeg || rightLeg) {
    return Color565::rgb(64, 56, 50);
  }
  return 0;
}

void Enemy::buildSpriteTexture(const void* owner, uint16_t* outTexture, uint32_t nowMs) {
  const Enemy& enemy = *static_cast<const Enemy*>(owner);
  EnemyTextures::build(enemy, outTexture, nowMs);
}

void Enemy::loadSpawns(Enemy* enemies, int& enemyCount, const char* layout) {
  enemyCount = 0;
  for (int i = 0; i < MAX_COUNT; i++) {
    enemies[i].clear();
  }

  int x = 0;
  int y = 0;
  for (const char* cursor = layout; *cursor != '\0'; cursor++) {
    char symbol = *cursor;
    if (symbol == '\r') {
      continue;
    }
    if (symbol == '\n') {
      if (x > 0) {
        y++;
      }
      x = 0;
      continue;
    }
    if (enemyCount < MAX_COUNT &&
        EnemyCatalog::spawnFromMapSymbol(
          enemies[enemyCount],
          symbol,
          x + 0.5f,
          y + 0.5f)) {
      enemyCount++;
    }
    x++;
  }
}

void Enemy::tryAutoRespawn(const WorldView& world) {
  if (respawnMode != RespawnMode::Auto ||
      world.nowMs < respawnAtMs ||
      world.grid.map == nullptr) {
    return;
  }
  for (int attempt = 0; attempt < 64; ++attempt) {
    int cellX = random(0, world.grid.mapWidth);
    int cellY = random(0, world.grid.mapHeight);
    uint8_t tile = world.grid.map[cellY * world.grid.mapStride + cellX];
    if (tile != 0) {
      continue;
    }
    float spawnX = cellX + 0.5f;
    float spawnY = cellY + 0.5f;
    float dx = spawnX - world.playerX;
    float dy = spawnY - world.playerY;
    if ((dx * dx + dy * dy) < 4.0f) {
      continue;
    }
    spawn(spawnX, spawnY);
    return;
  }
  respawnAtMs = world.nowMs + 250u;
}

bool Enemy::lineBlocked(
  const WorldView& world,
  float fromX,
  float fromY,
  float toX,
  float toY
) {
  return Navigation::lineBlocked(world.grid, fromX, fromY, toX, toY);
}

bool Enemy::blockedAt(const WorldView& world, float nextX, float nextY) const {
  return Navigation::isCellBlocked(
           world.grid,
           nextX - radius,
           nextY - radius) ||
         Navigation::isCellBlocked(
           world.grid,
           nextX + radius,
           nextY - radius) ||
         Navigation::isCellBlocked(
           world.grid,
           nextX - radius,
           nextY + radius) ||
         Navigation::isCellBlocked(
           world.grid,
           nextX + radius,
           nextY + radius);
}

bool Enemy::tryMove(const WorldView& world, float moveX, float moveY) {
  if (fabsf(moveX) <= 0.0001f && fabsf(moveY) <= 0.0001f) {
    return false;
  }

  float nextX = x + moveX;
  float nextY = y + moveY;
  if (!blockedAt(world, nextX, nextY)) {
    x = nextX;
    y = nextY;
    renderSprite.setPosition(x, y);
    return true;
  }

  bool preferX = fabsf(moveX) >= fabsf(moveY);
  if (preferX) {
    if (!blockedAt(world, nextX, y)) {
      x = nextX;
      renderSprite.setPosition(x, y);
      return true;
    }
    if (!blockedAt(world, x, nextY)) {
      y = nextY;
      renderSprite.setPosition(x, y);
      return true;
    }
  } else {
    if (!blockedAt(world, x, nextY)) {
      y = nextY;
      renderSprite.setPosition(x, y);
      return true;
    }
    if (!blockedAt(world, nextX, y)) {
      x = nextX;
      renderSprite.setPosition(x, y);
      return true;
    }
  }

  return false;
}
