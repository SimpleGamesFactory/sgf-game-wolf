#pragma once

#include "SGF/Synth.h"

class WolfAudio {
public:
  WolfAudio();

  SGFAudio::SynthEngine& synth() { return synthEngine; }
  const SGFAudio::SynthEngine& synth() const { return synthEngine; }

  void playFire();
  void playHit();
  void playPickup();
  void playDoorOpen();

private:
  SGFAudio::SynthEngine synthEngine;
};
