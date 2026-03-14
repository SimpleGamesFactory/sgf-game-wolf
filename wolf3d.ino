// sgf.name: wolf3d
// sgf.boards: esp32
// sgf.default_board: esp32
// sgf.port.esp32: /dev/ttyUSB0

// Define SGF_HW_PRESET only for manual IDE builds without `sgf`.
// Example:
// #define SGF_HW_PRESET SGF_HW_PRESET_ESP32_ST7789_240X240

#ifndef SGF_ESP32_ST7789_240X240_SPI_HZ
#define SGF_ESP32_ST7789_240X240_SPI_HZ 40000000u
#endif

#ifndef SGF_ESP32_ST7789_240X240_USE_DMA_BUS
#define SGF_ESP32_ST7789_240X240_USE_DMA_BUS 1
#endif

#ifndef WOLF_AUDIO_ENABLE
#define WOLF_AUDIO_ENABLE 1
#endif

#ifndef WOLF_AUDIO_DAC_PIN
#define WOLF_AUDIO_DAC_PIN 25
#endif

#include "SGF.h"
#include "SGFHardwarePresets.h"
#include "Game.h"

auto hardware = SGFHardwareProfile::makeRuntime();
Wolf3DGame game(hardware.renderTarget(), hardware.screen(), hardware.profile);
SerialMonitor serialMonitor(1000u, 115200u);
#if defined(ARDUINO_ARCH_ESP32) && WOLF_AUDIO_ENABLE
SGFAudio::ESP32DacSynthOutput audioOutput(game.audioBank().synth(), WOLF_AUDIO_DAC_PIN);
#endif

void setup() {
  hardware.display.begin(hardware.profile.display.spiHz);
  hardware.display.setRotation(hardware.profile.display.rotation);
  hardware.display.setBacklight(hardware.profile.display.backlightLevel);
#if defined(ARDUINO_ARCH_ESP32) && WOLF_AUDIO_ENABLE
  audioOutput.begin();
#endif
  game.attachSerialMonitor(serialMonitor);
  serialMonitor.attachProfiler(game.stageProfiler());
  game.setup();
}

void loop() {
  game.loop();
}
