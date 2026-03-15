#include "WolfAudio.h"

#include "AudioSamples.h"
#include <string.h>

using namespace SGFAudio;

namespace {

template <typename T, size_t N>
constexpr uint8_t count8(const T (&)[N]) {
  static_assert(N <= 255u);
  return N;
}

template <typename T, size_t N>
constexpr uint16_t count16(const T (&)[N]) {
  static_assert(N <= 65535u);
  return N;
}

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
  .pitchEnv = kFirePitchEnv,
  .pitchEnvCount = count8(kFirePitchEnv),
  .filterFlags = AUDIO_FILTER_LP,
  .lowPassCutoffHz = 1700.0f,
  .volume = 215u,
};

constexpr SfxStep kFireSteps[] = {
  {70u, 0, 0, 255u, true, true},
};

constexpr Sfx kFireSfx{
  .instrument = &kFireInstrument,
  .steps = kFireSteps,
  .stepCount = count8(kFireSteps),
};

constexpr PitchPoint kHitPitchEnv[] = {
  {0u, 120},
  {25u, -180},
};

constexpr Instrument kHitInstrument{
  .waveform = Waveform::Triangle,
  .ampEnv = {0u, 40u, 0u, 45u},
  .pitchEnv = kHitPitchEnv,
  .pitchEnvCount = count8(kHitPitchEnv),
  .filterFlags = AUDIO_FILTER_LP | AUDIO_FILTER_HP,
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
  .stepCount = count8(kHitSteps),
};

constexpr Instrument kPickupInstrument{
  .waveform = Waveform::Sine,
  .ampEnv = {0u, 20u, 0u, 20u},
  .filterFlags = AUDIO_FILTER_LP,
  .lowPassCutoffHz = 3200.0f,
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
  .stepCount = count8(kPickupSteps),
};

constexpr PitchPoint kDoorPitchEnv[] = {
  {0u, 0},
  {120u, -90},
};

constexpr Instrument kDoorInstrument{
  .waveform = Waveform::Triangle,
  .ampEnv = {3u, 120u, 110u, 70u},
  .pitchEnv = kDoorPitchEnv,
  .pitchEnvCount = count8(kDoorPitchEnv),
  .filterFlags = AUDIO_FILTER_LP,
  .lowPassCutoffHz = 1200.0f,
  .volume = 245u,
};

constexpr SfxStep kDoorSteps[] = {
  {110u, -12, 0, 210u, true, true},
  {110u, -10, -20, 170u, true, false},
};

constexpr Sfx kDoorSfx{
  .instrument = &kDoorInstrument,
  .steps = kDoorSteps,
  .stepCount = count8(kDoorSteps),
};

constexpr PitchPoint kZombieFirePitchEnv[] = {
  {0u, 180},
  {55u, -120},
};

constexpr Instrument kZombieFireInstrument{
  .waveform = Waveform::Square,
  .ampEnv = {0u, 24u, 0u, 24u},
  .pitchEnv = kZombieFirePitchEnv,
  .pitchEnvCount = count8(kZombieFirePitchEnv),
  .filterFlags = AUDIO_FILTER_LP | AUDIO_FILTER_HP,
  .lowPassCutoffHz = 1500.0f,
  .highPassCutoffHz = 160.0f,
  .volume = 180u,
};

constexpr SfxStep kZombieFireSteps[] = {
  {65u, 0, 0, 255u, true, true},
};

constexpr Sfx kZombieFireSfx{
  .instrument = &kZombieFireInstrument,
  .steps = kZombieFireSteps,
  .stepCount = count8(kZombieFireSteps),
};

constexpr PitchPoint kZombieDiePitchEnv[] = {
  {0u, 0},
  {80u, -420},
  {180u, -860},
};

constexpr Instrument kZombieDieInstrument{
  .waveform = Waveform::Noise,
  .ampEnv = {0u, 70u, 0u, 90u},
  .pitchEnv = kZombieDiePitchEnv,
  .pitchEnvCount = count8(kZombieDiePitchEnv),
  .filterFlags = AUDIO_FILTER_LP | AUDIO_FILTER_HP,
  .lowPassCutoffHz = 1800.0f,
  .highPassCutoffHz = 110.0f,
  .volume = 190u,
};

constexpr SfxStep kZombieDieSteps[] = {
  {180u, -8, 0, 255u, true, true},
};

constexpr Sfx kZombieDieSfx{
  .instrument = &kZombieDieInstrument,
  .steps = kZombieDieSteps,
  .stepCount = count8(kZombieDieSteps),
};

constexpr PitchPoint kGhostAttackPitchEnv[] = {
  {0u, 240},
  {90u, -160},
  {180u, -420},
};

constexpr Instrument kGhostAttackInstrument{
  .waveform = Waveform::Triangle,
  .ampEnv = {0u, 90u, 0u, 80u},
  .pitchLfo = {.enabled = true, .waveform = Waveform::Sine, .rateHz = 6.0f, .depthCents = 18.0f},
  .pitchEnv = kGhostAttackPitchEnv,
  .pitchEnvCount = count8(kGhostAttackPitchEnv),
  .filterFlags = AUDIO_FILTER_LP | AUDIO_FILTER_HP,
  .lowPassCutoffHz = 1400.0f,
  .highPassCutoffHz = 120.0f,
  .volume = 180u,
};

constexpr SfxStep kGhostAttackSteps[] = {
  {180u, -12, 0, 255u, true, true},
};

constexpr Sfx kGhostAttackSfx{
  .instrument = &kGhostAttackInstrument,
  .steps = kGhostAttackSteps,
  .stepCount = count8(kGhostAttackSteps),
};

constexpr PitchPoint kGhostDiePitchEnv[] = {
  {0u, 220},
  {120u, -240},
  {260u, -720},
};

constexpr Instrument kGhostDieInstrument{
  .waveform = Waveform::Triangle,
  .ampEnv = {0u, 120u, 0u, 120u},
  .pitchLfo = {.enabled = true, .waveform = Waveform::Sine, .rateHz = 5.0f, .depthCents = 24.0f},
  .pitchEnv = kGhostDiePitchEnv,
  .pitchEnvCount = count8(kGhostDiePitchEnv),
  .filterFlags = AUDIO_FILTER_LP | AUDIO_FILTER_HP,
  .lowPassCutoffHz = 1200.0f,
  .highPassCutoffHz = 100.0f,
  .volume = 190u,
};

constexpr SfxStep kGhostDieSteps[] = {
  {260u, -10, 0, 255u, true, true},
};

constexpr Sfx kGhostDieSfx{
  .instrument = &kGhostDieInstrument,
  .steps = kGhostDieSteps,
  .stepCount = count8(kGhostDieSteps),
};

constexpr Instrument kMusicInstrument{
  .waveform = Waveform::Sine,
  .ampEnv = {10u, 40u, 180u, 50u},
  .filterFlags = AUDIO_FILTER_LP,
  .lowPassCutoffHz = 1800.0f,
  .volume = 75u,
};

constexpr Instrument kBassInstrument{
  .waveform = Waveform::Saw,
  .ampEnv = {4u, 20u, 170u, 35u},
  .filterFlags = AUDIO_FILTER_LP,
  .lowPassCutoffHz = 300.0f,
  .volume = 40u,
};

constexpr Instrument kHatInstrument{
  .waveform = Waveform::Noise,
  .ampEnv = {0u, 8u, 0u, 12u},
  .filterFlags = AUDIO_FILTER_HP,
  .highPassCutoffHz = 2600.0f,
  .volume = 66u,
};

constexpr Instrument kSnareInstrumentFallback{
  .waveform = Waveform::Noise,
  .ampEnv = {0u, 18u, 0u, 28u},
  .filterFlags = AUDIO_FILTER_LP | AUDIO_FILTER_HP,
  .lowPassCutoffHz = 2200.0f,
  .highPassCutoffHz = 700.0f,
  .volume = 32u,
};

constexpr SampleInstrument kSampleOneShotInstrument{};

constexpr SampleInstrument kSamplePitchedInstrument{
  .volume = 180u,
  .oneShot = false,
};

constexpr uint16_t kBaseStepMs = 180u;

constexpr PatternStep kLeadPatternASteps[] = {
  {261.63f, 1u, 170u},  // C4
  {261.63f, 1u, 170u},
  {329.63f, 1u, 170u},  // E4
  {329.63f, 1u, 170u},
  {349.23f, 1u, 170u},  // F4
  {349.23f, 1u, 170u},
  {369.99f, 1u, 170u},  // F#4
  {392.00f, 1u, 170u},  // G4
};

constexpr Pattern kLeadPatternA{
  .steps = kLeadPatternASteps,
  .stepCount = count16(kLeadPatternASteps),
  .unitMs = kBaseStepMs,
  .loop = false,
};

constexpr PatternStep kLeadPatternBSteps[] = {
  {349.23f, 1u, 170u},  // F4
  {349.23f, 1u, 170u},
  {440.00f, 1u, 170u},  // A4
  {440.00f, 1u, 170u},
  {466.16f, 1u, 170u},  // A#4
  {466.16f, 1u, 170u},
  {493.88f, 1u, 170u},  // B4
  {523.25f, 1u, 170u},  // C5
};

constexpr Pattern kLeadPatternB{
  .steps = kLeadPatternBSteps,
  .stepCount = count16(kLeadPatternBSteps),
  .unitMs = kBaseStepMs,
  .loop = false,
};

constexpr PatternStep kBassPatternSteps[] = {
  {41.20f, 1u, 132u},   // E1
  {41.20f, 1u, 132u},   // E1
  {43.65f, 1u, 132u},   // F1
  {41.20f, 1u, 132u},   // E1
  {0.0f,  1u, 0u},      // P
  {41.20f, 1u, 132u},   // E1
  {43.65f, 1u, 132u},   // F1
  {41.20f, 1u, 132u},   // E1
  {0.0f,  1u, 0u},      // P
  {41.20f, 1u, 132u},   // E1
  {43.65f, 1u, 132u},   // F1
  {41.20f, 1u, 132u},   // E1
  {36.71f, 1u, 132u},   // D1
  {30.87f, 1u, 132u},   // B0
  {36.71f, 1u, 132u},   // D1
  {41.20f, 1u, 132u},   // E1
};

constexpr Pattern kBassPattern{
  .steps = kBassPatternSteps,
  .stepCount = count16(kBassPatternSteps),
  .unitMs = kBaseStepMs,
  .loop = false,
};

constexpr PatternStep kBassPatternEndSteps[] = {
  {41.20f, 4u, 132u},   // E1
  {0.0f,  12u, 0u},     // P
};

constexpr Pattern kBassPatternEnd{
  .steps = kBassPatternEndSteps,
  .stepCount = count16(kBassPatternEndSteps),
  .unitMs = kBaseStepMs,
  .loop = false,
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
  .stepCount = count16(kHatPatternSteps),
  .unitMs = kBaseStepMs,
  .loop = false,
};

constexpr PatternStep kSnarePatternSteps[] = {
  {0.0f, 4u, 0u},    // P
  {1.0f, 4u, 220u},  // S
};

constexpr Pattern kSnarePattern{
  .steps = kSnarePatternSteps,
  .stepCount = count16(kSnarePatternSteps),
  .unitMs = kBaseStepMs,
  .loop = false,
};

constexpr SongClip kLeadClips[] = {
  {.pattern = &kLeadPatternA, .repeats = 4u},
  {.pattern = &kLeadPatternB, .repeats = 4u},
};

constexpr SongClip kBassClips[] = {
  {.pattern = &kBassPattern, .repeats = 4u},
  {.pattern = &kBassPatternEnd, .repeats = 4u},
};

constexpr SongClip kHatClips[] = {
  {.pattern = &kHatPattern, .repeats = 8u},
};

constexpr SongClip kSnareClips[] = {
  {.pattern = &kSnarePattern, .repeats = 8u},
};

}  // namespace

WolfAudio::WolfAudio()
  : mixer(11025u),
    synthEngine(11025u),
    samplePlayer(11025u),
    songPlayer() {
  mixer.setMasterVolume(255u);
  mixer.addSource(synthEngine);
  mixer.addSource(samplePlayer);
  synthEngine.setMasterVolume(180u);

  fireInstrument = kFireInstrument;
  hitInstrument = kHitInstrument;
  pickupInstrument = kPickupInstrument;
  doorInstrument = kDoorInstrument;
  zombieFireInstrument = kZombieFireInstrument;
  zombieDieInstrument = kZombieDieInstrument;
  ghostAttackInstrument = kGhostAttackInstrument;
  ghostDieInstrument = kGhostDieInstrument;
  leadInstrument = kMusicInstrument;
  bassInstrument = kBassInstrument;
  hatInstrument = kHatInstrument;
  snareInstrument = kSnareInstrumentFallback;
  fireSampleInstrument = kSampleOneShotInstrument;
  hitSampleInstrument = kSampleOneShotInstrument;
  pickupSampleInstrument = kSampleOneShotInstrument;
  doorSampleInstrument = kSampleOneShotInstrument;
  zombieFireSampleInstrument = kSampleOneShotInstrument;
  zombieDieSampleInstrument = kSampleOneShotInstrument;
  ghostAttackSampleInstrument = kSampleOneShotInstrument;
  ghostDieSampleInstrument = kSampleOneShotInstrument;
  leadSampleInstrument = kSamplePitchedInstrument;
  bassSampleInstrument = kSamplePitchedInstrument;
  hatSampleInstrument = kSampleOneShotInstrument;
  snareSampleInstrument = {
    .sample = &WolfAudioSamples::kSnare,
    .volume = kSnareInstrumentFallback.volume,
  };
  fireSfx = kFireSfx;
  fireSfx.instrument = &fireInstrument;
  hitSfx = kHitSfx;
  hitSfx.instrument = &hitInstrument;
  pickupSfx = kPickupSfx;
  pickupSfx.instrument = &pickupInstrument;
  doorSfx = kDoorSfx;
  doorSfx.instrument = &doorInstrument;
  zombieFireSfx = kZombieFireSfx;
  zombieFireSfx.instrument = &zombieFireInstrument;
  zombieDieSfx = kZombieDieSfx;
  zombieDieSfx.instrument = &zombieDieInstrument;
  ghostAttackSfx = kGhostAttackSfx;
  ghostAttackSfx.instrument = &ghostAttackInstrument;
  ghostDieSfx = kGhostDieSfx;
  ghostDieSfx.instrument = &ghostDieInstrument;

  songLanes[1] = {
    .voiceIndex = kBassVoice,
    .player = &synthEngine,
    .program = makeProgramRef(bassInstrument),
    .clips = kBassClips,
    .clipCount = count16(kBassClips),
  };
  songLanes[2] = {
    .voiceIndex = kHatVoice,
    .player = &synthEngine,
    .program = makeProgramRef(hatInstrument),
    .clips = kHatClips,
    .clipCount = count16(kHatClips),
  };
  songLanes[3] = {
    .voiceIndex = kSnareVoice,
    .player = &samplePlayer,
    .program = makeProgramRef(snareSampleInstrument),
    .clips = kSnareClips,
    .clipCount = count16(kSnareClips),
  };
  song = {
    .lanes = songLanes,
    .laneCount = count8(songLanes),
  };
  configureSampleOverrides();
  songPlayer.bind(song);
}

void WolfAudio::configureSampleOverrides() {
  if (const AudioSample* sample = WolfAudioSamples::find("fire")) {
    fireSampleInstrument = kSampleOneShotInstrument;
    fireSampleInstrument.sample = sample;
  }
  if (const AudioSample* sample = WolfAudioSamples::find("hit")) {
    hitSampleInstrument = kSampleOneShotInstrument;
    hitSampleInstrument.sample = sample;
  }
  if (const AudioSample* sample = WolfAudioSamples::find("pickup")) {
    pickupSampleInstrument = kSampleOneShotInstrument;
    pickupSampleInstrument.sample = sample;
  }
  if (const AudioSample* sample = WolfAudioSamples::find("door")) {
    doorSampleInstrument = kSampleOneShotInstrument;
    doorSampleInstrument.sample = sample;
  }
  if (const AudioSample* sample = WolfAudioSamples::find("zombie_fire")) {
    zombieFireSampleInstrument = kSampleOneShotInstrument;
    zombieFireSampleInstrument.sample = sample;
  }
  if (const AudioSample* sample = WolfAudioSamples::find("zombie_die")) {
    zombieDieSampleInstrument = kSampleOneShotInstrument;
    zombieDieSampleInstrument.sample = sample;
  }
  if (const AudioSample* sample = WolfAudioSamples::find("ghost_attack")) {
    ghostAttackSampleInstrument = kSampleOneShotInstrument;
    ghostAttackSampleInstrument.sample = sample;
  }
  if (const AudioSample* sample = WolfAudioSamples::find("ghost_die")) {
    ghostDieSampleInstrument = kSampleOneShotInstrument;
    ghostDieSampleInstrument.sample = sample;
  }
  if (const AudioSample* sample = WolfAudioSamples::find("lead")) {
    leadSampleInstrument = kSamplePitchedInstrument;
    leadSampleInstrument.sample = sample;
    leadSampleInstrument.volume = kMusicInstrument.volume;
  }
  if (const AudioSample* sample = WolfAudioSamples::find("bass")) {
    bassSampleInstrument = kSamplePitchedInstrument;
    bassSampleInstrument.sample = sample;
    bassSampleInstrument.volume = kBassInstrument.volume;
    songLanes[1].player = &samplePlayer;
    songLanes[1].program = makeProgramRef(bassSampleInstrument);
  }
  if (const AudioSample* sample = WolfAudioSamples::find("hat")) {
    hatSampleInstrument = kSampleOneShotInstrument;
    hatSampleInstrument.sample = sample;
    hatSampleInstrument.volume = kHatInstrument.volume;
    songLanes[2].player = &samplePlayer;
    songLanes[2].program = makeProgramRef(hatSampleInstrument);
  }
  if (const AudioSample* sample = WolfAudioSamples::find("snare")) {
    snareSampleInstrument = kSampleOneShotInstrument;
    snareSampleInstrument.sample = sample;
    snareSampleInstrument.volume = kSnareInstrumentFallback.volume;
  }
}

#if WOLF_AUDIO_DIAG_TONE
int16_t WolfAudio::renderSample() {
  if (!synthEngine.voiceActive(kLeadVoice)) {
    synthEngine.noteOn(kLeadVoice, kMusicInstrument, 261.63f, 200u);
  }
  return mixer.renderSample();
#else
int16_t WolfAudio::renderSample() {
#if WOLF_AUDIO_ENABLE_MUSIC
  songPlayer.tick();
#endif
  return mixer.renderSample();
}
#endif

void WolfAudio::playSynthSfx(const Sfx& sfx, float baseHz, uint8_t velocity) {
#if WOLF_AUDIO_DIAG_TONE
  (void)sfx;
  (void)baseHz;
  (void)velocity;
  return;
#else
  synthEngine.triggerSfx(nextSynthSfxVoice, sfx, baseHz, velocity);
  nextSynthSfxVoice = (nextSynthSfxVoice + 1u) % kSfxVoiceCount;
#endif
}

void WolfAudio::playFire() {
  if (fireSampleInstrument.sample != nullptr) {
    samplePlayer.playOneShot(fireSampleInstrument, 255u);
  } else {
    playSynthSfx(fireSfx, 740.0f, 255u);
  }
}

void WolfAudio::playHit() {
  if (hitSampleInstrument.sample != nullptr) {
    samplePlayer.playOneShot(hitSampleInstrument, 255u);
  } else {
    playSynthSfx(hitSfx, 220.0f, 255u);
  }
}

void WolfAudio::playPickup() {
  if (pickupSampleInstrument.sample != nullptr) {
    samplePlayer.playOneShot(pickupSampleInstrument, 255u);
  } else {
    playSynthSfx(pickupSfx, 660.0f, 255u);
  }
}

void WolfAudio::playDoorOpen() {
  if (doorSampleInstrument.sample != nullptr) {
    samplePlayer.playOneShot(doorSampleInstrument, 255u);
  } else {
    playSynthSfx(doorSfx, 196.0f, 255u);
  }
}

void WolfAudio::playEnemyAudio(const char* audioId) {
  if (audioId == nullptr) {
    return;
  }
  if (strcmp(audioId, "zombie_fire") == 0) {
    playZombieFire();
    return;
  }
  if (strcmp(audioId, "zombie_die") == 0) {
    playZombieDie();
    return;
  }
  if (strcmp(audioId, "ghost_attack") == 0) {
    playGhostAttack();
    return;
  }
  if (strcmp(audioId, "ghost_die") == 0) {
    playGhostDie();
    return;
  }
}

void WolfAudio::playZombieFire() {
  if (zombieFireSampleInstrument.sample != nullptr) {
    samplePlayer.playOneShot(zombieFireSampleInstrument, 255u);
  } else {
    playSynthSfx(zombieFireSfx, 240.0f, 255u);
  }
}

void WolfAudio::playZombieDie() {
  if (zombieDieSampleInstrument.sample != nullptr) {
    samplePlayer.playOneShot(zombieDieSampleInstrument, 255u);
  } else {
    playSynthSfx(zombieDieSfx, 160.0f, 255u);
  }
}

void WolfAudio::playGhostAttack() {
  if (ghostAttackSampleInstrument.sample != nullptr) {
    samplePlayer.playOneShot(ghostAttackSampleInstrument, 255u);
  } else {
    playSynthSfx(ghostAttackSfx, 196.0f, 255u);
  }
}

void WolfAudio::playGhostDie() {
  if (ghostDieSampleInstrument.sample != nullptr) {
    samplePlayer.playOneShot(ghostDieSampleInstrument, 255u);
  } else {
    playSynthSfx(ghostDieSfx, 180.0f, 255u);
  }
}
