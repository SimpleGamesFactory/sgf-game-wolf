#include "Game.h"

#include <Arduino.h>
#include <math.h>
#include <stdio.h>
#include <stdlib.h>

#include "Door.h"
#include "FloorRenderer.h"
#include "Keys.h"
#include "Map.h"
#include "Minimap.h"
#include "SGF/Color565.h"
#include "SGF/FontRenderer.h"
#include "SGF/IFillRect.h"
#include "SGF/Math.h"

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
  world.doorOpenBits = doorOpenBits;
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

bool Wolf3DGame::isDoorOpen(int cellX, int cellY) const {
  if (cellX < 0 || cellX >= MAP_MAX_W || cellY < 0 || cellY >= MAP_MAX_H) {
    return false;
  }
  int index = cellY * MAP_MAX_W + cellX;
  return (doorOpenBits[index >> 3] & (1u << (index & 7))) != 0;
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

bool Wolf3DGame::wallAt(int cellX, int cellY) const {
  if (cellX < 0 || cellX >= mapWidth || cellY < 0 || cellY >= mapHeight) {
    return true;
  }
  uint8_t tile = map[cellY][cellX];
  if (tile == 0) {
    return false;
  }
  if (Keys::isPickup(tile)) {
    return false;
  }
  if (Door::isTile(tile)) {
    return !Door::isPassable(isDoorOpen(cellX, cellY));
  }
  return true;
}

bool Wolf3DGame::attemptMove(float nextX, float nextY) {
  bool moved = false;
  if (!wallAt(static_cast<int>(nextX), static_cast<int>(playerY))) {
    playerX = nextX;
    moved = true;
  }
  if (!wallAt(static_cast<int>(playerX), static_cast<int>(nextY))) {
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
    if (!hasKey(required)) {
      return true;
    }
    if (required != KeyColor::None) {
      keysOwned &= static_cast<uint8_t>(~Keys::bitFor(required));
      hud.setKeys(keysOwned);
      map[cellY][cellX] = 'D';
    }
    setDoorOpen(cellX, cellY, true);
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
  if (!Keys::isPickup(tile)) {
    return;
  }

  keysOwned |= Keys::bitFor(Keys::colorForPickup(tile));
  map[cellY][cellX] = 0;
  hud.setKeys(keysOwned);
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
    zombies[targetIndex].kill();
  }
}

void Wolf3DGame::applyDamage(int amount) {
  if (amount <= 0) {
    return;
  }

  uint32_t nowMs = millis();
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
    }
  }
  if (downAction.isPressed()) {
    if (!attemptMove(playerX - dirX * moveStep, playerY - dirY * moveStep)) {
      onBlockedMove();
    }
  }

  if (fireAction.isPressed()) {
    float sideX = -dirY;
    float sideY = dirX;
    if (leftAction.isPressed()) {
      if (!attemptMove(playerX - sideX * strafeStep, playerY - sideY * strafeStep)) {
        onBlockedMove();
      }
    }
    if (rightAction.isPressed()) {
      if (!attemptMove(playerX + sideX * strafeStep, playerY + sideY * strafeStep)) {
        onBlockedMove();
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

  sectionStartUs = micros();
  renderKeys();
  profiler.add(WolfProfiler::Slot::Keys, micros() - sectionStartUs);

  uint32_t nowMs = millis();
  sectionStartUs = micros();
  renderZombies(nowMs);
  profiler.add(WolfProfiler::Slot::ZombieRender, micros() - sectionStartUs);

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
      doorOpenBits,
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
  uint32_t nowMs = millis();
  uint8_t hitFlashStrength = damageFlashStrength(nowMs, damageFlashUntilMs, DAMAGE_FLASH_MS);
  uint8_t muzzleFlashStrength = shotFlashStrength(nowMs, shotUntilMs, WEAPON_FLASH_MS);
  const bool streamViewport = renderTarget.supportsBlit565Stream();
  const bool preSwappedStreamViewport =
    streamViewport && renderTarget.supportsPreSwappedBlit565Stream();
  const bool queuedPreSwappedStreamViewport =
    preSwappedStreamViewport && renderTarget.supportsQueuedPreSwappedBlit565Stream();
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
      const uint16_t* src = &frameBuffer[(srcY0 + localY) * RENDER_W];
      uint16_t* dstRow0 = &activeUpscaleBuffer[(localY * UPSCALE) * VIEWPORT_W];
      uint16_t* dstRow1 = dstRow0 + VIEWPORT_W;
      if (hitFlashStrength == 0 && muzzleFlashStrength == 0) {
        for (int x = 0; x < RENDER_W; x++) {
          uint16_t color565 = src[x];
          if (preSwappedStreamViewport) {
            color565 = Color565::bswap(color565);
          }
          int dstX = x * UPSCALE;
          dstRow0[dstX] = color565;
          dstRow0[dstX + 1] = color565;
          dstRow1[dstX] = color565;
          dstRow1[dstX + 1] = color565;
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
          dstRow0[dstX] = color565;
          dstRow0[dstX + 1] = color565;
          dstRow1[dstX] = color565;
          dstRow1[dstX + 1] = color565;
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
  for (int y = 0; y < RENDER_H; y++) {
    bool isSky = y < RENDER_H / 2;
    float bandT;
    if (isSky) {
      bandT = static_cast<float>(y) / static_cast<float>(RENDER_H / 2);
    } else {
      bandT = static_cast<float>(y - RENDER_H / 2) /
              static_cast<float>(RENDER_H - RENDER_H / 2);
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

    uint16_t* row = &frameBuffer[y * RENDER_W];
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
      if (wallAt(mapX, mapY)) {
        hit = true;
      }
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
    int drawStart = (-lineHeight / 2) + (RENDER_H / 2);
    int drawEnd = (lineHeight / 2) + (RENDER_H / 2);
    if (drawStart < 0) {
      drawStart = 0;
    }
    if (drawEnd >= RENDER_H) {
      drawEnd = RENDER_H - 1;
    }

    uint8_t tile = 1;
    if (mapX >= 0 && mapX < mapWidth && mapY >= 0 && mapY < mapHeight) {
      tile = map[mapY][mapX];
    }

    float wallX;
    if (!side) {
      wallX = playerY + perpWallDist * rayDirY;
    } else {
      wallX = playerX + perpWallDist * rayDirX;
    }
    wallX -= floorf(wallX);

    int texX = static_cast<int>(wallX * static_cast<float>(TEX_SIZE));
    texX = Math::clamp(texX, 0, TEX_SIZE - 1);
    if ((!side && rayDirX > 0.0f) || (side && rayDirY < 0.0f)) {
      texX = TEX_SIZE - texX - 1;
    }

    wallDepth[x] = perpWallDist;
    renderColumn(x, drawStart, drawEnd, tile, texX, perpWallDist, side);
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

void Wolf3DGame::renderKeys() {
  float invDet = 1.0f / (planeX * dirY - dirX * planeY);
  for (int mapY = 0; mapY < mapHeight; mapY++) {
    for (int mapX = 0; mapX < mapWidth; mapX++) {
      uint8_t tile = map[mapY][mapX];
      if (!Keys::isPickup(tile)) {
        continue;
      }
      const uint16_t* keyTexture = Keys::texture(tile);
      if (keyTexture == nullptr) {
        continue;
      }

      float spriteX = (static_cast<float>(mapX) + 0.5f) - playerX;
      float spriteY = (static_cast<float>(mapY) + 0.5f) - playerY;
      float transformX = invDet * (dirY * spriteX - dirX * spriteY);
      float transformY = invDet * (-planeY * spriteX + planeX * spriteY);
      if (transformY <= 0.15f) {
        continue;
      }

      int spriteScreenX =
        static_cast<int>((static_cast<float>(RENDER_W) * 0.5f) * (1.0f + transformX / transformY));
      int spriteHeight = abs(static_cast<int>(static_cast<float>(RENDER_H) / transformY));
      int spriteWidth = spriteHeight / 2;
      if (spriteWidth < 4) {
        spriteWidth = 4;
      }

      int pickupDrop = spriteHeight / 3;
      int rawDrawStartY = (-spriteHeight / 2) + (RENDER_H / 2) + pickupDrop;
      int rawDrawEndY = (spriteHeight / 2) + (RENDER_H / 2) + pickupDrop;
      int drawStartY = rawDrawStartY;
      int drawEndY = rawDrawEndY;
      drawStartY = Math::clamp(drawStartY, 0, RENDER_H - 1);
      drawEndY = Math::clamp(drawEndY, 0, RENDER_H - 1);

      int drawStartX = spriteScreenX - spriteWidth / 2;
      int drawEndX = spriteScreenX + spriteWidth / 2;
      drawStartX = Math::clamp(drawStartX, 0, RENDER_W - 1);
      drawEndX = Math::clamp(drawEndX, 0, RENDER_W - 1);
      int texXStep = (Keys::TEX_SIZE << 16) / Math::clamp(spriteWidth, 1, RENDER_W);
      int texXPos = ((drawStartX - (spriteScreenX - spriteWidth / 2)) * Keys::TEX_SIZE << 16) /
                    Math::clamp(spriteWidth, 1, RENDER_W);
      uint16_t shadedTexture[Keys::TEX_SIZE * Keys::TEX_SIZE];
      for (int i = 0; i < Keys::TEX_SIZE * Keys::TEX_SIZE; i++) {
        uint16_t color565 = keyTexture[i];
        shadedTexture[i] = (color565 == 0) ? 0 : shadeColor(color565, transformY, false);
      }

      for (int stripe = drawStartX; stripe <= drawEndX; stripe++) {
        if (transformY >= wallDepth[stripe]) {
          texXPos += texXStep;
          continue;
        }

        int texX = texXPos >> 16;
        texX = Math::clamp(texX, 0, Keys::TEX_SIZE - 1);
        texXPos += texXStep;
        int texYStep = (Keys::TEX_SIZE << 16) / Math::clamp(spriteHeight, 1, RENDER_H);
        int texYPos = ((drawStartY - rawDrawStartY) * Keys::TEX_SIZE << 16) /
                      Math::clamp(spriteHeight, 1, RENDER_H);

        for (int y = drawStartY; y <= drawEndY; y++) {
          int texY = texYPos >> 16;
          texY = Math::clamp(texY, 0, Keys::TEX_SIZE - 1);
          texYPos += texYStep;
          uint16_t color565 = shadedTexture[texY * Keys::TEX_SIZE + texX];
          if (color565 == 0) {
            continue;
          }
          frameBuffer[y * RENDER_W + stripe] = color565;
        }
      }
    }
  }
}

void Wolf3DGame::renderZombies(uint32_t nowMs) {
  int order[Zombie::MAX_COUNT]{};
  float distances[Zombie::MAX_COUNT]{};
  int renderCount = 0;
  for (int i = 0; i < zombieCount; i++) {
    if (!zombies[i].isAlive()) {
      continue;
    }
    float dx = zombies[i].getX() - playerX;
    float dy = zombies[i].getY() - playerY;
    order[renderCount] = i;
    distances[renderCount] = dx * dx + dy * dy;
    renderCount++;
  }

  for (int i = 0; i < renderCount - 1; i++) {
    for (int j = i + 1; j < renderCount; j++) {
      if (distances[j] > distances[i]) {
        float tmpDistance = distances[i];
        distances[i] = distances[j];
        distances[j] = tmpDistance;

        int tmpOrder = order[i];
        order[i] = order[j];
        order[j] = tmpOrder;
      }
    }
  }

  float invDet = 1.0f / (planeX * dirY - dirX * planeY);
  for (int i = 0; i < renderCount; i++) {
    const Zombie& zombie = zombies[order[i]];
    float spriteX = zombie.getX() - playerX;
    float spriteY = zombie.getY() - playerY;
    float transformX = invDet * (dirY * spriteX - dirX * spriteY);
    float transformY = invDet * (-planeY * spriteX + planeX * spriteY);
    if (transformY <= 0.15f) {
      continue;
    }

    int spriteScreenX =
      static_cast<int>((static_cast<float>(RENDER_W) * 0.5f) * (1.0f + transformX / transformY));
    int spriteHeight = abs(static_cast<int>(static_cast<float>(RENDER_H) / transformY));
    int spriteWidth = (spriteHeight * 3) / 4;
    if (spriteWidth < 4) {
      spriteWidth = 4;
    }

      int drawStartY = (-spriteHeight / 2) + (RENDER_H / 2);
    int drawEndY = (spriteHeight / 2) + (RENDER_H / 2);
    drawStartY = Math::clamp(drawStartY, 0, RENDER_H - 1);
    drawEndY = Math::clamp(drawEndY, 0, RENDER_H - 1);

    int drawStartX = spriteScreenX - spriteWidth / 2;
    int drawEndX = spriteScreenX + spriteWidth / 2;
    drawStartX = Math::clamp(drawStartX, 0, RENDER_W - 1);
    drawEndX = Math::clamp(drawEndX, 0, RENDER_W - 1);
    int texXStep = (Zombie::TEX_SIZE << 16) / Math::clamp(spriteWidth, 1, RENDER_W);
    int texXPos = ((drawStartX - (spriteScreenX - spriteWidth / 2)) * Zombie::TEX_SIZE << 16) /
                  Math::clamp(spriteWidth, 1, RENDER_W);
    int texYStep = (Zombie::TEX_SIZE << 16) / Math::clamp(spriteHeight, 1, RENDER_H);
    int rawDrawStartY = (-spriteHeight / 2) + (RENDER_H / 2);
    uint16_t shadedTexture[Zombie::TEX_SIZE * Zombie::TEX_SIZE];
    for (int texY = 0; texY < Zombie::TEX_SIZE; texY++) {
      for (int texX = 0; texX < Zombie::TEX_SIZE; texX++) {
        uint16_t color565 = zombie.texel(texX, texY, nowMs);
        shadedTexture[texY * Zombie::TEX_SIZE + texX] =
          (color565 == 0) ? 0 : shadeColor(color565, transformY, false);
      }
    }

    for (int stripe = drawStartX; stripe <= drawEndX; stripe++) {
      if (transformY >= wallDepth[stripe]) {
        texXPos += texXStep;
        continue;
      }

      int texX = texXPos >> 16;
      texX = Math::clamp(texX, 0, Zombie::TEX_SIZE - 1);
      texXPos += texXStep;
      int texYPos = ((drawStartY - rawDrawStartY) * Zombie::TEX_SIZE << 16) /
                    Math::clamp(spriteHeight, 1, RENDER_H);

      for (int y = drawStartY; y <= drawEndY; y++) {
        int texY = texYPos >> 16;
        texY = Math::clamp(texY, 0, Zombie::TEX_SIZE - 1);
        texYPos += texYStep;
        uint16_t color565 = shadedTexture[texY * Zombie::TEX_SIZE + texX];
        if (color565 == 0) {
          continue;
        }
        frameBuffer[y * RENDER_W + stripe] = color565;
      }
    }
  }
}

void Wolf3DGame::renderWeapon(uint32_t nowMs) {
  const uint16_t metal = Color565::rgb(124, 130, 142);
  const uint16_t darkMetal = Color565::rgb(76, 82, 92);
  const uint16_t grip = Color565::rgb(78, 54, 38);
  const uint16_t muzzle = Color565::rgb(240, 188, 84);
  const uint16_t muzzleHot = Color565::rgb(255, 246, 210);
  const int gunW = 28;
  const int gunH = 18;
  int baseX = (RENDER_W - gunW) / 2;
  int baseY = RENDER_H - gunH - 2;

  fillRect(baseX + 10, baseY + 2, 12, 5, metal);
  fillRect(baseX + 12, baseY + 1, 8, 2, darkMetal);
  fillRect(baseX + 16, baseY + 7, 4, 7, darkMetal);
  fillRect(baseX + 9, baseY + 8, 7, 9, grip);
  fillRect(baseX + 8, baseY + 13, 5, 4, darkMetal);
  fillRect(baseX + 20, baseY + 7, 2, 2, metal);

  bool muzzleFlash =
    nowMs < shotUntilMs && (shotUntilMs - nowMs) > (FACE_SHOOT_MS - WEAPON_FLASH_MS);
  if (muzzleFlash) {
    fillRect(baseX + 22, baseY + 5, 3, 4, muzzle);
    putPixel(baseX + 25, baseY + 6, muzzleHot);
    putPixel(baseX + 25, baseY + 7, muzzle);
    putPixel(baseX + 24, baseY + 4, muzzleHot);
    putPixel(baseX + 24, baseY + 9, muzzle);
  }

  putPixel(RENDER_W / 2, (RENDER_H / 2) + 2, Color565::rgb(240, 240, 240));
  putPixel((RENDER_W / 2) - 2, (RENDER_H / 2) + 2, Color565::rgb(200, 200, 200));
  putPixel((RENDER_W / 2) + 2, (RENDER_H / 2) + 2, Color565::rgb(200, 200, 200));
}

void Wolf3DGame::renderColumn(
  int x,
  int drawStart,
  int drawEnd,
  uint8_t tile,
  int texX,
  float distance,
  bool side
) {
  if (x < 0 || x >= RENDER_W) {
    return;
  }
  int span = drawEnd - drawStart + 1;
  if (span <= 0) {
    return;
  }
  uint16_t shadedColumn[TEX_SIZE];
  for (int texY = 0; texY < TEX_SIZE; texY++) {
    shadedColumn[texY] = shadeColor(wallTexel(tile, texX, texY), distance, side);
  }
  int texStep = (TEX_SIZE << 16) / span;
  int texPos = 0;
  for (int y = drawStart; y <= drawEnd; y++) {
    int texY = texPos >> 16;
    if (texY >= TEX_SIZE) {
      texY = TEX_SIZE - 1;
    }
    frameBuffer[y * RENDER_W + x] = shadedColumn[texY];
    texPos += texStep;
  }
}

void Wolf3DGame::putPixel(int x, int y, uint16_t color565) {
  if (x < 0 || x >= RENDER_W || y < 0 || y >= RENDER_H) {
    return;
  }
  frameBuffer[y * RENDER_W + x] = color565;
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
  if (side) {
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

uint16_t Wolf3DGame::wallColor(uint8_t tile) const {
  switch (tile) {
    case 1: return Color565::rgb(176, 72, 64);
    case 2: return Color565::rgb(80, 168, 96);
    case 3: return Color565::rgb(72, 116, 196);
    case 4: return Color565::rgb(196, 168, 80);
    case 5: return Color565::rgb(168, 92, 180);
    case 6: return Color565::rgb(72, 180, 196);
    case 7: return Color565::rgb(180, 108, 72);
    case 8: return Color565::rgb(196, 72, 128);
    case 9: return Color565::rgb(224, 224, 224);
    default: return Color565::rgb(255, 255, 255);
  }
}

uint16_t Wolf3DGame::wallTexel(uint8_t tile, int texX, int texY) const {
  if (Door::isTile(tile)) {
    return Door::texel(tile, texX, texY);
  }

  uint16_t base = wallColor(tile);
  uint16_t dark = Color565::darken(base);
  uint16_t light = Color565::lighten(base);

  bool edge = (texX == 0 || texY == 0 || texX == TEX_SIZE - 1 || texY == TEX_SIZE - 1);
  bool mortar = ((texX % 8) == 0) || ((texY % 8) == 0);
  bool stripe = ((texX / 3) % 2) == 0;
  bool checker = (((texX / 4) + (texY / 4)) % 2) == 0;
  bool diamond = abs(texX - 8) + abs(texY - 8) < 5;
  bool slit = (texX > 5 && texX < 10 && texY > 2 && texY < 13);

  switch (tile) {
    case 1:
      if (edge || mortar) {
        return dark;
      }
      return base;
    case 2:
      if (edge) {
        return dark;
      }
      return stripe ? light : base;
    case 3:
      if (edge) {
        return light;
      }
      return checker ? base : dark;
    case 4:
      if (edge || ((texY % 5) == 0)) {
        return dark;
      }
      return ((texX % 5) == 0) ? light : base;
    case 5:
      if (edge) {
        return dark;
      }
      return diamond ? light : base;
    case 6:
      if (edge || slit) {
        return dark;
      }
      return (texY < 3 || texY > 12) ? light : base;
    case 7:
      if (edge) {
        return dark;
      }
      return ((texX + texY) % 6) < 3 ? base : light;
    case 8:
      if (edge) {
        return dark;
      }
      return ((texX - texY + TEX_SIZE) % 6) < 3 ? light : base;
    case 9:
      if (edge || checker) {
        return dark;
      }
      return light;
    default:
      return base;
  }
}
