#include "AudioSamples.h"

#include <string.h>

#include "AudioSamplesGenerated.h"

namespace WolfAudioSamples {

namespace {

constexpr int8_t kSnarePcm[] = {
   92, -87,  79, -74,  68, -63,  58, -52,
   48, -44,  39, -35,  31, -28,  24, -22,
   54, -51,  46, -42,  38, -34,  31, -28,
   25, -23,  20, -18,  16, -15,  13, -12,
   36, -33,  29, -27,  24, -22,  20, -18,
   17, -15,  14, -13,  11, -10,   9,  -8,
   23, -21,  19, -17,  15, -14,  13, -12,
   10,  -9,   8,  -7,   6,  -6,   5,  -5,
   14, -13,  12, -11,  10,  -9,   8,  -7,
    6,  -6,   5,  -5,   4,  -4,   3,  -3,
    8,  -8,   7,  -7,   6,  -5,   5,  -4,
    4,  -4,   3,  -3,   2,  -2,   2,  -2,
};

}  // namespace

const SGFAudio::AudioSample kSnare{
  .pcm = kSnarePcm,
  .length = sizeof(kSnarePcm) / sizeof(kSnarePcm[0]),
  .sampleRate = 11025u,
  .rootHz = 1.0f,
  .loop = false,
  .loopStart = 0u,
  .loopEnd = 0u,
};

const SGFAudio::AudioSample* find(const char* code) {
  if (code == nullptr || *code == '\0') {
    return nullptr;
  }
  for (unsigned int i = 0; i < AudioSamplesGenerated::ENTRY_COUNT; ++i) {
    const AudioSamplesGenerated::Entry& entry = AudioSamplesGenerated::ENTRIES[i];
    if (entry.code != nullptr && strcmp(entry.code, code) == 0) {
      return entry.sample;
    }
  }
  return nullptr;
}

}  // namespace WolfAudioSamples
