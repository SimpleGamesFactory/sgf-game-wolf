#include "WolfAudio.h"

using namespace SGFAudio;

namespace {

constexpr PitchPoint kFirePitchEnv[] = {
  {0u, 500},
  {24u, -350},
  {60u, -1100},
};

constexpr Instrument kFireInstrument{
  .waveform = Waveform::Square,
  .ampEnv = {1u, 18u, 0u, 25u},
  .pitchLfo = {},
  .pitchEnv = kFirePitchEnv,
  .pitchEnvCount = static_cast<uint8_t>(sizeof(kFirePitchEnv) / sizeof(kFirePitchEnv[0])),
  .filterFlags = static_cast<uint8_t>(FilterLowPass),
  .lowPassCutoffHz = 1700.0f,
  .highPassCutoffHz = 0.0f,
  .volume = 215u,
};

constexpr SfxStep kFireSteps[] = {
  {70u, 0, 0, 255u, true, true},
};

constexpr Sfx kFireSfx{
  .instrument = &kFireInstrument,
  .steps = kFireSteps,
  .stepCount = static_cast<uint8_t>(sizeof(kFireSteps) / sizeof(kFireSteps[0])),
};

constexpr PitchPoint kHitPitchEnv[] = {
  {0u, 120},
  {25u, -180},
};

constexpr Instrument kHitInstrument{
  .waveform = Waveform::Triangle,
  .ampEnv = {0u, 40u, 0u, 45u},
  .pitchLfo = {},
  .pitchEnv = kHitPitchEnv,
  .pitchEnvCount = static_cast<uint8_t>(sizeof(kHitPitchEnv) / sizeof(kHitPitchEnv[0])),
  .filterFlags = static_cast<uint8_t>(FilterLowPass | FilterHighPass),
  .lowPassCutoffHz = 1300.0f,
  .highPassCutoffHz = 120.0f,
  .volume = 190u,
};

constexpr SfxStep kHitSteps[] = {
  {85u, -12, 0, 255u, true, true},
};

constexpr Sfx kHitSfx{
  .instrument = &kHitInstrument,
  .steps = kHitSteps,
  .stepCount = static_cast<uint8_t>(sizeof(kHitSteps) / sizeof(kHitSteps[0])),
};

constexpr Instrument kPickupInstrument{
  .waveform = Waveform::Sine,
  .ampEnv = {0u, 20u, 0u, 20u},
  .pitchLfo = {},
  .pitchEnv = nullptr,
  .pitchEnvCount = 0u,
  .filterFlags = static_cast<uint8_t>(FilterLowPass),
  .lowPassCutoffHz = 3200.0f,
  .highPassCutoffHz = 0.0f,
  .volume = 220u,
};

constexpr SfxStep kPickupSteps[] = {
  {45u, 0, 0, 255u, true, true},
  {45u, 4, 0, 255u, true, true},
  {45u, 7, 0, 235u, true, true},
};

constexpr Sfx kPickupSfx{
  .instrument = &kPickupInstrument,
  .steps = kPickupSteps,
  .stepCount = static_cast<uint8_t>(sizeof(kPickupSteps) / sizeof(kPickupSteps[0])),
};

constexpr PitchPoint kDoorPitchEnv[] = {
  {0u, 0},
  {120u, -90},
};

constexpr Instrument kDoorInstrument{
  .waveform = Waveform::Triangle,
  .ampEnv = {3u, 120u, 110u, 70u},
  .pitchLfo = {},
  .pitchEnv = kDoorPitchEnv,
  .pitchEnvCount = static_cast<uint8_t>(sizeof(kDoorPitchEnv) / sizeof(kDoorPitchEnv[0])),
  .filterFlags = static_cast<uint8_t>(FilterLowPass),
  .lowPassCutoffHz = 1200.0f,
  .highPassCutoffHz = 0.0f,
  .volume = 245u,
};

constexpr SfxStep kDoorSteps[] = {
  {110u, -12, 0, 210u, true, true},
  {110u, -10, -20, 170u, true, false},
};

constexpr Sfx kDoorSfx{
  .instrument = &kDoorInstrument,
  .steps = kDoorSteps,
  .stepCount = static_cast<uint8_t>(sizeof(kDoorSteps) / sizeof(kDoorSteps[0])),
};

}  // namespace

WolfAudio::WolfAudio()
  : synthEngine(11025u) {
  synthEngine.setMasterVolume(180u);
}

void WolfAudio::playFire() {
  synthEngine.playSfx(kFireSfx, 740.0f, 255u);
}

void WolfAudio::playHit() {
  synthEngine.playSfx(kHitSfx, 220.0f, 255u);
}

void WolfAudio::playPickup() {
  synthEngine.playSfx(kPickupSfx, 660.0f, 255u);
}

void WolfAudio::playDoorOpen() {
  synthEngine.playSfx(kDoorSfx, 196.0f, 255u);
}
