#pragma once

class Enemy;

namespace EnemyCatalog {

bool isMapSymbol(char symbol);
bool spawnFromMapSymbol(Enemy& enemy, char symbol, float x, float y);

}  // namespace EnemyCatalog
