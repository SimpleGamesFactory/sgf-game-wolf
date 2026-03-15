#include "EnemyTextures.h"

#include <string.h>

#include "SGF/Color565.h"
#include "SGF/Math.h"
#include "Textures.h"
#include "Enemy.h"

namespace {

uint16_t applyAttackTint(uint16_t color565) {
  uint8_t r5 = (color565 >> 11) & 0x1F;
  uint8_t g6 = (color565 >> 5) & 0x3F;
  uint8_t b5 = color565 & 0x1F;
  uint8_t r = (r5 * 255) / 31;
  uint8_t g = (g6 * 255) / 63;
  uint8_t b = (b5 * 255) / 31;

  r = Math::clamp(r + 56, 0, 255);
  g = (g * 3) / 4;
  b = (b * 3) / 4;
  return Color565::rgb(r, g, b);
}

}  // namespace

namespace EnemyTextures {

void build(const Enemy& enemy, uint16_t* outTexture, uint32_t nowMs) {
  const char* textureName = enemy.isAlive()
                              ? enemy.aliveTextureName
                              : enemy.deadTextureName;
  const uint16_t* bmpTexture = Textures::pixels(textureName);
  if (bmpTexture != nullptr) {
    memcpy(outTexture, bmpTexture, Enemy::TEX_SIZE * Enemy::TEX_SIZE * sizeof(uint16_t));
    if (enemy.isAlive() && enemy.isAttacking(nowMs)) {
      for (int i = 0; i < Enemy::TEX_SIZE * Enemy::TEX_SIZE; i++) {
        if (outTexture[i] != 0) {
          outTexture[i] = applyAttackTint(outTexture[i]);
        }
      }
    }
    return;
  }

  for (int texY = 0; texY < Enemy::TEX_SIZE; texY++) {
    for (int texX = 0; texX < Enemy::TEX_SIZE; texX++) {
      if (!enemy.isAlive()) {
        bool body =
          (texY >= 8 && texY <= 11 && texX >= 2 && texX <= 13) ||
          (texY >= 11 && texY <= 13 && texX >= 4 && texX <= 11) ||
          (texY == 7 && texX >= 4 && texX <= 6);
        bool head = (texY >= 6 && texY <= 8 && texX >= 11 && texX <= 13);
        bool blood = (texY == 12 && texX >= 11 && texX <= 13);
        if (blood) {
          outTexture[texY * Enemy::TEX_SIZE + texX] = Color565::rgb(132, 28, 20);
        } else if (head) {
          outTexture[texY * Enemy::TEX_SIZE + texX] = Color565::rgb(118, 152, 98);
        } else if (body) {
          outTexture[texY * Enemy::TEX_SIZE + texX] = Color565::rgb(74, 92, 122);
        } else {
          outTexture[texY * Enemy::TEX_SIZE + texX] = 0;
        }
      } else {
        outTexture[texY * Enemy::TEX_SIZE + texX] = enemy.texel(texX, texY, nowMs);
      }
    }
  }
}

}  // namespace EnemyTextures
