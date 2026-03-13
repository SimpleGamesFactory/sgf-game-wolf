#include "Sprite.h"

namespace WolfRender {

void Sprite::clear() {
  x = 0.0f;
  y = 0.0f;
  textureSize = 0;
  widthNum = 1;
  widthDen = 1;
  floorDiv = 0;
  maxDistanceValue = 0.0f;
  owner = nullptr;
  builder = nullptr;
  renderable = false;
}

void Sprite::configure(
  float posX,
  float posY,
  int textureSizeValue,
  int widthNumValue,
  int widthDenValue,
  int floorDivValue,
  float maxDistance,
  const void* ownerPtr,
  TextureBuilder textureBuilder
) {
  x = posX;
  y = posY;
  textureSize = textureSizeValue;
  widthNum = widthNumValue;
  widthDen = widthDenValue;
  floorDiv = floorDivValue;
  maxDistanceValue = maxDistance;
  owner = ownerPtr;
  builder = textureBuilder;
  renderable = true;
}

void Sprite::setPosition(float posX, float posY) {
  x = posX;
  y = posY;
}

void Sprite::buildTexture(uint16_t* outTexture, uint32_t nowMs) const {
  if (builder == nullptr) {
    return;
  }
  builder(owner, outTexture, nowMs);
}

}  // namespace WolfRender
