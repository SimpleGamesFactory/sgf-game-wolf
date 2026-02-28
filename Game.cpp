#include "Game.h"

#include <Arduino.h>
#include <math.h>
#include <stdlib.h>

#include "Door.h"
#include "Map.h"
#include "Minimap.h"
#include "SGF/Color565.h"
#include "SGF/Math.h"

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
  start();
}

void Wolf3DGame::onSetup() {
  screenW = renderTarget.width();
  screenH = renderTarget.height();
  worldScreenH = screenH - HUD_H;
  if (screenW != MAX_SCREEN_W || screenH != MAX_SCREEN_H || worldScreenH != WORLD_H) {
    while (true) {
      delay(1000);
    }
  }

  leftPinInput.attach(pinLeft, true);
  rightPinInput.attach(pinRight, true);
  upPinInput.attach(pinUp, true);
  downPinInput.attach(pinDown, true);
  firePinInput.attach(pinFire, true);
  leftPinInput.begin(INPUT_PULLUP);
  rightPinInput.begin(INPUT_PULLUP);
  upPinInput.begin(INPUT_PULLUP);
  downPinInput.begin(INPUT_PULLUP);
  firePinInput.begin(INPUT_PULLUP);
  leftPinInput.resetFromPin();
  rightPinInput.resetFromPin();
  upPinInput.resetFromPin();
  downPinInput.resetFromPin();
  firePinInput.resetFromPin();
  leftAction.reset(leftPinInput.pressed());
  rightAction.reset(rightPinInput.pressed());
  upAction.reset(upPinInput.pressed());
  downAction.reset(downPinInput.pressed());
  fireAction.reset(firePinInput.pressed());

  ammo = START_AMMO;
  lives = START_LIVES;
  energy = START_ENERGY;
  resetMap();
  resetPlayerPose();
  nextBlinkMs = millis() + 1300;
  blinkUntilMs = 0;
  shotUntilMs = 0;
  hurtUntilMs = 0;
  nextDamageMs = 0;

  hud.begin(screenW, worldScreenH);
  hud.setLives(lives);
  hud.setAmmo(ammo);
  hud.setEnergy(energy);
  hud.setFaceMood(currentFaceMood(millis()));
  hud.invalidate();

  renderFrame();
  presentFrame();
  hud.render();
  hud.flush(renderTarget);
  resetClock();
}

void Wolf3DGame::onPhysics(float delta) {
  leftAction.update(leftPinInput.update());
  rightAction.update(rightPinInput.update());
  upAction.update(upPinInput.update());
  downAction.update(downPinInput.update());
  fireAction.update(firePinInput.update());

  updateInput(delta);
  updateZombies(delta);
  updateHudAnimation();
}

void Wolf3DGame::onProcess(float delta) {
  (void)delta;
  renderFrame();
  presentFrame();
  hud.render();
  hud.flush(renderTarget);
  frameCounter++;
}

void Wolf3DGame::resetMap() {
  Map::load(map, doorOpen, mapWidth, mapHeight, spawn);
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
  world.doorOpen = &doorOpen[0][0];
  world.mapStride = MAP_MAX_W;
  world.mapWidth = mapWidth;
  world.mapHeight = mapHeight;
  world.playerX = playerX;
  world.playerY = playerY;
  world.nowMs = nowMs;
  world.delta = delta;
  return world;
}

bool Wolf3DGame::wallAt(int cellX, int cellY) const {
  if (cellX < 0 || cellX >= mapWidth || cellY < 0 || cellY >= mapHeight) {
    return true;
  }
  uint8_t tile = map[cellY][cellX];
  if (tile == 0) {
    return false;
  }
  if (Door::isTile(tile)) {
    return !Door::isPassable(doorOpen[cellY][cellX]);
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

  if (doorOpen[cellY][cellX]) {
    if (canCloseDoor(cellX, cellY)) {
      doorOpen[cellY][cellX] = false;
    }
  } else {
    doorOpen[cellY][cellX] = true;
  }
  return true;
}

bool Wolf3DGame::canCloseDoor(int cellX, int cellY) const {
  int playerCellX = static_cast<int>(playerX);
  int playerCellY = static_cast<int>(playerY);
  return playerCellX != cellX || playerCellY != cellY;
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

  energy -= amount;
  hurtUntilMs = millis() + FACE_HURT_MS;
  if (energy > 0) {
    hud.setEnergy(energy);
    hud.setFaceMood(currentFaceMood(millis()));
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
  hud.setFaceMood(currentFaceMood(millis()));
}

void Wolf3DGame::updateInput(float delta) {
  bool minimapShortcut = fireAction.pressed() && upAction.pressed();
  if (minimapShortcut && !minimapShortcutHeld) {
    showMinimap = !showMinimap;
  }
  minimapShortcutHeld = minimapShortcut;

  if (fireAction.justPressed() &&
      !upAction.pressed() &&
      !downAction.pressed() &&
      !leftAction.pressed() &&
      !rightAction.pressed()) {
    if (!toggleDoorAhead()) {
      shoot();
    }
  }

  float moveStep = MOVE_SPEED * delta;
  float strafeStep = STRAFE_SPEED * delta;
  float turnStep = TURN_SPEED * delta;

  if (upAction.pressed() && !fireAction.pressed()) {
    if (!attemptMove(playerX + dirX * moveStep, playerY + dirY * moveStep)) {
      onBlockedMove();
    }
  }
  if (downAction.pressed()) {
    if (!attemptMove(playerX - dirX * moveStep, playerY - dirY * moveStep)) {
      onBlockedMove();
    }
  }

  if (fireAction.pressed()) {
    float sideX = -dirY;
    float sideY = dirX;
    if (leftAction.pressed()) {
      if (!attemptMove(playerX - sideX * strafeStep, playerY - sideY * strafeStep)) {
        onBlockedMove();
      }
    }
    if (rightAction.pressed()) {
      if (!attemptMove(playerX + sideX * strafeStep, playerY + sideY * strafeStep)) {
        onBlockedMove();
      }
    }
  } else {
    if (leftAction.pressed()) {
      rotate(-turnStep);
    }
    if (rightAction.pressed()) {
      rotate(turnStep);
    }
  }
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

void Wolf3DGame::renderFrame() {
  clearFrame();
  renderFloor();
  renderWorld();
  renderZombies(millis());
  renderWeapon(millis());
  if (showMinimap) {
    Minimap::render(
      frameBuffer,
      RENDER_W,
      RENDER_H,
      &map[0][0],
      &doorOpen[0][0],
      MAP_MAX_W,
      mapWidth,
      mapHeight,
      playerX,
      playerY,
      dirX,
      dirY);
  }
}

void Wolf3DGame::presentFrame() {
  for (int y = 0; y < RENDER_H; y++) {
    const uint16_t* src = &frameBuffer[y * RENDER_W];
    for (int x = 0; x < RENDER_W; x++) {
      uint16_t color565 = src[x];
      int dstX = x * UPSCALE;
      upscaleBuffer[dstX] = color565;
      upscaleBuffer[dstX + 1] = color565;
      upscaleBuffer[screenW + dstX] = color565;
      upscaleBuffer[screenW + dstX + 1] = color565;
    }
    renderTarget.blit565(0, y * UPSCALE, screenW, UPSCALE, upscaleBuffer);
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
  const int halfH = RENDER_H / 2;
  const uint16_t floorColor = Color565::rgb(60, 52, 46);
  for (int y = halfH + 1; y < RENDER_H; y++) {
    uint16_t* row = &frameBuffer[y * RENDER_W];
    for (int x = 0; x < RENDER_W; x++) {
      row[x] = floorColor;
    }
  }
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

    for (int stripe = drawStartX; stripe <= drawEndX; stripe++) {
      if (transformY >= wallDepth[stripe]) {
        continue;
      }

      int texX =
        ((stripe - drawStartX) * Zombie::TEX_SIZE) / Math::clamp(spriteWidth, 1, RENDER_W);
      texX = Math::clamp(texX, 0, Zombie::TEX_SIZE - 1);

      for (int y = drawStartY; y <= drawEndY; y++) {
        int texY =
          ((y - drawStartY) * Zombie::TEX_SIZE) / Math::clamp(spriteHeight, 1, RENDER_H);
        texY = Math::clamp(texY, 0, Zombie::TEX_SIZE - 1);
        uint16_t color565 = zombie.texel(texX, texY, nowMs);
        if (color565 == 0) {
          continue;
        }
        frameBuffer[y * RENDER_W + stripe] = shadeColor(color565, transformY, false);
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
  for (int y = drawStart; y <= drawEnd; y++) {
    int texY = ((y - drawStart) * TEX_SIZE) / span;
    texY = Math::clamp(texY, 0, TEX_SIZE - 1);
    uint16_t color565 = wallTexel(tile, texX, texY);
    frameBuffer[y * RENDER_W + x] = shadeColor(color565, distance, side);
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
    return Door::texel(texX, texY);
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
