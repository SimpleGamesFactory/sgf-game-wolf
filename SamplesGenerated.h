#pragma once

#include <stdint.h>

#include "SGF/Synth.h"

namespace SamplesGenerated {

struct Entry {
  const char* code;
  const SGFAudio::AudioSample* sample;
};

extern const Entry ENTRIES[];
extern const unsigned int ENTRY_COUNT;

}
