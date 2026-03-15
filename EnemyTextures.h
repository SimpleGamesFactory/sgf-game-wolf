#pragma once

#include <stdint.h>

class Enemy;

namespace EnemyTextures {

void build(const Enemy& enemy, uint16_t* outTexture, uint32_t nowMs);

}  // namespace EnemyTextures
