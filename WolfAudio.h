#pragma once

#ifndef WOLF_AUDIO_DIAG_TONE
#define WOLF_AUDIO_DIAG_TONE 0
#endif

#ifndef WOLF_AUDIO_ENABLE_MUSIC
#define WOLF_AUDIO_ENABLE_MUSIC 1
#endif

#include "SGF/AudioMixer.h"
#include "SGF/AudioTypes.h"
#include "SGF/SamplePlayer.h"
#include "SGF/Song.h"
#include "SGF/Synth.h"

class WolfAudio : public SGFAudio::IAudioSource {
public:
  WolfAudio();

  SGFAudio::SynthEngine& synth() { return synthEngine; }
  const SGFAudio::SynthEngine& synth() const { return synthEngine; }

  uint32_t sampleRate() const override { return mixer.sampleRate(); }
  int16_t renderSample() override;
  void playFire();
  void playHit();
  void playPickup();
  void playDoorOpen();
  void playZombieFire();
  void playZombieDie();
  void playGhostAttack();
  void playGhostDie();

private:
  void playSynthSfx(const SGFAudio::Sfx& sfx, float baseHz, uint8_t velocity = 255u);
  void configureSampleOverrides();

  SGFAudio::AudioMixer mixer;
  SGFAudio::SynthEngine synthEngine;
  SGFAudio::SamplePlayer samplePlayer;
  SGFAudio::Instrument fireInstrument{};
  SGFAudio::Instrument hitInstrument{};
  SGFAudio::Instrument pickupInstrument{};
  SGFAudio::Instrument doorInstrument{};
  SGFAudio::Instrument zombieFireInstrument{};
  SGFAudio::Instrument zombieDieInstrument{};
  SGFAudio::Instrument ghostAttackInstrument{};
  SGFAudio::Instrument ghostDieInstrument{};
  SGFAudio::Instrument leadInstrument{};
  SGFAudio::Instrument bassInstrument{};
  SGFAudio::Instrument hatInstrument{};
  SGFAudio::Instrument snareInstrument{};
  SGFAudio::SampleInstrument fireSampleInstrument{};
  SGFAudio::SampleInstrument hitSampleInstrument{};
  SGFAudio::SampleInstrument pickupSampleInstrument{};
  SGFAudio::SampleInstrument doorSampleInstrument{};
  SGFAudio::SampleInstrument zombieFireSampleInstrument{};
  SGFAudio::SampleInstrument zombieDieSampleInstrument{};
  SGFAudio::SampleInstrument ghostAttackSampleInstrument{};
  SGFAudio::SampleInstrument ghostDieSampleInstrument{};
  SGFAudio::SampleInstrument leadSampleInstrument{};
  SGFAudio::SampleInstrument bassSampleInstrument{};
  SGFAudio::SampleInstrument hatSampleInstrument{};
  SGFAudio::SampleInstrument snareSampleInstrument{};
  SGFAudio::Sfx fireSfx{};
  SGFAudio::Sfx hitSfx{};
  SGFAudio::Sfx pickupSfx{};
  SGFAudio::Sfx doorSfx{};
  SGFAudio::Sfx zombieFireSfx{};
  SGFAudio::Sfx zombieDieSfx{};
  SGFAudio::Sfx ghostAttackSfx{};
  SGFAudio::Sfx ghostDieSfx{};
  SGFAudio::SongLane songLanes[4]{};
  SGFAudio::Song song{};
  SGFAudio::SongPlayer songPlayer;
  uint8_t nextSynthSfxVoice = 0u;
};
