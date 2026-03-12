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

#include "SGFHardwarePresets.h"
#include "Game.h"

auto hardware = SGFHardwareProfile::makeRuntime();
Wolf3DGame game(hardware.renderTarget(), hardware.screen(), hardware.profile);

void setup() {
  hardware.display.begin(hardware.profile.display.spiHz);
  hardware.display.setRotation(hardware.profile.display.rotation);
  hardware.display.setBacklight(hardware.profile.display.backlightLevel);
  game.setup();
}

void loop() {
  game.loop();
}
