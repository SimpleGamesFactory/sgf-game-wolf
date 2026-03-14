#include "WolfAudio.h"

#include "AudioSamples.h"

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
  .volume = 75u,
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
  .volume = 59u,
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
  .volume = 46u,
};

constexpr Instrument kSnareInstrumentFallback{
  .waveform = Waveform::Noise,
  .ampEnv = {0u, 18u, 0u, 28u},
  .pitchLfo = {},
  .pitchEnv = nullptr,
  .pitchEnvCount = 0u,
  .filterFlags = static_cast<uint8_t>(FilterLowPass | FilterHighPass),
  .lowPassCutoffHz = 2200.0f,
  .highPassCutoffHz = 700.0f,
  .volume = 62u,
};

constexpr SampleInstrument kSampleOneShotInstrument{
  .sample = nullptr,
  .volume = 255u,
  .oneShot = true,
};

constexpr SampleInstrument kSamplePitchedInstrument{
  .sample = nullptr,
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
  .stepCount = static_cast<uint16_t>(sizeof(kLeadPatternASteps) / sizeof(kLeadPatternASteps[0])),
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
  .stepCount = static_cast<uint16_t>(sizeof(kLeadPatternBSteps) / sizeof(kLeadPatternBSteps[0])),
  .unitMs = kBaseStepMs,
  .loop = false,
};

constexpr PatternStep kBassPatternCSteps[] = {
  {65.41f, 4u, 132u},   // C2
  {65.41f, 1u, 132u},
  {0.0f,   27u, 0u},    // pause
};

constexpr Pattern kBassPatternC{
  .steps = kBassPatternCSteps,
  .stepCount = static_cast<uint16_t>(sizeof(kBassPatternCSteps) / sizeof(kBassPatternCSteps[0])),
  .unitMs = kBaseStepMs,
  .loop = false,
};

constexpr PatternStep kBassPatternFSteps[] = {
  {87.31f, 4u, 132u},   // F2
  {87.31f, 1u, 132u},
  {0.0f,   27u, 0u},
};

constexpr Pattern kBassPatternF{
  .steps = kBassPatternFSteps,
  .stepCount = static_cast<uint16_t>(sizeof(kBassPatternFSteps) / sizeof(kBassPatternFSteps[0])),
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
  .stepCount = static_cast<uint16_t>(sizeof(kHatPatternSteps) / sizeof(kHatPatternSteps[0])),
  .unitMs = kBaseStepMs,
  .loop = false,
};

constexpr PatternStep kSnarePatternSteps[] = {
  {0.0f, 4u, 0u},    // P
  {1.0f, 4u, 220u},  // S
};

constexpr Pattern kSnarePattern{
  .steps = kSnarePatternSteps,
  .stepCount = static_cast<uint16_t>(sizeof(kSnarePatternSteps) / sizeof(kSnarePatternSteps[0])),
  .unitMs = kBaseStepMs,
  .loop = false,
};

constexpr SongClip kLeadClips[] = {
  {.pattern = &kLeadPatternA, .repeats = 4u},
  {.pattern = &kLeadPatternB, .repeats = 4u},
};

constexpr SongClip kBassClips[] = {
  {.pattern = &kBassPatternC, .repeats = 1u},
  {.pattern = &kBassPatternF, .repeats = 1u},
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
  leadInstrument = kMusicInstrument;
  bassInstrument = kBassInstrument;
  hatInstrument = kHatInstrument;
  snareInstrument = kSnareInstrumentFallback;
  fireSampleInstrument = kSampleOneShotInstrument;
  hitSampleInstrument = kSampleOneShotInstrument;
  pickupSampleInstrument = kSampleOneShotInstrument;
  doorSampleInstrument = kSampleOneShotInstrument;
  leadSampleInstrument = kSamplePitchedInstrument;
  bassSampleInstrument = kSamplePitchedInstrument;
  hatSampleInstrument = kSampleOneShotInstrument;
  snareSampleInstrument = {
    .sample = &WolfAudioSamples::kSnare,
    .volume = kSnareInstrumentFallback.volume,
    .oneShot = true,
  };
  fireSfx = kFireSfx;
  fireSfx.instrument = &fireInstrument;
  hitSfx = kHitSfx;
  hitSfx.instrument = &hitInstrument;
  pickupSfx = kPickupSfx;
  pickupSfx.instrument = &pickupInstrument;
  doorSfx = kDoorSfx;
  doorSfx.instrument = &doorInstrument;

  songLanes[0] = {
    .voiceIndex = kLeadVoice,
    .player = &synthEngine,
    .program = makeProgramRef(leadInstrument),
    .clips = kLeadClips,
    .clipCount = static_cast<uint16_t>(sizeof(kLeadClips) / sizeof(kLeadClips[0])),
  };
  songLanes[1] = {
    .voiceIndex = kBassVoice,
    .player = &synthEngine,
    .program = makeProgramRef(bassInstrument),
    .clips = kBassClips,
    .clipCount = static_cast<uint16_t>(sizeof(kBassClips) / sizeof(kBassClips[0])),
  };
  songLanes[2] = {
    .voiceIndex = kHatVoice,
    .player = &synthEngine,
    .program = makeProgramRef(hatInstrument),
    .clips = kHatClips,
    .clipCount = static_cast<uint16_t>(sizeof(kHatClips) / sizeof(kHatClips[0])),
  };
  songLanes[3] = {
    .voiceIndex = kSnareVoice,
    .player = &samplePlayer,
    .program = makeProgramRef(snareSampleInstrument),
    .clips = kSnareClips,
    .clipCount = static_cast<uint16_t>(sizeof(kSnareClips) / sizeof(kSnareClips[0])),
  };
  song = {
    .lanes = songLanes,
    .laneCount = static_cast<uint8_t>(sizeof(songLanes) / sizeof(songLanes[0])),
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
  if (const AudioSample* sample = WolfAudioSamples::find("lead")) {
    leadSampleInstrument = kSamplePitchedInstrument;
    leadSampleInstrument.sample = sample;
    leadSampleInstrument.volume = kMusicInstrument.volume;
    songLanes[0].player = &samplePlayer;
    songLanes[0].program = makeProgramRef(leadSampleInstrument);
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
  songPlayer.tick();
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
  nextSynthSfxVoice = static_cast<uint8_t>((nextSynthSfxVoice + 1u) % kSfxVoiceCount);
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
