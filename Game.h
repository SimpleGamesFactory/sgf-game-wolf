#pragma once

#include <stdint.h>

#ifndef WOLF_VIEWPORT_W
#define WOLF_VIEWPORT_W 240
#endif

#ifndef WOLF_VIEWPORT_H
#define WOLF_VIEWPORT_H 180
#endif

#ifndef WOLF_SIMPLE_FLOOR
#define WOLF_SIMPLE_FLOOR 1
#endif

#ifndef WOLF_CAMERA_PLANE_SCALE
#define WOLF_CAMERA_PLANE_SCALE 0.66f
#endif

#ifndef WOLF_PLAYER_RADIUS
#define WOLF_PLAYER_RADIUS 0.28f
#endif

#ifndef WOLF_SIDE_SHADE
#define WOLF_SIDE_SHADE 0
#endif

#ifndef WOLF_UPSCALE
#define WOLF_UPSCALE 2
#endif

#ifndef WOLF_UPSCALE_BUFFER_COUNT
#define WOLF_UPSCALE_BUFFER_COUNT 2
#endif

#ifndef WOLF_DYNAMIC_FRAMEBUFFER
#define WOLF_DYNAMIC_FRAMEBUFFER 1
#endif

#ifndef WOLF_DYNAMIC_FRAMEBUFFER_ALIGNMENT
#define WOLF_DYNAMIC_FRAMEBUFFER_ALIGNMENT 32
#endif

#ifndef WOLF_HEAP_COLD_BUFFERS
#define WOLF_HEAP_COLD_BUFFERS 1
#endif

#include "SGF/ActionState.h"
#include "SGF/IFillRect.h"
#include "SGF/Font5x7.h"
#include "SGF/Game.h"
#include "SGF/HardwareProfile.h"
#include "SGF/DebouncedInputPin.h"
#include "SGF/IRenderTarget.h"
#include "SGF/IScreen.h"
#include "Hud.h"
#include "Keys.h"
#include "Map.h"
#include "Sprite.h"
#include "SpriteRenderer.h"
#include "Walls.h"
#include "WolfProfiler.h"
#include "Zombie.h"

class Wolf3DGame : public Game {
public:
  Wolf3DGame(
    IRenderTarget& renderTarget,
    IScreen& screen,
    const SGFHardware::HardwareProfile& hardwareProfile
  );

  void setup();
  Profiler& stageProfiler() { return profiler.profiler(); }

private:
  static constexpr uint32_t FRAME_DEFAULT_STEP_US = 16666u;
  static constexpr uint32_t FRAME_MAX_STEP_US = 100000u;
  static constexpr int MAX_SCREEN_W = 240;
  static constexpr int MAX_SCREEN_H = 240;
  static constexpr int HUD_H = 40;
  static constexpr int WORLD_AREA_H = MAX_SCREEN_H - HUD_H;
  static constexpr int VIEWPORT_W = WOLF_VIEWPORT_W;
  static constexpr int VIEWPORT_H = WOLF_VIEWPORT_H;
  static constexpr int VIEWPORT_X = (MAX_SCREEN_W - VIEWPORT_W) / 2;
  static constexpr int VIEWPORT_Y = (WORLD_AREA_H - VIEWPORT_H) / 2;
  static constexpr int UPSCALE = WOLF_UPSCALE;
  static constexpr size_t DYNAMIC_FRAMEBUFFER_ALIGNMENT = WOLF_DYNAMIC_FRAMEBUFFER_ALIGNMENT;
  static constexpr int UPSCALE_CHUNK_SRC_ROWS = 3;
  static constexpr int UPSCALE_BUFFER_COUNT = WOLF_UPSCALE_BUFFER_COUNT;
  static constexpr bool SIMPLE_FLOOR = WOLF_SIMPLE_FLOOR != 0;
  static constexpr int RENDER_W = VIEWPORT_W / UPSCALE;
  static constexpr int RENDER_H = VIEWPORT_H / UPSCALE;
  static_assert(UPSCALE >= 1 && UPSCALE <= 4, "WOLF_UPSCALE must be 1..4");
  static_assert(
    DYNAMIC_FRAMEBUFFER_ALIGNMENT >= sizeof(void*) &&
      (DYNAMIC_FRAMEBUFFER_ALIGNMENT & (DYNAMIC_FRAMEBUFFER_ALIGNMENT - 1u)) == 0u,
    "WOLF_DYNAMIC_FRAMEBUFFER_ALIGNMENT must be a power of two and >= sizeof(void*)");
  static_assert(
    UPSCALE_BUFFER_COUNT >= 1 && UPSCALE_BUFFER_COUNT <= 4,
    "WOLF_UPSCALE_BUFFER_COUNT must be 1..4");
  static_assert((VIEWPORT_W % UPSCALE) == 0, "WOLF_VIEWPORT_W must be divisible by WOLF_UPSCALE");
  static_assert((VIEWPORT_H % UPSCALE) == 0, "WOLF_VIEWPORT_H must be divisible by WOLF_UPSCALE");
  static constexpr int MAP_MAX_W = Map::MAX_WIDTH;
  static constexpr int MAP_MAX_H = Map::MAX_HEIGHT;
  static constexpr float MOVE_SPEED = 2.2f;
  static constexpr float STRAFE_SPEED = 2.2f;
  static constexpr float TURN_SPEED = 2.4f;
  static constexpr float HEAD_BOB_SPEED = 12.0f;
  static constexpr float HEAD_BOB_AMPLITUDE = 1.5f;
  static constexpr float HEAD_BOB_SETTLE = 10.0f;
  static constexpr bool SIDE_SHADE = WOLF_SIDE_SHADE != 0;
  static constexpr float CAMERA_PLANE_SCALE = WOLF_CAMERA_PLANE_SCALE;
  static constexpr float PLAYER_RADIUS = WOLF_PLAYER_RADIUS;
  static constexpr int MINIMAP_CELL = 5;
  static constexpr int MINIMAP_MARGIN = 6;
  static constexpr int START_AMMO = 99;
  static constexpr int START_LIVES = 3;
  static constexpr int START_ENERGY = 100;
  static constexpr int DAMAGE_ON_BUMP = 6;
  static constexpr float DOOR_REACH = 0.85f;
  static constexpr uint32_t WEAPON_FLASH_MS = 90;
  static constexpr uint32_t DAMAGE_FLASH_MS = 120;
  static constexpr uint32_t BUMP_DAMAGE_COOLDOWN_MS = 260;
  static constexpr uint32_t FACE_BLINK_MS = 120;
  static constexpr uint32_t FACE_SHOOT_MS = 150;
  static constexpr uint32_t FACE_HURT_MS = 260;
  static constexpr uint32_t MINIMAP_HOLD_MS = 250;
  static constexpr int HUD_LIVES_X = 4;
  static constexpr int HUD_LIVES_Y = 4;
  static constexpr int HUD_LIVES_W = 58;
  static constexpr int HUD_LIVES_H = 32;
  static constexpr int HUD_FACE_X = 74;
  static constexpr int HUD_FACE_Y = 4;
  static constexpr int HUD_FACE_W = 92;
  static constexpr int HUD_FACE_H = 32;
  static constexpr int HUD_STATS_X = 176;
  static constexpr int HUD_AMMO_Y = 4;
  static constexpr int HUD_AMMO_H = 14;
  static constexpr int HUD_ENERGY_Y = 21;
  static constexpr int HUD_ENERGY_H = 14;
  static constexpr int HUD_STATS_W = 60;

  IRenderTarget& renderTarget;
  IScreen& screen;
  SGFHardware::HardwareProfile hardwareProfile;
  int screenW = 0;
  int screenH = 0;
  int worldScreenH = WORLD_AREA_H;

  uint8_t pinLeft = 0;
  uint8_t pinRight = 0;
  uint8_t pinUp = 0;
  uint8_t pinDown = 0;
  uint8_t pinFire = 0;
  DebouncedInputPin leftPinInput;
  DebouncedInputPin rightPinInput;
  DebouncedInputPin upPinInput;
  DebouncedInputPin downPinInput;
  DebouncedInputPin firePinInput;
  ActionState leftAction;
  ActionState rightAction;
  ActionState upAction;
  ActionState downAction;
  ActionState fireAction;

#if WOLF_DYNAMIC_FRAMEBUFFER
  uint16_t* frameBuffer = nullptr;
#else
  uint16_t frameBufferStorage[RENDER_W * RENDER_H]{};
  uint16_t* frameBuffer = frameBufferStorage;
#endif
  uint16_t upscaleBuffers[UPSCALE_BUFFER_COUNT][MAX_SCREEN_W * UPSCALE * UPSCALE_CHUNK_SRC_ROWS]{};
  float wallDepth[RENDER_W]{};
  WolfRender::Sprite spriteStorage[SpriteRenderer::MAX_SPRITES]{};
  const WolfRender::ISprite* spriteRefs[SpriteRenderer::MAX_SPRITES]{};

  float playerX = 3.5f;
  float playerY = 3.5f;
  float dirX = 1.0f;
  float dirY = 0.0f;
  float planeX = 0.0f;
  float planeY = CAMERA_PLANE_SCALE;
  float headBobPhase = 0.0f;
  float headBobOffset = 0.0f;
  uint32_t frameCounter = 0;
  uint16_t displayedFps = 0;
  uint16_t fpsSampleFrames = 0;
  uint32_t fpsSampleStartMs = 0;
  int ammo = START_AMMO;
  int lives = START_LIVES;
  int energy = START_ENERGY;
  uint8_t keysOwned = 0;
  bool showMinimap = false;
  bool minimapShortcutHeld = false;
  bool minimapShortcutTriggered = false;
  uint32_t minimapShortcutStartMs = 0;
  uint32_t damageFlashUntilMs = 0;
  uint32_t nextBlinkMs = 0;
  uint32_t blinkUntilMs = 0;
  uint32_t shotUntilMs = 0;
  uint32_t hurtUntilMs = 0;
  uint32_t nextDamageMs = 0;
  WolfProfiler profiler;
  Hud hud;
#if WOLF_HEAP_COLD_BUFFERS
  uint8_t (*map)[MAP_MAX_W] = nullptr;
  uint8_t* doorOpenAmounts = nullptr;
#else
  uint8_t mapStorage[MAP_MAX_H][MAP_MAX_W]{};
  uint8_t (*map)[MAP_MAX_W] = mapStorage;
  uint8_t doorOpenAmountStorage[MAP_MAX_H * MAP_MAX_W]{};
  uint8_t* doorOpenAmounts = doorOpenAmountStorage;
#endif
  uint8_t doorOpenBits[Map::DOOR_OPEN_BYTES]{};
  uint8_t doorUnlockedBits[Map::DOOR_OPEN_BYTES]{};
  int mapWidth = 0;
  int mapHeight = 0;
  Map::Spawn spawn;
  Zombie zombies[Zombie::MAX_COUNT]{};
  int zombieCount = 0;

  void onSetup() override;
  void onPhysics(float delta) override;
  void onProcess(float delta) override;

  void resetMap();
  void resetPlayerPose();
  Zombie::WorldView makeZombieWorldView(uint32_t nowMs, float delta) const;
  bool hasKey(KeyColor color) const;
  uint8_t doorOpenAmountAt(int cellX, int cellY) const;
  bool isDoorOpen(int cellX, int cellY) const;
  bool isDoorUnlocked(int cellX, int cellY) const;
  void setDoorOpen(int cellX, int cellY, bool open);
  void setDoorUnlocked(int cellX, int cellY, bool unlocked);
  void updateDoors(float delta);
  bool wallAt(int cellX, int cellY) const;
  bool blockedAt(float testX, float testY) const;
  bool attemptMove(float nextX, float nextY);
  void onBlockedMove();
  void rotate(float angle);
  void updateHeadBob(float delta, bool moved);
  int bobOffsetY() const;
  bool toggleDoorAhead();
  bool canCloseDoor(int cellX, int cellY) const;
  void collectPickupUnderPlayer();
  void shoot();
  void applyDamage(int amount);
  void updateInput(float delta);
  void updateZombies(float delta);
  void updateHudAnimation();
  void updateFpsCounter(uint32_t nowMs);
  Hud::FaceMood currentFaceMood(uint32_t nowMs) const;

  void renderFrame();
  void presentFrame();
  void clearFrame();
  void renderFloor();
  void renderFpsCounter();
  void renderSprites(uint32_t nowMs);
  void renderWorld();
  void renderWeapon(uint32_t nowMs);
  void renderColumn(
    int x,
    int lineHeight,
    int rawDrawStart,
    int drawStart,
    int drawEnd,
    uint8_t tile,
    int texX,
    float distance,
    bool side,
    uint8_t doorOpenAmount);
  void fillRect(int x0, int y0, int w, int h, uint16_t color565);
  void putPixel(int x, int y, uint16_t color565);
  uint16_t shadeColor(uint16_t color565, float distance, bool side) const;
};
