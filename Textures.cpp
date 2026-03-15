#include "Textures.h"

#include <stddef.h>
#include <stdint.h>
#include <string.h>

#if defined(ARDUINO_ARCH_ESP32)
#include <esp_heap_caps.h>
#endif

#include "SGF/Color565.h"
#include "TexturesGenerated.h"

namespace Textures {

namespace {

constexpr uint16_t BLOB_MAGIC = 0x5754u;

struct Entry {
  Asset asset{};
  const uint8_t* indices = nullptr;
  uint16_t texelCount = 0;
};

Entry* entries = nullptr;
uint16_t entryCount = 0;
bool catalogReady = false;
const char* lastLookupName = nullptr;
Entry* lastLookupEntry = nullptr;

bool usesTransparentIndex0(const char* name) {
  if (name == nullptr) {
    return false;
  }
  return strncmp(name, "sprite_", 7) == 0 || strncmp(name, "weapon_", 7) == 0;
}

uint16_t readU16(const uint8_t*& cursor, const uint8_t* end) {
  if (cursor + 2 > end) {
    return 0u;
  }
  uint16_t value = cursor[0] | (cursor[1] << 8);
  cursor += 2;
  return value;
}

int clampCoord(int value) {
  if (value < 0) {
    return 0;
  }
  if (value >= TEX_SIZE) {
    return TEX_SIZE - 1;
  }
  return value;
}

uint16_t decodePaletteIndex(const Entry& entry, uint8_t paletteIndex) {
  if (usesTransparentIndex0(entry.asset.name) && paletteIndex == 0u) {
    return 0;
  }
  if (paletteIndex >= PALETTE_SIZE) {
    return 0;
  }
  const uint8_t* rgb = &TexturesGenerated::PALETTE_RGB[paletteIndex * 3];
  return Color565::rgb(rgb[0], rgb[1], rgb[2]);
}

void* textureAlloc(size_t bytes) {
#if defined(ARDUINO_ARCH_ESP32)
  return heap_caps_malloc(bytes, MALLOC_CAP_INTERNAL | MALLOC_CAP_8BIT);
#else
  return malloc(bytes);
#endif
}

void ensureCatalog() {
  if (catalogReady) {
    return;
  }

  const uint8_t* cursor = TexturesGenerated::BLOB;
  const uint8_t* end = TexturesGenerated::BLOB + TexturesGenerated::BLOB_SIZE;
  if (cursor + 4 > end) {
    catalogReady = true;
    return;
  }

  uint16_t magic = readU16(cursor, end);
  if (magic != BLOB_MAGIC) {
    catalogReady = true;
    return;
  }

  entryCount = readU16(cursor, end);
  if (entryCount == 0u) {
    catalogReady = true;
    return;
  }

  entries = static_cast<Entry*>(textureAlloc(sizeof(Entry) * entryCount));
  if (entries == nullptr) {
    entryCount = 0u;
    catalogReady = true;
    return;
  }
  memset(entries, 0, sizeof(Entry) * entryCount);

  uint16_t parsedCount = 0u;
  while (parsedCount < entryCount && cursor < end) {
    uint16_t nameLen = readU16(cursor, end);
    if (nameLen == 0u || cursor + nameLen > end) {
      break;
    }
    const char* name = reinterpret_cast<const char*>(cursor);
    cursor += nameLen;

    uint16_t texelCount = readU16(cursor, end);
    if (texelCount == 0u || cursor + texelCount > end) {
      break;
    }

    entries[parsedCount].asset.name = name;
    entries[parsedCount].indices = cursor;
    entries[parsedCount].texelCount = texelCount;
    cursor += texelCount;
    parsedCount++;
  }

  entryCount = parsedCount;
  catalogReady = true;
}

void ensureDecoded(Entry& entry) {
  if (entry.asset.pixels != nullptr || entry.indices == nullptr || entry.texelCount == 0u) {
    return;
  }

  uint16_t* decoded = static_cast<uint16_t*>(textureAlloc(sizeof(uint16_t) * entry.texelCount));
  if (decoded == nullptr) {
    return;
  }

  for (uint16_t i = 0; i < entry.texelCount; i++) {
    decoded[i] = decodePaletteIndex(entry, entry.indices[i]);
  }
  entry.asset.pixels = decoded;
}

Entry* findEntry(const char* name) {
  ensureCatalog();
  if (name == nullptr || name[0] == '\0' || entryCount == 0u || entries == nullptr) {
    return nullptr;
  }

  if (lastLookupName == name && lastLookupEntry != nullptr) {
    return lastLookupEntry;
  }

  for (uint16_t i = 0; i < entryCount; i++) {
    const char* candidate = entries[i].asset.name;
    if (candidate != nullptr && strcmp(candidate, name) == 0) {
      lastLookupName = name;
      lastLookupEntry = &entries[i];
      return &entries[i];
    }
  }
  return nullptr;
}

}  // namespace

const Asset* find(const char* name) {
  Entry* entry = findEntry(name);
  if (entry == nullptr) {
    return nullptr;
  }
  ensureDecoded(*entry);
  return (entry->asset.pixels != nullptr) ? &entry->asset : nullptr;
}

const uint16_t* pixels(const char* name) {
  const Asset* asset = find(name);
  return asset ? asset->pixels : nullptr;
}

uint16_t texel(const char* name, int texX, int texY, uint16_t fallback565) {
  const uint16_t* tex = pixels(name);
  if (tex == nullptr) {
    return fallback565;
  }

  texX = clampCoord(texX);
  texY = clampCoord(texY);
  return tex[texY * TEX_SIZE + texX];
}

}  // namespace Textures
