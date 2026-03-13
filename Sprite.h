#pragma once

#include <stdint.h>

namespace WolfRender {

class ISprite {
public:
  virtual ~ISprite() = default;

  virtual float worldX() const = 0;
  virtual float worldY() const = 0;
  virtual bool isRenderable() const = 0;
  virtual int texSize() const = 0;
  virtual int widthScaleNum() const = 0;
  virtual int widthScaleDen() const = 0;
  virtual int floorOffsetDiv() const = 0;
  virtual float maxRenderDistance() const = 0;
  virtual void buildTexture(uint16_t* outTexture, uint32_t nowMs) const = 0;
};

class Sprite : public ISprite {
public:
  using TextureBuilder = void (*)(const void* owner, uint16_t* outTexture, uint32_t nowMs);

  void clear();
  void configure(
    float posX,
    float posY,
    int textureSize,
    int widthNum,
    int widthDen,
    int floorDiv,
    float maxDistanceValue,
    const void* ownerPtr,
    TextureBuilder textureBuilder
  );
  void setPosition(float posX, float posY);

  float worldX() const override { return x; }
  float worldY() const override { return y; }
  bool isRenderable() const override { return renderable && builder != nullptr && textureSize > 0; }
  int texSize() const override { return textureSize; }
  int widthScaleNum() const override { return widthNum; }
  int widthScaleDen() const override { return widthDen; }
  int floorOffsetDiv() const override { return floorDiv; }
  float maxRenderDistance() const override { return maxDistanceValue; }
  void buildTexture(uint16_t* outTexture, uint32_t nowMs) const override;

private:
  float x = 0.0f;
  float y = 0.0f;
  int textureSize = 0;
  int widthNum = 1;
  int widthDen = 1;
  int floorDiv = 0;
  float maxDistanceValue = 0.0f;
  const void* owner = nullptr;
  TextureBuilder builder = nullptr;
  bool renderable = false;
};

}  // namespace WolfRender
