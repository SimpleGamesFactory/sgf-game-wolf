#include "Zombie.h"

#include <Arduino.h>
#include <math.h>
#include <string.h>

#include "Door.h"
#include "SGF/Color565.h"
#include "SGF/Math.h"
#include "Textures.h"

namespace {

constexpr const char* ZOMBIE_TEXTURE_NAME = "sprite_zombie";
constexpr const char* ZOMBIE_DEAD_TEXTURE_NAME = "sprite_zombie_dead";
constexpr const char* GHOST_TEXTURE_NAME = "sprite_ghost";

uint16_t applyAttackTint(uint16_t color565) {
  uint8_t r5 = (color565 >> 11) & 0x1F;
  uint8_t g6 = (color565 >> 5) & 0x3F;
  uint8_t b5 = color565 & 0x1F;
  uint8_t r = static_cast<uint8_t>((r5 * 255) / 31);
  uint8_t g = static_cast<uint8_t>((g6 * 255) / 63);
  uint8_t b = static_cast<uint8_t>((b5 * 255) / 31);

  r = static_cast<uint8_t>(Math::clamp(static_cast<int>(r) + 56, 0, 255));
  g = static_cast<uint8_t>((static_cast<int>(g) * 3) / 4);
  b = static_cast<uint8_t>((static_cast<int>(b) * 3) / 4);
  return Color565::rgb(r, g, b);
}

bool doorIsOpen(const Zombie::WorldView& world, int cellX, int cellY) {
  if (!world.doorOpenAmounts) {
    return false;
  }
  int index = cellY * world.mapStride + cellX;
  return Door::isPassable(world.doorOpenAmounts[index]);
}

bool wallAt(const Zombie::WorldView& world, int cellX, int cellY) {
  if (cellX < 0 || cellX >= world.mapWidth || cellY < 0 || cellY >= world.mapHeight) {
    return true;
  }
  uint8_t tile = world.map[cellY * world.mapStride + cellX];
  if (tile == 0) {
    return false;
  }
  if (Door::isTile(tile)) {
    return !doorIsOpen(world, cellX, cellY);
  }
  return true;
}

}  // namespace

void Zombie::clear() {
  x = 0.0f;
  y = 0.0f;
  hp = 0;
  alive = false;
  corpse = false;
  kind = Kind::Zombie;
  respawnMode = RespawnMode::None;
  nextShotMs = 0;
  attackUntilMs = 0;
  respawnAtMs = 0;
  renderSprite.clear();
}

void Zombie::spawn(float spawnX, float spawnY, Kind spawnKind, RespawnMode respawn) {
  x = spawnX;
  y = spawnY;
  kind = spawnKind;
  respawnMode = respawn;
  hp = maxHpForKind();
  alive = true;
  corpse = false;
  nextShotMs = 0;
  attackUntilMs = 0;
  respawnAtMs = 0;
  renderSprite.configure(x, y, TEX_SIZE, 3, 4, 8, 12.0f, this, buildSpriteTexture);
}

bool Zombie::isAlive() const {
  return alive;
}

bool Zombie::hasCorpse() const {
  return corpse;
}

bool Zombie::isGhost() const {
  return kind == Kind::Ghost;
}

float Zombie::getX() const {
  return x;
}

float Zombie::getY() const {
  return y;
}

bool Zombie::isAttacking(uint32_t nowMs) const {
  return alive && nowMs < attackUntilMs;
}

void Zombie::kill() {
  hp = 0;
  alive = false;
  corpse = (respawnMode == RespawnMode::None);
  respawnAtMs = (respawnMode == RespawnMode::Auto) ? (millis() + GHOST_RESPAWN_MS) : 0u;
  if (!corpse) {
    renderSprite.clear();
  }
}

void Zombie::applyDamage(int amount) {
  if (!alive || amount <= 0) {
    return;
  }
  hp -= amount;
  if (hp <= 0) {
    kill();
  }
}

void Zombie::update(const WorldView& world, int& damageOut, int& zombieShotsOut, int& ghostAttacksOut) {
  if (!alive) {
    tryAutoRespawn(world);
    return;
  }

  float dx = world.playerX - x;
  float dy = world.playerY - y;
  float distanceSq = dx * dx + dy * dy;
  if (distanceSq > DETECT_RANGE * DETECT_RANGE) {
    return;
  }

  float distance = sqrtf(distanceSq);
  float stopRange = isGhost() ? GHOST_STOP_RANGE : STOP_RANGE;
  if (distance > 0.001f && distance > stopRange) {
    float step = MOVE_SPEED * world.delta;
    float moveX = dx / distance * step;
    float moveY = dy / distance * step;

    float nextX = x + moveX;
    if (!blockedAt(world, nextX, y)) {
      x = nextX;
      renderSprite.setPosition(x, y);
    }

    float nextY = y + moveY;
    if (!blockedAt(world, x, nextY)) {
      y = nextY;
      renderSprite.setPosition(x, y);
    }
  }

  if (isGhost()) {
    if (distance <= GHOST_TOUCH_RANGE && world.nowMs >= nextShotMs) {
      damageOut += DAMAGE;
      ghostAttacksOut++;
      attackUntilMs = world.nowMs + ATTACK_FLASH_MS;
      nextShotMs = world.nowMs + SHOT_COOLDOWN_MS;
    }
  } else {
    if (distance <= SHOOT_RANGE &&
        world.nowMs >= nextShotMs &&
        !lineBlocked(world, x, y, world.playerX, world.playerY)) {
      damageOut += DAMAGE;
      zombieShotsOut++;
      attackUntilMs = world.nowMs + ATTACK_FLASH_MS;
      nextShotMs = world.nowMs + SHOT_COOLDOWN_MS;
    }
  }
}

bool Zombie::tryHit(const AimView& aim, float& hitDistance) const {
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

  if (lineBlocked(aim.world, aim.world.playerX, aim.world.playerY, x, y)) {
    return false;
  }

  hitDistance = forward;
  return true;
}

uint16_t Zombie::texel(int texX, int texY, uint32_t nowMs) const {
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

void Zombie::buildSpriteTexture(const void* owner, uint16_t* outTexture, uint32_t nowMs) {
  const Zombie& zombie = *static_cast<const Zombie*>(owner);
  const char* textureName = nullptr;
  if (zombie.alive) {
    textureName = zombie.kind == Kind::Ghost ? GHOST_TEXTURE_NAME : ZOMBIE_TEXTURE_NAME;
  } else {
    textureName = ZOMBIE_DEAD_TEXTURE_NAME;
  }
  const uint16_t* bmpTexture = Textures::pixels(textureName);
  if (bmpTexture != nullptr) {
    memcpy(outTexture, bmpTexture, TEX_SIZE * TEX_SIZE * sizeof(uint16_t));
    if (zombie.alive && zombie.isAttacking(nowMs)) {
      for (int i = 0; i < TEX_SIZE * TEX_SIZE; i++) {
        if (outTexture[i] != 0) {
          outTexture[i] = applyAttackTint(outTexture[i]);
        }
      }
    }
    return;
  }

  for (int texY = 0; texY < TEX_SIZE; texY++) {
    for (int texX = 0; texX < TEX_SIZE; texX++) {
      if (!zombie.alive) {
        bool body =
          (texY >= 8 && texY <= 11 && texX >= 2 && texX <= 13) ||
          (texY >= 11 && texY <= 13 && texX >= 4 && texX <= 11) ||
          (texY == 7 && texX >= 4 && texX <= 6);
        bool head = (texY >= 6 && texY <= 8 && texX >= 11 && texX <= 13);
        bool blood = (texY == 12 && texX >= 11 && texX <= 13);
        if (blood) {
          outTexture[texY * TEX_SIZE + texX] = Color565::rgb(132, 28, 20);
        } else if (head) {
          outTexture[texY * TEX_SIZE + texX] = Color565::rgb(118, 152, 98);
        } else if (body) {
          outTexture[texY * TEX_SIZE + texX] = Color565::rgb(74, 92, 122);
        } else {
          outTexture[texY * TEX_SIZE + texX] = 0;
        }
      } else {
        outTexture[texY * TEX_SIZE + texX] = zombie.texel(texX, texY, nowMs);
      }
    }
  }
}

void Zombie::loadSpawns(Zombie* zombies, int& zombieCount, const char* layout) {
  zombieCount = 0;
  for (int i = 0; i < MAX_COUNT; i++) {
    zombies[i].clear();
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
    if ((symbol == MAP_SYMBOL || symbol == GHOST_SYMBOL) && zombieCount < MAX_COUNT) {
      Zombie::Kind kind = (symbol == GHOST_SYMBOL) ? Kind::Ghost : Kind::Zombie;
      Zombie::RespawnMode respawn =
        (symbol == GHOST_SYMBOL) ? RespawnMode::Auto : RespawnMode::None;
      zombies[zombieCount].spawn(
        static_cast<float>(x) + 0.5f,
        static_cast<float>(y) + 0.5f,
        kind,
        respawn);
      zombieCount++;
    }
    x++;
  }
}

void Zombie::tryAutoRespawn(const WorldView& world) {
  if (respawnMode != RespawnMode::Auto || world.nowMs < respawnAtMs || world.map == nullptr) {
    return;
  }
  for (int attempt = 0; attempt < 64; ++attempt) {
    int cellX = random(0, world.mapWidth);
    int cellY = random(0, world.mapHeight);
    uint8_t tile = world.map[cellY * world.mapStride + cellX];
    if (tile != 0) {
      continue;
    }
    float spawnX = static_cast<float>(cellX) + 0.5f;
    float spawnY = static_cast<float>(cellY) + 0.5f;
    float dx = spawnX - world.playerX;
    float dy = spawnY - world.playerY;
    if ((dx * dx + dy * dy) < 4.0f) {
      continue;
    }
    spawn(spawnX, spawnY, kind, respawnMode);
    return;
  }
  respawnAtMs = world.nowMs + 250u;
}

int Zombie::maxHpForKind() const {
  return kind == Kind::Ghost ? GHOST_MAX_HP : ZOMBIE_MAX_HP;
}

bool Zombie::lineBlocked(
  const WorldView& world,
  float fromX,
  float fromY,
  float toX,
  float toY
) {
  float dx = toX - fromX;
  float dy = toY - fromY;
  float distance = sqrtf(dx * dx + dy * dy);
  if (distance <= 0.001f) {
    return false;
  }

  int steps = Math::clamp(static_cast<int>(distance * 12.0f), 1, 128);
  for (int i = 1; i < steps; i++) {
    float t = static_cast<float>(i) / static_cast<float>(steps);
    float sampleX = fromX + dx * t;
    float sampleY = fromY + dy * t;
    if (wallAt(world, static_cast<int>(sampleX), static_cast<int>(sampleY))) {
      return true;
    }
  }
  return false;
}

bool Zombie::blockedAt(const WorldView& world, float nextX, float nextY) const {
  return wallAt(world, static_cast<int>(nextX - RADIUS), static_cast<int>(nextY - RADIUS)) ||
         wallAt(world, static_cast<int>(nextX + RADIUS), static_cast<int>(nextY - RADIUS)) ||
         wallAt(world, static_cast<int>(nextX - RADIUS), static_cast<int>(nextY + RADIUS)) ||
         wallAt(world, static_cast<int>(nextX + RADIUS), static_cast<int>(nextY + RADIUS));
}
