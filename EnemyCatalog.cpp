#include "EnemyCatalog.h"

#include "Enemy.h"

namespace {

constexpr char ZOMBIE_SYMBOL = '@';
constexpr char GHOST_SYMBOL = '&';

void configureZombie(Enemy& enemy) {
  enemy.respawnMode = Enemy::RespawnMode::None;
  enemy.attackMode = Enemy::AttackMode::Ranged;
  enemy.stopRange = 1.45f;
  enemy.moveSpeed = 1.08f;
  enemy.attackRange = 4.5f;
  enemy.maxHp = 18;
  enemy.leavesCorpse = true;
  enemy.aliveTextureName = "sprite_zombie";
  enemy.deadTextureName = "sprite_zombie_dead";
  enemy.attackSampleId = "zombie_fire";
  enemy.dieSampleId = "zombie_die";
}

void configureGhost(Enemy& enemy) {
  enemy.respawnMode = Enemy::RespawnMode::Auto;
  enemy.attackMode = Enemy::AttackMode::Touch;
  enemy.stopRange = 0.55f;
  enemy.moveSpeed = 0.72f;
  enemy.attackRange = 0.75f;
  enemy.maxHp = 36;
  enemy.respawnDelayMs = 1800u;
  enemy.aliveTextureName = "sprite_ghost";
  enemy.attackSampleId = "ghost_attack";
  enemy.dieSampleId = "ghost_die";
}

}  // namespace

namespace EnemyCatalog {

bool isMapSymbol(char symbol) {
  return symbol == ZOMBIE_SYMBOL || symbol == GHOST_SYMBOL;
}

bool spawnFromMapSymbol(Enemy& enemy, char symbol, float x, float y) {
  enemy.clear();
  switch (symbol) {
    case ZOMBIE_SYMBOL:
      configureZombie(enemy);
      enemy.spawn(x, y);
      return true;
    case GHOST_SYMBOL:
      configureGhost(enemy);
      enemy.spawn(x, y);
      return true;
    default:
      return false;
  }
}

}  // namespace EnemyCatalog
