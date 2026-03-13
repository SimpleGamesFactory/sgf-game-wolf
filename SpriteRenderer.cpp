#include "SpriteRenderer.h"

#include <math.h>

#include "SGF/Color565.h"
#include "SGF/Math.h"

namespace SpriteRenderer {
namespace {

uint16_t shadeColor(uint16_t color565, float distance) {
  float brightness = 1.0f / (1.0f + distance * 0.18f);
  brightness = Math::clamp(brightness, 0.18f, 1.0f);

  uint8_t r5 = (color565 >> 11) & 0x1F;
  uint8_t g6 = (color565 >> 5) & 0x3F;
  uint8_t b5 = color565 & 0x1F;
  uint8_t r = static_cast<uint8_t>((static_cast<float>(r5) * 255.0f / 31.0f) * brightness);
  uint8_t g = static_cast<uint8_t>((static_cast<float>(g6) * 255.0f / 63.0f) * brightness);
  uint8_t b = static_cast<uint8_t>((static_cast<float>(b5) * 255.0f / 31.0f) * brightness);
  return Color565::rgb(r, g, b);
}

}  // namespace

void render(const RenderView& view, const WolfRender::ISprite* const* sprites, int spriteCount) {
  if (view.frameBuffer == nullptr || view.wallDepth == nullptr || spriteCount <= 0) {
    return;
  }

  int order[MAX_SPRITES]{};
  float distances[MAX_SPRITES]{};
  int renderCount = 0;
  for (int i = 0; i < spriteCount && i < MAX_SPRITES; i++) {
    const WolfRender::ISprite* sprite = sprites[i];
    if (sprite == nullptr || !sprite->isRenderable()) {
      continue;
    }

    float dx = sprite->worldX() - view.playerX;
    float dy = sprite->worldY() - view.playerY;
    float distanceSq = dx * dx + dy * dy;
    float maxDistance = sprite->maxRenderDistance();
    if (maxDistance > 0.0f && distanceSq > maxDistance * maxDistance) {
      continue;
    }

    order[renderCount] = i;
    distances[renderCount] = distanceSq;
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

  float invDet = 1.0f / (view.planeX * view.dirY - view.dirX * view.planeY);
  for (int i = 0; i < renderCount; i++) {
    const WolfRender::ISprite& sprite = *sprites[order[i]];
    int texSize = sprite.texSize();
    if (texSize <= 0 || texSize * texSize > MAX_TEXELS) {
      continue;
    }

    float spriteX = sprite.worldX() - view.playerX;
    float spriteY = sprite.worldY() - view.playerY;
    float transformX = invDet * (view.dirY * spriteX - view.dirX * spriteY);
    float transformY = invDet * (-view.planeY * spriteX + view.planeX * spriteY);
    if (transformY <= 0.15f) {
      continue;
    }

    int spriteScreenX =
      static_cast<int>((static_cast<float>(view.width) * 0.5f) * (1.0f + transformX / transformY));
    int spriteHeight = abs(static_cast<int>(static_cast<float>(view.height) / transformY));
    int spriteWidth =
      (spriteHeight * Math::clamp(sprite.widthScaleNum(), 1, 16)) /
      Math::clamp(sprite.widthScaleDen(), 1, 16);
    if (spriteWidth < 4) {
      spriteWidth = 4;
    }

    int floorDiv = sprite.floorOffsetDiv();
    int floorOffset = (floorDiv > 0) ? (spriteHeight / floorDiv) : 0;
    int rawDrawEndY = (view.height / 2) + (spriteHeight / 2) + floorOffset;
    int rawDrawStartY = rawDrawEndY - spriteHeight + 1;
    int drawStartY = Math::clamp(rawDrawStartY, 0, view.height - 1);
    int drawEndY = Math::clamp(rawDrawEndY, 0, view.height - 1);

    int rawDrawStartX = spriteScreenX - spriteWidth / 2;
    int rawDrawEndX = spriteScreenX + spriteWidth / 2;
    int drawStartX = Math::clamp(rawDrawStartX, 0, view.width - 1);
    int drawEndX = Math::clamp(rawDrawEndX, 0, view.width - 1);

    int texXStep = (texSize << 16) / Math::clamp(spriteWidth, 1, 1 << 14);
    int texYStep = (texSize << 16) / Math::clamp(spriteHeight, 1, 1 << 14);
    int texXPos = (drawStartX - rawDrawStartX) * texXStep;

    uint16_t texture[MAX_TEXELS];
    uint16_t shadedTexture[MAX_TEXELS];
    sprite.buildTexture(texture, view.nowMs);
    for (int texY = 0; texY < texSize; texY++) {
      for (int texX = 0; texX < texSize; texX++) {
        uint16_t color565 = texture[texY * texSize + texX];
        shadedTexture[texY * texSize + texX] =
          (color565 == 0) ? 0 : shadeColor(color565, transformY);
      }
    }

    for (int stripe = drawStartX; stripe <= drawEndX; stripe++) {
      if (transformY >= view.wallDepth[stripe]) {
        texXPos += texXStep;
        continue;
      }

      int texX = Math::clamp(texXPos >> 16, 0, texSize - 1);
      texXPos += texXStep;
      int texYPos = (drawStartY - rawDrawStartY) * texYStep;

      for (int y = drawStartY; y <= drawEndY; y++) {
        int texY = Math::clamp(texYPos >> 16, 0, texSize - 1);
        texYPos += texYStep;
        uint16_t color565 = shadedTexture[texY * texSize + texX];
        if (color565 == 0) {
          continue;
        }
        view.frameBuffer[y * view.width + stripe] = color565;
      }
    }
  }
}

}  // namespace SpriteRenderer
