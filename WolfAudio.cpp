#include "WolfAudio.h"

using namespace SGFAudio;

namespace {

constexpr int kSfxVoiceCount = 2;
constexpr int kLeadVoice = 2;
constexpr int kBassVoice = 3;
constexpr int kHatVoice = 4;
constexpr int kSnareVoice = 5;

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

constexpr Instrument kMusicInstrument{
  .waveform = Waveform::Sine,
  .ampEnv = {10u, 40u, 180u, 50u},
  .pitchLfo = {},
  .pitchEnv = nullptr,
  .pitchEnvCount = 0u,
  .filterFlags = static_cast<uint8_t>(FilterLowPass),
  .lowPassCutoffHz = 1800.0f,
  .highPassCutoffHz = 0.0f,
  .volume = 150u,
};

constexpr Instrument kBassInstrument{
  .waveform = Waveform::Saw,
  .ampEnv = {4u, 20u, 170u, 35u},
  .pitchLfo = {},
  .pitchEnv = nullptr,
  .pitchEnvCount = 0u,
  .filterFlags = FilterNone,
  .lowPassCutoffHz = 0.0f,
  .highPassCutoffHz = 0.0f,
  .volume = 118u,
};

constexpr Instrument kHatInstrument{
  .waveform = Waveform::Noise,
  .ampEnv = {0u, 8u, 0u, 12u},
  .pitchLfo = {},
  .pitchEnv = nullptr,
  .pitchEnvCount = 0u,
  .filterFlags = static_cast<uint8_t>(FilterHighPass),
  .lowPassCutoffHz = 0.0f,
  .highPassCutoffHz = 2600.0f,
  .volume = 92u,
};

constexpr Instrument kSnareInstrument{
  .waveform = Waveform::Noise,
  .ampEnv = {0u, 18u, 0u, 28u},
  .pitchLfo = {},
  .pitchEnv = nullptr,
  .pitchEnvCount = 0u,
  .filterFlags = static_cast<uint8_t>(FilterLowPass | FilterHighPass),
  .lowPassCutoffHz = 2200.0f,
  .highPassCutoffHz = 700.0f,
  .volume = 124u,
};

constexpr uint16_t kBaseStepMs = 180u;

constexpr PatternStep kLeadPatternSteps[] = {
  {261.63f, 1u, 170u},  // C4
  {261.63f, 1u, 170u},
  {329.63f, 1u, 170u},  // E4
  {329.63f, 1u, 170u},
  {349.23f, 1u, 170u},  // F4
  {349.23f, 1u, 170u},
  {369.99f, 1u, 170u},  // F#4
  {392.00f, 1u, 170u},  // G4
  {261.63f, 1u, 170u},
  {261.63f, 1u, 170u},
  {329.63f, 1u, 170u},
  {329.63f, 1u, 170u},
  {349.23f, 1u, 170u},
  {349.23f, 1u, 170u},
  {369.99f, 1u, 170u},
  {392.00f, 1u, 170u},
  {261.63f, 1u, 170u},
  {261.63f, 1u, 170u},
  {329.63f, 1u, 170u},
  {329.63f, 1u, 170u},
  {349.23f, 1u, 170u},
  {349.23f, 1u, 170u},
  {369.99f, 1u, 170u},
  {392.00f, 1u, 170u},
  {261.63f, 1u, 170u},
  {261.63f, 1u, 170u},
  {329.63f, 1u, 170u},
  {329.63f, 1u, 170u},
  {349.23f, 1u, 170u},
  {349.23f, 1u, 170u},
  {369.99f, 1u, 170u},
  {392.00f, 1u, 170u},
  {349.23f, 1u, 170u},  // F4
  {349.23f, 1u, 170u},
  {440.00f, 1u, 170u},  // A4
  {440.00f, 1u, 170u},
  {466.16f, 1u, 170u},  // A#4
  {466.16f, 1u, 170u},
  {493.88f, 1u, 170u},  // B4
  {523.25f, 1u, 170u},  // C5
  {349.23f, 1u, 170u},
  {349.23f, 1u, 170u},
  {440.00f, 1u, 170u},
  {440.00f, 1u, 170u},
  {466.16f, 1u, 170u},
  {466.16f, 1u, 170u},
  {493.88f, 1u, 170u},
  {523.25f, 1u, 170u},
  {349.23f, 1u, 170u},
  {349.23f, 1u, 170u},
  {440.00f, 1u, 170u},
  {440.00f, 1u, 170u},
  {466.16f, 1u, 170u},
  {466.16f, 1u, 170u},
  {493.88f, 1u, 170u},
  {523.25f, 1u, 170u},
  {349.23f, 1u, 170u},
  {349.23f, 1u, 170u},
  {440.00f, 1u, 170u},
  {440.00f, 1u, 170u},
  {466.16f, 1u, 170u},
  {466.16f, 1u, 170u},
  {493.88f, 1u, 170u},
  {523.25f, 1u, 170u},
};

constexpr Pattern kLeadPattern{
  .steps = kLeadPatternSteps,
  .stepCount = static_cast<uint16_t>(sizeof(kLeadPatternSteps) / sizeof(kLeadPatternSteps[0])),
  .unitMs = kBaseStepMs,
  .loop = true,
};

constexpr PatternStep kBassPatternSteps[] = {
  {65.41f, 4u, 132u},   // C2
  {65.41f, 1u, 132u},
  {0.0f,   27u, 0u},    // pause
  {87.31f, 4u, 132u},   // F2
  {87.31f, 1u, 132u},
  {0.0f,   27u, 0u},
};

constexpr Pattern kBassPattern{
  .steps = kBassPatternSteps,
  .stepCount = static_cast<uint16_t>(sizeof(kBassPatternSteps) / sizeof(kBassPatternSteps[0])),
  .unitMs = kBaseStepMs,
  .loop = true,
};

constexpr PatternStep kHatPatternSteps[] = {
  {1.0f, 1u, 220u},  // H
  {1.0f, 1u, 132u},  // h
  {1.0f, 1u, 220u},
  {1.0f, 1u, 132u},
  {1.0f, 1u, 220u},
  {1.0f, 1u, 132u},
  {1.0f, 1u, 220u},
  {1.0f, 1u, 132u},
};

constexpr Pattern kHatPattern{
  .steps = kHatPatternSteps,
  .stepCount = static_cast<uint16_t>(sizeof(kHatPatternSteps) / sizeof(kHatPatternSteps[0])),
  .unitMs = kBaseStepMs,
  .loop = true,
};

constexpr PatternStep kSnarePatternSteps[] = {
  {0.0f, 4u, 0u},    // P
  {1.0f, 4u, 220u},  // S
};

constexpr Pattern kSnarePattern{
  .steps = kSnarePatternSteps,
  .stepCount = static_cast<uint16_t>(sizeof(kSnarePatternSteps) / sizeof(kSnarePatternSteps[0])),
  .unitMs = kBaseStepMs,
  .loop = true,
};

}  // namespace

WolfAudio::WolfAudio()
  : synthEngine(11025u),
    leadTrack(synthEngine, kLeadVoice, kMusicInstrument, kLeadPattern),
    bassTrack(synthEngine, kBassVoice, kBassInstrument, kBassPattern),
    hatTrack(synthEngine, kHatVoice, kHatInstrument, kHatPattern),
    snareTrack(synthEngine, kSnareVoice, kSnareInstrument, kSnarePattern) {
  synthEngine.setMasterVolume(180u);
}

#if WOLF_AUDIO_DIAG_TONE
int16_t WolfAudio::renderSample() {
  if (!synthEngine.voiceActive(kLeadVoice)) {
    synthEngine.noteOn(kLeadVoice, kMusicInstrument, 261.63f, 200u);
  }
  return synthEngine.renderSample();
#else
int16_t WolfAudio::renderSample() {
  leadTrack.tick();
  bassTrack.tick();
  hatTrack.tick();
  snareTrack.tick();
  return synthEngine.renderSample();
}
#endif

void WolfAudio::playSfx(const Sfx& sfx, float baseHz, uint8_t velocity) {
#if WOLF_AUDIO_DIAG_TONE
  (void)sfx;
  (void)baseHz;
  (void)velocity;
  return;
#else
  synthEngine.triggerSfx(nextSfxVoice, sfx, baseHz, velocity);
  nextSfxVoice = static_cast<uint8_t>((nextSfxVoice + 1u) % kSfxVoiceCount);
#endif
}

void WolfAudio::playFire() {
  playSfx(kFireSfx, 740.0f, 255u);
}

void WolfAudio::playHit() {
  playSfx(kHitSfx, 220.0f, 255u);
}

void WolfAudio::playPickup() {
  playSfx(kPickupSfx, 660.0f, 255u);
}

void WolfAudio::playDoorOpen() {
  playSfx(kDoorSfx, 196.0f, 255u);
}
