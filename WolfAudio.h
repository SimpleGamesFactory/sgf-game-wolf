#pragma once

#ifndef WOLF_AUDIO_DIAG_TONE
#define WOLF_AUDIO_DIAG_TONE 0
#endif

#include "SGF/Pattern.h"
#include "SGF/Synth.h"

class WolfAudio : public SGFAudio::IAudioSource {
public:
  WolfAudio();

  SGFAudio::SynthEngine& synth() { return synthEngine; }
  const SGFAudio::SynthEngine& synth() const { return synthEngine; }

  uint32_t sampleRate() const override { return synthEngine.sampleRate(); }
  int16_t renderSample() override;
  void playFire();
  void playHit();
  void playPickup();
  void playDoorOpen();

private:
  void playSfx(const SGFAudio::Sfx& sfx, float baseHz, uint8_t velocity = 255u);

  SGFAudio::SynthEngine synthEngine;
  SGFAudio::PatternTrack leadTrack;
  SGFAudio::PatternTrack bassTrack;
  SGFAudio::PatternTrack hatTrack;
  SGFAudio::PatternTrack snareTrack;
  uint8_t nextSfxVoice = 0u;
};
