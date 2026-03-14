#include "Game.h"

#include <Arduino.h>
#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#if defined(ARDUINO_ARCH_ESP32)
#include <esp_heap_caps.h>
#endif

#include "Door.h"
#include "FloorRenderer.h"
#include "Keys.h"
#include "Map.h"
#include "Minimap.h"
#include "Pickups.h"
#include "SGF/Color565.h"
#include "SGF/FontRenderer.h"
#include "SGF/IFillRect.h"
#include "SGF/Math.h"
#include "WeaponRenderer.h"
#include "Walls.h"

namespace {

class FrameBufferFillRect : public IFillRect {
public:
  FrameBufferFillRect(uint16_t* pixelsRef, int widthRef, int heightRef)
    : pixels(pixelsRef), width(widthRef), height(heightRef) {}

  void fillRect565(int x0, int y0, int w, int h, uint16_t color565) override {
    if (pixels == 0 || w <= 0 || h <= 0) {
      return;
    }

    for (int yy = y0; yy < y0 + h; yy++) {
      if (yy < 0 || yy >= height) {
        continue;
      }
      uint16_t* row = &pixels[yy * width];
      for (int xx = x0; xx < x0 + w; xx++) {
        if (xx < 0 || xx >= width) {
          continue;
        }
        row[xx] = color565;
      }
    }
  }

private:
  uint16_t* pixels = nullptr;
  int width = 0;
  int height = 0;
};

uint16_t applyDamageFlash(uint16_t color565, uint8_t strength) {
  if (strength == 0) {
    return color565;
  }

  uint8_t r5 = (color565 >> 11) & 0x1F;
  uint8_t g6 = (color565 >> 5) & 0x3F;
  uint8_t b5 = color565 & 0x1F;

  r5 = static_cast<uint8_t>(r5 + (((31u - r5) * strength) >> 8));
  g6 = static_cast<uint8_t>((g6 * (255u - (strength >> 1))) >> 8);
  b5 = static_cast<uint8_t>((b5 * (255u - ((strength * 3u) >> 2))) >> 8);

  return static_cast<uint16_t>((r5 << 11) | (g6 << 5) | b5);
}

uint16_t applyShotFlash(uint16_t color565, uint8_t strength) {
  if (strength == 0) {
    return color565;
  }

  uint8_t r5 = (color565 >> 11) & 0x1F;
  uint8_t g6 = (color565 >> 5) & 0x3F;
  uint8_t b5 = color565 & 0x1F;

  r5 = static_cast<uint8_t>(r5 + (((31u - r5) * strength) >> 8));
  g6 = static_cast<uint8_t>(g6 + (((63u - g6) * strength) >> 9));
  b5 = static_cast<uint8_t>(b5 + (((31u - b5) * strength) >> 10));

  return static_cast<uint16_t>((r5 << 11) | (g6 << 5) | b5);
}

uint8_t damageFlashStrength(uint32_t nowMs, uint32_t damageFlashUntilMs, uint32_t durationMs) {
  if (damageFlashUntilMs == 0 || nowMs >= damageFlashUntilMs || durationMs == 0) {
    return 0;
  }

  uint32_t remainingMs = damageFlashUntilMs - nowMs;
  uint32_t strength = (remainingMs * 224u) / durationMs;
  if (strength > 224u) {
    strength = 224u;
  }
  return static_cast<uint8_t>(strength);
}

uint8_t shotFlashStrength(uint32_t nowMs, uint32_t shotUntilMs, uint32_t durationMs) {
  if (shotUntilMs == 0 || nowMs >= shotUntilMs || durationMs == 0) {
    return 0;
  }

  uint32_t activeStartMs = shotUntilMs > durationMs ? (shotUntilMs - durationMs) : 0;
  if (nowMs < activeStartMs) {
    return 0;
  }

  uint32_t remainingMs = shotUntilMs - nowMs;
  uint32_t strength = (remainingMs * 112u) / durationMs;
  if (strength > 112u) {
    strength = 112u;
  }
  return static_cast<uint8_t>(strength);
}

}  // namespace

Wolf3DGame::Wolf3DGame(
  IRenderTarget& renderTargetRef,
  IScreen& screenRef,
  const SGFHardware::HardwareProfile& hardwareProfileIn
)
  : Game(FRAME_DEFAULT_STEP_US, FRAME_MAX_STEP_US),
    renderTarget(renderTargetRef),
    screen(screenRef),
    hardwareProfile(hardwareProfileIn) {
  pinLeft = hardwareProfile.input.left;
  pinRight = hardwareProfile.input.right;
  pinUp = hardwareProfile.input.up;
  pinDown = hardwareProfile.input.down;
  pinFire = hardwareProfile.input.fire;
}

void Wolf3DGame::setup() {
  profiler.begin();
  start();
}

void Wolf3DGame::onSetup() {
  Vector2i targetSize = renderTarget.size();
  screenW = targetSize.x;
  screenH = targetSize.y;
  worldScreenH = screenH - HUD_H;
  if (screenW != MAX_SCREEN_W || screenH != MAX_SCREEN_H || worldScreenH != WORLD_AREA_H) {
    while (true) {
      delay(1000);
    }
  }
#if WOLF_HEAP_COLD_BUFFERS
  if (map == nullptr) {
    const size_t mapBytes = sizeof(uint8_t) * MAP_MAX_W * MAP_MAX_H;
#if defined(ARDUINO_ARCH_ESP32)
    map = static_cast<uint8_t (*)[MAP_MAX_W]>(
      heap_caps_malloc(mapBytes, MALLOC_CAP_INTERNAL | MALLOC_CAP_8BIT));
#else
    map = static_cast<uint8_t (*)[MAP_MAX_W]>(malloc(mapBytes));
#endif
    if (map == nullptr) {
      while (true) {
        delay(1000);
      }
    }
  }
  if (doorOpenAmounts == nullptr) {
    const size_t doorBytes = sizeof(uint8_t) * MAP_MAX_W * MAP_MAX_H;
#if defined(ARDUINO_ARCH_ESP32)
    doorOpenAmounts = static_cast<uint8_t*>(
      heap_caps_malloc(doorBytes, MALLOC_CAP_INTERNAL | MALLOC_CAP_8BIT));
#else
    doorOpenAmounts = static_cast<uint8_t*>(malloc(doorBytes));
#endif
    if (doorOpenAmounts == nullptr) {
      while (true) {
        delay(1000);
      }
    }
  }
#endif
#if WOLF_DYNAMIC_FRAMEBUFFER
  const size_t frameBytes = sizeof(uint16_t) * RENDER_W * RENDER_H;
#if defined(ARDUINO_ARCH_ESP32)
  frameBuffer = static_cast<uint16_t*>(heap_caps_aligned_alloc(
    DYNAMIC_FRAMEBUFFER_ALIGNMENT,
    frameBytes,
    MALLOC_CAP_INTERNAL | MALLOC_CAP_8BIT | MALLOC_CAP_CACHE_ALIGNED));
#else
  frameBuffer = static_cast<uint16_t*>(malloc(frameBytes));
#endif
  if (frameBuffer == nullptr) {
    while (true) {
      delay(1000);
    }
  }
#endif
  const uint16_t viewportBg = Color565::rgb(4, 5, 8);
  if (VIEWPORT_Y > 0) {
    screen.fillRect565(0, 0, screenW, VIEWPORT_Y, viewportBg);
    screen.fillRect565(0, VIEWPORT_Y + VIEWPORT_H, screenW, worldScreenH - (VIEWPORT_Y + VIEWPORT_H), viewportBg);
  }
  if (VIEWPORT_X > 0) {
    screen.fillRect565(0, VIEWPORT_Y, VIEWPORT_X, VIEWPORT_H, viewportBg);
    screen.fillRect565(VIEWPORT_X + VIEWPORT_W, VIEWPORT_Y, screenW - (VIEWPORT_X + VIEWPORT_W), VIEWPORT_H, viewportBg);
  }

  leftPinInput.attach(pinLeft, true);
  rightPinInput.attach(pinRight, true);
  upPinInput.attach(pinUp, true);
  downPinInput.attach(pinDown, true);
  firePinInput.attach(pinFire, true);
  leftAction.sync(leftPinInput.isActive());
  rightAction.sync(rightPinInput.isActive());
  upAction.sync(upPinInput.isActive());
  downAction.sync(downPinInput.isActive());
  fireAction.sync(firePinInput.isActive());

  ammo = START_AMMO;
  lives = START_LIVES;
  energy = START_ENERGY;
  keysOwned = 0;
  resetMap();
  resetPlayerPose();
  nextBlinkMs = millis() + 1300;
  blinkUntilMs = 0;
  shotUntilMs = 0;
  hurtUntilMs = 0;
  damageFlashUntilMs = 0;
  nextDamageMs = 0;
  displayedFps = 0;
  fpsSampleFrames = 0;
  fpsSampleStartMs = millis();

  hud.begin(screenW, worldScreenH);
  hud.setLives(lives);
  hud.setAmmo(ammo);
  hud.setEnergy(energy);
  hud.setKeys(keysOwned);
  hud.setFaceMood(currentFaceMood(millis()));
  collectPickupUnderPlayer();
  hud.invalidate();

  renderFrame();
  presentFrame();
  hud.render();
  hud.flush(renderTarget);
}

void Wolf3DGame::onPhysics(float delta) {
  uint32_t physicsStartUs = micros();
  leftPinInput.update();
  rightPinInput.update();
  upPinInput.update();
  downPinInput.update();
  firePinInput.update();
  leftAction.update(leftPinInput.isActive());
  rightAction.update(rightPinInput.isActive());
  upAction.update(upPinInput.isActive());
  downAction.update(downPinInput.isActive());
  fireAction.update(firePinInput.isActive());

  uint32_t sectionStartUs = micros();
  updateInput(delta);
  profiler.add(WolfProfiler::Slot::Input, micros() - sectionStartUs);

  updateDoors(delta);

  sectionStartUs = micros();
  updateZombies(delta);
  profiler.add(WolfProfiler::Slot::ZombieUpdate, micros() - sectionStartUs);

  sectionStartUs = micros();
  updateHudAnimation();
  profiler.add(WolfProfiler::Slot::HudAnim, micros() - sectionStartUs);
  profiler.add(WolfProfiler::Slot::Physics, micros() - physicsStartUs);
}

void Wolf3DGame::onProcess(float delta) {
  (void)delta;
  uint32_t nowMs = millis();
  updateFpsCounter(nowMs);
  renderFrame();
  uint32_t sectionStartUs = micros();
  presentFrame();
  profiler.add(WolfProfiler::Slot::Present, micros() - sectionStartUs);

  sectionStartUs = micros();
  hud.render();
  profiler.add(WolfProfiler::Slot::HudRender, micros() - sectionStartUs);

  sectionStartUs = micros();
  hud.flush(renderTarget);
  profiler.add(WolfProfiler::Slot::HudFlush, micros() - sectionStartUs);

  frameCounter++;
  profiler.frame();
}

void Wolf3DGame::resetMap() {
  Map::load(map, doorOpenBits, mapWidth, mapHeight, spawn);
  memset(doorOpenAmounts, 0, sizeof(uint8_t) * MAP_MAX_W * MAP_MAX_H);
  memset(doorUnlockedBits, 0, sizeof(doorUnlockedBits));
  Zombie::loadSpawns(zombies, zombieCount, MapLayout::E1M1);
}

void Wolf3DGame::resetPlayerPose() {
  playerX = spawn.x;
  playerY = spawn.y;
  dirX = spawn.dirX;
  dirY = spawn.dirY;
  planeX = 0.0f;
  planeY = CAMERA_PLANE_SCALE;
  if (dirX == 1.0f && dirY == 0.0f) {
    planeY = CAMERA_PLANE_SCALE;
  } else if (dirX == -1.0f && dirY == 0.0f) {
    planeY = -CAMERA_PLANE_SCALE;
  } else if (dirX == 0.0f && dirY == -1.0f) {
    planeX = CAMERA_PLANE_SCALE;
    planeY = 0.0f;
  } else if (dirX == 0.0f && dirY == 1.0f) {
    planeX = -CAMERA_PLANE_SCALE;
    planeY = 0.0f;
  }
}

Zombie::WorldView Wolf3DGame::makeZombieWorldView(uint32_t nowMs, float delta) const {
  Zombie::WorldView world;
  world.map = &map[0][0];
  world.doorOpenAmounts = doorOpenAmounts;
  world.mapStride = MAP_MAX_W;
  world.mapWidth = mapWidth;
  world.mapHeight = mapHeight;
  world.playerX = playerX;
  world.playerY = playerY;
  world.nowMs = nowMs;
  world.delta = delta;
  return world;
}

bool Wolf3DGame::hasKey(KeyColor color) const {
  if (color == KeyColor::None) {
    return true;
  }
  return (keysOwned & Keys::bitFor(color)) != 0;
}

uint8_t Wolf3DGame::doorOpenAmountAt(int cellX, int cellY) const {
  if (cellX < 0 || cellX >= MAP_MAX_W || cellY < 0 || cellY >= MAP_MAX_H || doorOpenAmounts == nullptr) {
    return Door::OPEN_AMOUNT_CLOSED;
  }
  return doorOpenAmounts[cellY * MAP_MAX_W + cellX];
}

bool Wolf3DGame::isDoorOpen(int cellX, int cellY) const {
  if (cellX < 0 || cellX >= MAP_MAX_W || cellY < 0 || cellY >= MAP_MAX_H) {
    return false;
  }
  int index = cellY * MAP_MAX_W + cellX;
  return (doorOpenBits[index >> 3] & (1u << (index & 7))) != 0;
}

bool Wolf3DGame::isDoorUnlocked(int cellX, int cellY) const {
  if (cellX < 0 || cellX >= MAP_MAX_W || cellY < 0 || cellY >= MAP_MAX_H) {
    return false;
  }
  int index = cellY * MAP_MAX_W + cellX;
  return (doorUnlockedBits[index >> 3] & (1u << (index & 7))) != 0;
}

void Wolf3DGame::setDoorOpen(int cellX, int cellY, bool open) {
  if (cellX < 0 || cellX >= MAP_MAX_W || cellY < 0 || cellY >= MAP_MAX_H) {
    return;
  }
  int index = cellY * MAP_MAX_W + cellX;
  uint8_t mask = static_cast<uint8_t>(1u << (index & 7));
  if (open) {
    doorOpenBits[index >> 3] |= mask;
  } else {
    doorOpenBits[index >> 3] &= static_cast<uint8_t>(~mask);
  }
}

void Wolf3DGame::setDoorUnlocked(int cellX, int cellY, bool unlocked) {
  if (cellX < 0 || cellX >= MAP_MAX_W || cellY < 0 || cellY >= MAP_MAX_H) {
    return;
  }
  int index = cellY * MAP_MAX_W + cellX;
  uint8_t mask = static_cast<uint8_t>(1u << (index & 7));
  if (unlocked) {
    doorUnlockedBits[index >> 3] |= mask;
  } else {
    doorUnlockedBits[index >> 3] &= static_cast<uint8_t>(~mask);
  }
}

bool Wolf3DGame::wallAt(int cellX, int cellY) const {
  if (cellX < 0 || cellX >= mapWidth || cellY < 0 || cellY >= mapHeight) {
    return true;
  }
  uint8_t tile = map[cellY][cellX];
  if (tile == 0) {
    return false;
  }
  if (Pickups::isPickup(tile)) {
    return false;
  }
  if (Door::isTile(tile)) {
    return !Door::isPassable(doorOpenAmountAt(cellX, cellY));
  }
  return true;
}

void Wolf3DGame::updateDoors(float delta) {
  if (doorOpenAmounts == nullptr) {
    return;
  }
  int step = static_cast<int>(Door::OPEN_SPEED_PER_SEC * 255.0f * delta);
  if (step < 1) {
    step = 1;
  }
  for (int y = 0; y < mapHeight; y++) {
    for (int x = 0; x < mapWidth; x++) {
      if (!Door::isTile(map[y][x])) {
        continue;
      }
      int index = y * MAP_MAX_W + x;
      int target = isDoorOpen(x, y) ? Door::OPEN_AMOUNT_OPEN : Door::OPEN_AMOUNT_CLOSED;
      int current = doorOpenAmounts[index];
      if (current < target) {
        current += step;
        if (current > target) {
          current = target;
        }
      } else if (current > target) {
        current -= step;
        if (current < target) {
          current = target;
        }
      }
      doorOpenAmounts[index] = static_cast<uint8_t>(current);
    }
  }
}

bool Wolf3DGame::blockedAt(float testX, float testY) const {
  const float r = PLAYER_RADIUS;
  return
    wallAt(static_cast<int>(testX - r), static_cast<int>(testY - r)) ||
    wallAt(static_cast<int>(testX + r), static_cast<int>(testY - r)) ||
    wallAt(static_cast<int>(testX - r), static_cast<int>(testY + r)) ||
    wallAt(static_cast<int>(testX + r), static_cast<int>(testY + r));
}

bool Wolf3DGame::attemptMove(float nextX, float nextY) {
  bool moved = false;
  if (!blockedAt(nextX, playerY)) {
    playerX = nextX;
    moved = true;
  }
  if (!blockedAt(playerX, nextY)) {
    playerY = nextY;
    moved = true;
  }
  return moved;
}

void Wolf3DGame::onBlockedMove() {
  uint32_t nowMs = millis();
  if (nowMs < nextDamageMs) {
    return;
  }
  nextDamageMs = nowMs + BUMP_DAMAGE_COOLDOWN_MS;
  applyDamage(DAMAGE_ON_BUMP);
}

void Wolf3DGame::updateHeadBob(float delta, bool moved) {
  if (moved) {
    headBobPhase += delta * HEAD_BOB_SPEED;
    if (headBobPhase > 6.2831853f) {
      headBobPhase = fmodf(headBobPhase, 6.2831853f);
    }
    headBobOffset = sinf(headBobPhase) * HEAD_BOB_AMPLITUDE;
    return;
  }

  float settle = Math::clamp(delta * HEAD_BOB_SETTLE, 0.0f, 1.0f);
  headBobOffset += (0.0f - headBobOffset) * settle;
  if (fabsf(headBobOffset) < 0.05f) {
    headBobOffset = 0.0f;
    headBobPhase = 0.0f;
  }
}

int Wolf3DGame::bobOffsetY() const {
  return static_cast<int>(roundf(headBobOffset));
}

void Wolf3DGame::rotate(float angle) {
  float oldDirX = dirX;
  float cosA = cosf(angle);
  float sinA = sinf(angle);
  dirX = dirX * cosA - dirY * sinA;
  dirY = oldDirX * sinA + dirY * cosA;

  float oldPlaneX = planeX;
  planeX = planeX * cosA - planeY * sinA;
  planeY = oldPlaneX * sinA + planeY * cosA;
}

bool Wolf3DGame::toggleDoorAhead() {
  int cellX = static_cast<int>(playerX + dirX * DOOR_REACH);
  int cellY = static_cast<int>(playerY + dirY * DOOR_REACH);
  if (cellX < 0 || cellX >= mapWidth || cellY < 0 || cellY >= mapHeight) {
    return false;
  }
  if (!Door::isTile(map[cellY][cellX])) {
    return false;
  }

  if (isDoorOpen(cellX, cellY)) {
    if (canCloseDoor(cellX, cellY)) {
      setDoorOpen(cellX, cellY, false);
    }
  } else {
    KeyColor required = Door::requiredKey(map[cellY][cellX]);
    if (required != KeyColor::None && !isDoorUnlocked(cellX, cellY) && !hasKey(required)) {
      return true;
    }
    if (required != KeyColor::None && !isDoorUnlocked(cellX, cellY)) {
      keysOwned &= static_cast<uint8_t>(~Keys::bitFor(required));
      hud.setKeys(keysOwned);
      setDoorUnlocked(cellX, cellY, true);
    }
    setDoorOpen(cellX, cellY, true);
    audio.playDoorOpen();
  }
  return true;
}

bool Wolf3DGame::canCloseDoor(int cellX, int cellY) const {
  int playerCellX = static_cast<int>(playerX);
  int playerCellY = static_cast<int>(playerY);
  return playerCellX != cellX || playerCellY != cellY;
}

void Wolf3DGame::collectPickupUnderPlayer() {
  int cellX = static_cast<int>(playerX);
  int cellY = static_cast<int>(playerY);
  if (cellX < 0 || cellX >= mapWidth || cellY < 0 || cellY >= mapHeight) {
    return;
  }

  uint8_t tile = map[cellY][cellX];
  if (!Pickups::isPickup(tile)) {
    return;
  }
  bool consumed = false;
  if (Pickups::isKey(tile)) {
    keysOwned |= Keys::bitFor(Keys::colorForPickup(tile));
    hud.setKeys(keysOwned);
    consumed = true;
  } else if (Pickups::isAmmo(tile)) {
    if (ammo < Pickups::MAX_AMMO) {
      ammo += Pickups::AMMO_PICKUP_AMOUNT;
      if (ammo > Pickups::MAX_AMMO) {
        ammo = Pickups::MAX_AMMO;
      }
      hud.setAmmo(ammo);
      consumed = true;
    }
  } else if (Pickups::isMedkit(tile)) {
    if (energy < START_ENERGY) {
      energy += Pickups::MEDKIT_PICKUP_AMOUNT;
      if (energy > START_ENERGY) {
        energy = START_ENERGY;
      }
      hud.setEnergy(energy);
      consumed = true;
    }
  }
  if (consumed) {
    map[cellY][cellX] = 0;
    audio.playPickup();
  }
}

int Wolf3DGame::shotDamage(float hitDistance) const {
  float distance = Math::clamp(hitDistance, 0.0f, 8.0f);
  float baseDamage = 10.0f - distance * 0.9f;
  int damage = static_cast<int>(roundf(baseDamage)) + random(-2, 3);
  return Math::clamp(damage, 1, 12);
}

void Wolf3DGame::shoot() {
  if (ammo <= 0) {
    return;
  }

  uint32_t nowMs = millis();
  ammo--;
  shotUntilMs = nowMs + FACE_SHOOT_MS;
  hud.setAmmo(ammo);
  hud.setFaceMood(currentFaceMood(nowMs));
  audio.playFire();

  Zombie::WorldView world = makeZombieWorldView(nowMs, 0.0f);
  Zombie::AimView aim{world, dirX, dirY};
  int targetIndex = -1;
  float bestDistance = 1e30f;
  for (int i = 0; i < zombieCount; i++) {
    float hitDistance = 0.0f;
    if (zombies[i].tryHit(aim, hitDistance) && hitDistance < bestDistance) {
      bestDistance = hitDistance;
      targetIndex = i;
    }
  }
  if (targetIndex >= 0) {
    zombies[targetIndex].applyDamage(shotDamage(bestDistance));
  }
}

void Wolf3DGame::applyDamage(int amount) {
  if (amount <= 0) {
    return;
  }

  uint32_t nowMs = millis();
  audio.playHit();
  energy -= amount;
  hurtUntilMs = nowMs + FACE_HURT_MS;
  damageFlashUntilMs = nowMs + DAMAGE_FLASH_MS;
  if (energy > 0) {
    hud.setEnergy(energy);
    hud.setFaceMood(currentFaceMood(nowMs));
    return;
  }

  lives--;
  if (lives <= 0) {
    lives = START_LIVES;
    ammo = START_AMMO;
  }
  energy = START_ENERGY;
  resetPlayerPose();

  hud.setLives(lives);
  hud.setAmmo(ammo);
  hud.setEnergy(energy);
  hud.setFaceMood(currentFaceMood(nowMs));
}

void Wolf3DGame::updateInput(float delta) {
  bool minimapShortcut = leftAction.isPressed() && rightAction.isPressed();
  bool strafing = fireAction.isPressed() && (leftAction.isPressed() || rightAction.isPressed());
  bool moved = false;
  uint32_t nowMs = millis();
  if (minimapShortcut) {
    if (!minimapShortcutHeld) {
      minimapShortcutHeld = true;
      minimapShortcutTriggered = false;
      minimapShortcutStartMs = nowMs;
    } else if (!minimapShortcutTriggered &&
               nowMs - minimapShortcutStartMs >= MINIMAP_HOLD_MS) {
      showMinimap = !showMinimap;
      minimapShortcutTriggered = true;
    }
  } else {
    minimapShortcutHeld = false;
    minimapShortcutTriggered = false;
  }

  if (fireAction.isJustPressed() && !strafing && !minimapShortcut) {
    if (!toggleDoorAhead()) {
      shoot();
    }
  }

  float moveStep = MOVE_SPEED * delta;
  float strafeStep = STRAFE_SPEED * delta;
  float turnStep = TURN_SPEED * delta;

  if (upAction.isPressed() && !fireAction.isPressed()) {
    if (!attemptMove(playerX + dirX * moveStep, playerY + dirY * moveStep)) {
      onBlockedMove();
    } else {
      moved = true;
    }
  }
  if (downAction.isPressed()) {
    if (!attemptMove(playerX - dirX * moveStep, playerY - dirY * moveStep)) {
      onBlockedMove();
    } else {
      moved = true;
    }
  }

  if (fireAction.isPressed()) {
    float sideX = -dirY;
    float sideY = dirX;
    if (leftAction.isPressed()) {
      if (!attemptMove(playerX - sideX * strafeStep, playerY - sideY * strafeStep)) {
        onBlockedMove();
      } else {
        moved = true;
      }
    }
    if (rightAction.isPressed()) {
      if (!attemptMove(playerX + sideX * strafeStep, playerY + sideY * strafeStep)) {
        onBlockedMove();
      } else {
        moved = true;
      }
    }
  } else {
    if (leftAction.isPressed()) {
      rotate(-turnStep);
    }
    if (rightAction.isPressed()) {
      rotate(turnStep);
    }
  }

  updateHeadBob(delta, moved);
  collectPickupUnderPlayer();
}

void Wolf3DGame::updateZombies(float delta) {
  Zombie::WorldView world = makeZombieWorldView(millis(), delta);
  int damage = 0;
  for (int i = 0; i < zombieCount; i++) {
    zombies[i].update(world, damage);
  }
  if (damage > 0) {
    applyDamage(damage);
  }
}

Hud::FaceMood Wolf3DGame::currentFaceMood(uint32_t nowMs) const {
  if (nowMs < hurtUntilMs) {
    return Hud::FaceMood::Hurt;
  }
  if (energy <= 25) {
    return Hud::FaceMood::Low;
  }
  if (nowMs < shotUntilMs) {
    return Hud::FaceMood::Shoot;
  }
  if (nowMs < blinkUntilMs) {
    return Hud::FaceMood::Blink;
  }
  return Hud::FaceMood::Normal;
}

void Wolf3DGame::updateHudAnimation() {
  uint32_t nowMs = millis();
  if (nowMs >= nextBlinkMs && nowMs >= blinkUntilMs) {
    blinkUntilMs = nowMs + FACE_BLINK_MS;
    nextBlinkMs = nowMs + 1800u + ((frameCounter * 97u) % 700u);
  }
  hud.setFaceMood(currentFaceMood(nowMs));
}

void Wolf3DGame::updateFpsCounter(uint32_t nowMs) {
  if (fpsSampleStartMs == 0) {
    fpsSampleStartMs = nowMs;
  }

  fpsSampleFrames++;
  uint32_t elapsedMs = nowMs - fpsSampleStartMs;
  if (elapsedMs < 250u) {
    return;
  }

  displayedFps = static_cast<uint16_t>((fpsSampleFrames * 1000u + (elapsedMs / 2u)) / elapsedMs);
  fpsSampleFrames = 0;
  fpsSampleStartMs = nowMs;
}

void Wolf3DGame::renderFrame() {
  clearFrame();
  uint32_t sectionStartUs = micros();
  renderFloor();
  profiler.add(WolfProfiler::Slot::Floor, micros() - sectionStartUs);

  sectionStartUs = micros();
  renderWorld();
  profiler.add(WolfProfiler::Slot::World, micros() - sectionStartUs);

  uint32_t nowMs = millis();
  sectionStartUs = micros();
  renderSprites(nowMs);
  profiler.add(WolfProfiler::Slot::Sprites, micros() - sectionStartUs);

  sectionStartUs = micros();
  renderWeapon(nowMs);
  profiler.add(WolfProfiler::Slot::Weapon, micros() - sectionStartUs);

  if (showMinimap) {
    sectionStartUs = micros();
    Minimap::render(
      frameBuffer,
      RENDER_W,
      RENDER_H,
      &map[0][0],
      doorOpenAmounts,
      MAP_MAX_W,
      mapWidth,
      mapHeight,
      playerX,
      playerY,
      dirX,
      dirY);
    profiler.add(WolfProfiler::Slot::Minimap, micros() - sectionStartUs);
  }
  sectionStartUs = micros();
  renderFpsCounter();
  profiler.add(WolfProfiler::Slot::FpsOverlay, micros() - sectionStartUs);
}

void Wolf3DGame::presentFrame() {
  if (!LCD_TRANSFER) {
    return;
  }

  uint16_t* fb = frameBuffer;
  uint32_t nowMs = millis();
  uint8_t hitFlashStrength = damageFlashStrength(nowMs, damageFlashUntilMs, DAMAGE_FLASH_MS);
  uint8_t muzzleFlashStrength = shotFlashStrength(nowMs, shotUntilMs, WEAPON_FLASH_MS);
  const bool streamViewport = renderTarget.supportsBlit565Stream();
  const bool preSwappedStreamViewport =
    streamViewport && renderTarget.supportsPreSwappedBlit565Stream();
  const bool queuedPreSwappedStreamViewport =
    preSwappedStreamViewport &&
    UPSCALE_BUFFER_COUNT > 1 &&
    renderTarget.supportsQueuedPreSwappedBlit565Stream();
  int queuedChunks = 0;
  int chunkIndex = 0;
  if (streamViewport) {
    renderTarget.beginBlit565Stream(VIEWPORT_X, VIEWPORT_Y, VIEWPORT_W, VIEWPORT_H);
  }
  for (int srcY0 = 0; srcY0 < RENDER_H; srcY0 += UPSCALE_CHUNK_SRC_ROWS) {
    if (queuedPreSwappedStreamViewport && queuedChunks >= UPSCALE_BUFFER_COUNT) {
      renderTarget.waitQueuedPreSwappedBlit565StreamSlot();
      queuedChunks--;
    }
    int srcRows = Math::clamp(UPSCALE_CHUNK_SRC_ROWS, 1, RENDER_H - srcY0);
    int dstRows = srcRows * UPSCALE;
    uint16_t* activeUpscaleBuffer = upscaleBuffers[chunkIndex % UPSCALE_BUFFER_COUNT];
    chunkIndex++;
    for (int localY = 0; localY < srcRows; localY++) {
      const uint16_t* src = &fb[(srcY0 + localY) * RENDER_W];
      if (hitFlashStrength == 0 && muzzleFlashStrength == 0) {
        for (int x = 0; x < RENDER_W; x++) {
          uint16_t color565 = src[x];
          if (preSwappedStreamViewport) {
            color565 = Color565::bswap(color565);
          }
          int dstX = x * UPSCALE;
          for (int copyY = 0; copyY < UPSCALE; copyY++) {
            uint16_t* dstRow =
              &activeUpscaleBuffer[(localY * UPSCALE + copyY) * VIEWPORT_W];
            for (int copyX = 0; copyX < UPSCALE; copyX++) {
              dstRow[dstX + copyX] = color565;
            }
          }
        }
      } else {
        for (int x = 0; x < RENDER_W; x++) {
          uint16_t color565 = src[x];
          if (muzzleFlashStrength > 0) {
            color565 = applyShotFlash(color565, muzzleFlashStrength);
          }
          if (hitFlashStrength > 0) {
            color565 = applyDamageFlash(color565, hitFlashStrength);
          }
          if (preSwappedStreamViewport) {
            color565 = Color565::bswap(color565);
          }
          int dstX = x * UPSCALE;
          for (int copyY = 0; copyY < UPSCALE; copyY++) {
            uint16_t* dstRow =
              &activeUpscaleBuffer[(localY * UPSCALE + copyY) * VIEWPORT_W];
            for (int copyX = 0; copyX < UPSCALE; copyX++) {
              dstRow[dstX + copyX] = color565;
            }
          }
        }
      }
    }
    if (streamViewport) {
      if (queuedPreSwappedStreamViewport) {
        renderTarget.queuePreSwappedBlit565StreamChunk(
          activeUpscaleBuffer,
          static_cast<size_t>(VIEWPORT_W * dstRows));
        queuedChunks++;
      } else if (preSwappedStreamViewport) {
        renderTarget.writePreSwappedBlit565StreamChunk(
          activeUpscaleBuffer,
          static_cast<size_t>(VIEWPORT_W * dstRows));
      } else {
        renderTarget.writeBlit565StreamChunk(activeUpscaleBuffer, static_cast<size_t>(VIEWPORT_W * dstRows));
      }
    } else {
      renderTarget.blit565(VIEWPORT_X, VIEWPORT_Y + srcY0 * UPSCALE, VIEWPORT_W, dstRows, activeUpscaleBuffer);
    }
  }
  if (streamViewport) {
    renderTarget.endBlit565Stream();
  }
}

void Wolf3DGame::clearFrame() {
  uint16_t* fb = frameBuffer;
  int horizon = Math::clamp((RENDER_H / 2) + bobOffsetY(), 1, RENDER_H - 1);
  for (int y = 0; y < RENDER_H; y++) {
    bool isSky = y < horizon;
    float bandT;
    if (isSky) {
      bandT = static_cast<float>(y) / static_cast<float>(horizon);
    } else {
      bandT = static_cast<float>(y - horizon) /
              static_cast<float>(RENDER_H - horizon);
    }

    uint16_t color565;
    if (isSky) {
      uint8_t r = static_cast<uint8_t>(18.0f + bandT * 26.0f);
      uint8_t g = static_cast<uint8_t>(28.0f + bandT * 40.0f);
      uint8_t b = static_cast<uint8_t>(60.0f + bandT * 52.0f);
      color565 = Color565::rgb(r, g, b);
    } else if (SIMPLE_FLOOR) {
      uint8_t c = static_cast<uint8_t>(72.0f - bandT * 34.0f);
      color565 = Color565::rgb(c, c, c + 4);
    } else {
      uint8_t r = static_cast<uint8_t>(28.0f - bandT * 12.0f);
      uint8_t g = static_cast<uint8_t>(20.0f - bandT * 8.0f);
      uint8_t b = static_cast<uint8_t>(18.0f - bandT * 6.0f);
      color565 = Color565::rgb(r, g, b);
    }

    uint16_t* row = &fb[y * RENDER_W];
    for (int x = 0; x < RENDER_W; x++) {
      row[x] = color565;
    }
  }
}

void Wolf3DGame::renderFloor() {
  if (SIMPLE_FLOOR) {
    return;
  }

  FloorRenderer::render(
    frameBuffer,
    RENDER_W,
    RENDER_H,
    playerX,
    playerY,
    dirX,
    dirY,
    planeX,
    planeY);
}

void Wolf3DGame::renderWorld() {
  int centerY = (RENDER_H / 2) + bobOffsetY();
  for (int x = 0; x < RENDER_W; x++) {
    float cameraX = 2.0f * static_cast<float>(x) / static_cast<float>(RENDER_W) - 1.0f;
    float rayDirX = dirX + planeX * cameraX;
    float rayDirY = dirY + planeY * cameraX;

    int mapX = static_cast<int>(playerX);
    int mapY = static_cast<int>(playerY);
    float deltaDistX = (rayDirX == 0.0f) ? 1e30f : fabsf(1.0f / rayDirX);
    float deltaDistY = (rayDirY == 0.0f) ? 1e30f : fabsf(1.0f / rayDirY);

    int stepX = 0;
    int stepY = 0;
    float sideDistX = 0.0f;
    float sideDistY = 0.0f;

    if (rayDirX < 0.0f) {
      stepX = -1;
      sideDistX = (playerX - static_cast<float>(mapX)) * deltaDistX;
    } else {
      stepX = 1;
      sideDistX = (static_cast<float>(mapX + 1) - playerX) * deltaDistX;
    }
    if (rayDirY < 0.0f) {
      stepY = -1;
      sideDistY = (playerY - static_cast<float>(mapY)) * deltaDistY;
    } else {
      stepY = 1;
      sideDistY = (static_cast<float>(mapY + 1) - playerY) * deltaDistY;
    }

    bool hit = false;
    bool side = false;
    uint8_t tile = 1;
    while (!hit) {
      if (sideDistX < sideDistY) {
        sideDistX += deltaDistX;
        mapX += stepX;
        side = false;
      } else {
        sideDistY += deltaDistY;
        mapY += stepY;
        side = true;
      }

      if (mapX < 0 || mapX >= mapWidth || mapY < 0 || mapY >= mapHeight) {
        tile = 1;
        hit = true;
        continue;
      }

      tile = map[mapY][mapX];
      if (tile == 0 || Pickups::isPickup(tile)) {
        continue;
      }

      if (Door::isTile(tile)) {
        float testPerpWallDist;
        if (!side) {
          testPerpWallDist =
            (static_cast<float>(mapX) - playerX + (1.0f - static_cast<float>(stepX)) * 0.5f) /
            rayDirX;
        } else {
          testPerpWallDist =
            (static_cast<float>(mapY) - playerY + (1.0f - static_cast<float>(stepY)) * 0.5f) /
            rayDirY;
        }
        if (testPerpWallDist < 0.001f) {
          testPerpWallDist = 0.001f;
        }

        float testWallX;
        if (!side) {
          testWallX = playerY + testPerpWallDist * rayDirY;
        } else {
          testWallX = playerX + testPerpWallDist * rayDirX;
        }
        testWallX -= floorf(testWallX);

        int testTexX = static_cast<int>(testWallX * static_cast<float>(Walls::TEX_SIZE));
        testTexX = Math::clamp(testTexX, 0, Walls::TEX_SIZE - 1);
        if ((!side && rayDirX < 0.0f) || (side && rayDirY > 0.0f)) {
          testTexX = Walls::TEX_SIZE - testTexX - 1;
        }

        int shiftedDoorTexX = Door::shiftedTexX(testTexX, doorOpenAmountAt(mapX, mapY));
        if (shiftedDoorTexX < 0 || shiftedDoorTexX >= Door::TEX_SIZE) {
          continue;
        }
      }
      hit = true;
    }

    float perpWallDist;
    if (!side) {
      perpWallDist =
        (static_cast<float>(mapX) - playerX + (1.0f - static_cast<float>(stepX)) * 0.5f) /
        rayDirX;
    } else {
      perpWallDist =
        (static_cast<float>(mapY) - playerY + (1.0f - static_cast<float>(stepY)) * 0.5f) /
        rayDirY;
    }
    if (perpWallDist < 0.001f) {
      perpWallDist = 0.001f;
    }

    int lineHeight = static_cast<int>(static_cast<float>(RENDER_H) / perpWallDist);
    int rawDrawStart = (-lineHeight / 2) + centerY;
    int drawStart = rawDrawStart;
    int drawEnd = (lineHeight / 2) + centerY;
    if (drawStart < 0) {
      drawStart = 0;
    }
    if (drawEnd >= RENDER_H) {
      drawEnd = RENDER_H - 1;
    }

    float wallX;
    if (!side) {
      wallX = playerY + perpWallDist * rayDirY;
    } else {
      wallX = playerX + perpWallDist * rayDirX;
    }
    wallX -= floorf(wallX);

    int texX = static_cast<int>(wallX * static_cast<float>(Walls::TEX_SIZE));
    texX = Math::clamp(texX, 0, Walls::TEX_SIZE - 1);
    if ((!side && rayDirX < 0.0f) || (side && rayDirY > 0.0f)) {
      texX = Walls::TEX_SIZE - texX - 1;
    }

    uint8_t doorOpenAmount = Door::isTile(tile) ? doorOpenAmountAt(mapX, mapY) : 0;
    wallDepth[x] = perpWallDist;
    renderColumn(
      x,
      lineHeight,
      rawDrawStart,
      drawStart,
      drawEnd,
      tile,
      texX,
      perpWallDist,
      side,
      doorOpenAmount);
  }
}

void Wolf3DGame::renderFpsCounter() {
  char fpsText[12];
  snprintf(fpsText, sizeof(fpsText), "%u FPS", static_cast<unsigned>(displayedFps));

  FrameBufferFillRect fillRect(frameBuffer, RENDER_W, RENDER_H);
  const int textScale = 1;
  const int textW = FontRenderer::textWidth(FONT_5X7, fpsText, textScale);
  const int textH = FONT_5X7.glyphHeight() * textScale;
  const int paddingX = 3;
  const int paddingY = 2;
  const int boxW = textW + paddingX * 2;
  const int boxH = textH + paddingY * 2;
  const int boxX = RENDER_W - boxW - 2;
  const int boxY = 2;
  const uint16_t bg = Color565::rgb(8, 10, 14);
  const uint16_t border = Color565::rgb(60, 76, 92);
  const uint16_t text = Color565::rgb(236, 220, 180);

  fillRect.fillRect565(boxX, boxY, boxW, boxH, bg);
  fillRect.fillRect565(boxX, boxY, boxW, 1, border);
  fillRect.fillRect565(boxX, boxY + boxH - 1, boxW, 1, border);
  fillRect.fillRect565(boxX, boxY, 1, boxH, border);
  fillRect.fillRect565(boxX + boxW - 1, boxY, 1, boxH, border);
  FontRenderer::drawText(FONT_5X7, fillRect, boxX + paddingX, boxY + paddingY, fpsText, textScale, text);
}

void Wolf3DGame::renderSprites(uint32_t nowMs) {
  int spriteCount = 0;

  for (int mapY = 0; mapY < mapHeight && spriteCount < SpriteRenderer::MAX_SPRITES; mapY++) {
    for (int mapX = 0; mapX < mapWidth && spriteCount < SpriteRenderer::MAX_SPRITES; mapX++) {
      uint8_t tile = map[mapY][mapX];
      if (!Pickups::isPickup(tile)) {
        continue;
      }

      Pickups::initSprite(
        spriteStorage[spriteCount],
        tile,
        static_cast<float>(mapX) + 0.5f,
        static_cast<float>(mapY) + 0.5f);
      spriteRefs[spriteCount] = &spriteStorage[spriteCount];
      spriteCount++;
    }
  }

  for (int i = 0; i < zombieCount && spriteCount < SpriteRenderer::MAX_SPRITES; i++) {
    if (!zombies[i].isAlive()) {
      continue;
    }
    spriteRefs[spriteCount] = &zombies[i].sprite();
    spriteCount++;
  }

  SpriteRenderer::RenderView view;
  view.frameBuffer = frameBuffer;
  view.width = RENDER_W;
  view.height = RENDER_H;
  view.wallDepth = wallDepth;
  view.playerX = playerX;
  view.playerY = playerY;
  view.dirX = dirX;
  view.dirY = dirY;
  view.planeX = planeX;
  view.planeY = planeY;
  view.cameraYOffset = bobOffsetY();
  view.nowMs = nowMs;
  SpriteRenderer::render(view, spriteRefs, spriteCount);
}

void Wolf3DGame::renderWeapon(uint32_t nowMs) {
  bool muzzleFlash =
    nowMs < shotUntilMs && (shotUntilMs - nowMs) > (FACE_SHOOT_MS - WEAPON_FLASH_MS);
  WeaponRenderer::render(frameBuffer, RENDER_W, RENDER_H, bobOffsetY(), muzzleFlash);

  putPixel(RENDER_W / 2, (RENDER_H / 2) + 2 + bobOffsetY(), Color565::rgb(240, 240, 240));
  putPixel((RENDER_W / 2) - 2, (RENDER_H / 2) + 2 + bobOffsetY(), Color565::rgb(200, 200, 200));
  putPixel((RENDER_W / 2) + 2, (RENDER_H / 2) + 2 + bobOffsetY(), Color565::rgb(200, 200, 200));
}

void Wolf3DGame::renderColumn(
  int x,
  int lineHeight,
  int rawDrawStart,
  int drawStart,
  int drawEnd,
  uint8_t tile,
  int texX,
  float distance,
  bool side,
  uint8_t doorOpenAmount
) {
  uint16_t* fb = frameBuffer;
  if (x < 0 || x >= RENDER_W) {
    return;
  }
  int span = drawEnd - drawStart + 1;
  if (span <= 0) {
    return;
  }
  uint16_t shadedColumn[Walls::TEX_SIZE];
  bool opaqueColumn[Walls::TEX_SIZE];
  bool isDoor = Door::isTile(tile);
  int shiftedTexX = texX;
  if (isDoor && doorOpenAmount > 0) {
    shiftedTexX = Door::shiftedTexX(texX, doorOpenAmount);
  }
  for (int texY = 0; texY < Walls::TEX_SIZE; texY++) {
    uint16_t texel565 = 0;
    bool opaque = true;
    if (isDoor) {
      if (shiftedTexX >= 0 && shiftedTexX < Door::TEX_SIZE) {
        texel565 = Door::texel(tile, shiftedTexX, texY);
      } else {
        opaque = false;
      }
    } else {
      texel565 = Walls::texel(tile, texX, texY);
    }
    opaqueColumn[texY] = opaque;
    shadedColumn[texY] = opaque ? shadeColor(texel565, distance, side) : 0;
  }
  int texStep = (Walls::TEX_SIZE << 16) / Math::clamp(lineHeight, 1, 1 << 14);
  int texPos = (drawStart - rawDrawStart) * texStep;
  for (int y = drawStart; y <= drawEnd; y++) {
    int texY = texPos >> 16;
    if (texY < 0) {
      texY = 0;
    }
    if (texY >= Walls::TEX_SIZE) {
      texY = Walls::TEX_SIZE - 1;
    }
    if (isDoor && !opaqueColumn[texY]) {
      texPos += texStep;
      continue;
    }
    fb[y * RENDER_W + x] = shadedColumn[texY];
    texPos += texStep;
  }
}

void Wolf3DGame::putPixel(int x, int y, uint16_t color565) {
  uint16_t* fb = frameBuffer;
  if (x < 0 || x >= RENDER_W || y < 0 || y >= RENDER_H) {
    return;
  }
  fb[y * RENDER_W + x] = color565;
}

void Wolf3DGame::fillRect(int x0, int y0, int w, int h, uint16_t color565) {
  if (w <= 0 || h <= 0) {
    return;
  }
  for (int y = y0; y < y0 + h; y++) {
    for (int x = x0; x < x0 + w; x++) {
      putPixel(x, y, color565);
    }
  }
}

uint16_t Wolf3DGame::shadeColor(uint16_t color565, float distance, bool side) const {
  float brightness = 1.0f / (1.0f + distance * 0.18f);
  if (SIDE_SHADE && side) {
    brightness *= 0.72f;
  }
  brightness = Math::clamp(brightness, 0.18f, 1.0f);

  uint8_t r5 = (color565 >> 11) & 0x1F;
  uint8_t g6 = (color565 >> 5) & 0x3F;
  uint8_t b5 = color565 & 0x1F;
  uint8_t r = static_cast<uint8_t>((static_cast<float>(r5) * 255.0f / 31.0f) * brightness);
  uint8_t g = static_cast<uint8_t>((static_cast<float>(g6) * 255.0f / 63.0f) * brightness);
  uint8_t b = static_cast<uint8_t>((static_cast<float>(b5) * 255.0f / 31.0f) * brightness);
  return Color565::rgb(r, g, b);
}
