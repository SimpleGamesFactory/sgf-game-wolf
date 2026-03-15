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

#ifndef ENABLE_AUDIO
#define ENABLE_AUDIO 1
#endif

#ifndef WOLF_AUDIO_ENABLE
#define WOLF_AUDIO_ENABLE ENABLE_AUDIO
#endif

#ifndef WOLF_AUDIO_DAC_PIN
#define WOLF_AUDIO_DAC_PIN 25
#endif

#ifndef WOLF_AUDIO_SILENT_SOURCE
#define WOLF_AUDIO_SILENT_SOURCE 0
#endif

#ifndef WOLF_AUDIO_DAC_TIMER_TEST
#define WOLF_AUDIO_DAC_TIMER_TEST 0
#endif

#ifndef WOLF_AUDIO_DIAG_QUIET
#define WOLF_AUDIO_DIAG_QUIET 0
#endif

#include "SGF.h"
#include "SGFHardwarePresets.h"
#include "Game.h"

#if defined(ARDUINO_ARCH_ESP32) && WOLF_AUDIO_DIAG_QUIET
#include <esp_bt.h>
#include <esp_log.h>
#include <esp_wifi.h>
#endif

auto hardware = SGFHardwareProfile::makeRuntime();
Wolf3DGame game(hardware.renderTarget(), hardware.screen(), hardware.profile);
SerialMonitor serialMonitor(1000u, 115200u);
#if defined(ARDUINO_ARCH_ESP32) && WOLF_AUDIO_ENABLE
#if WOLF_AUDIO_DAC_TIMER_TEST
class DacTimerSilenceTestOutput {
public:
  explicit DacTimerSilenceTestOutput(uint8_t pinRef) : pin(pinRef) {}

  bool begin() {
    if (began) {
      return true;
    }
    began = true;
    dacWrite(pin, 128u);
#if WOLF_AUDIO_DAC_TIMER_TEST == 1
    timer = timerBegin(11025u);
    if (timer == nullptr) {
      began = false;
      return false;
    }
    timerAttachInterruptArg(timer, &DacTimerSilenceTestOutput::onTimerThunk, this);
    timerAlarm(timer, 1u, true, 0u);
#endif
    return true;
  }

  void end() {
    if (timer != nullptr) {
      timerEnd(timer);
      timer = nullptr;
    }
    began = false;
    dacWrite(pin, 128u);
  }

  bool attachSerialMonitor(SerialMonitor&) { return false; }

private:
  static void onTimerThunk(void* arg) {
    if (arg == nullptr) {
      return;
    }
    auto* output = static_cast<DacTimerSilenceTestOutput*>(arg);
    output->onTimer();
  }

  void onTimer() { dacWrite(pin, 128u); }

  uint8_t pin = 25u;
  bool began = false;
  hw_timer_t* timer = nullptr;
};

DacTimerSilenceTestOutput audioOutput(WOLF_AUDIO_DAC_PIN);
#elif WOLF_AUDIO_SILENT_SOURCE
class SilentAudioSource : public SGFAudio::IAudioSource {
public:
  uint32_t sampleRate() const override { return 11025u; }
  int16_t renderSample() override { return 0; }
};

SilentAudioSource silentAudioSource;
SGFAudio::ESP32DacAudioOutput audioOutput(silentAudioSource, WOLF_AUDIO_DAC_PIN);
#else
SGFAudio::ESP32DacAudioOutput audioOutput(game.audioBank(), WOLF_AUDIO_DAC_PIN);
#endif
#endif

#if defined(ARDUINO_ARCH_ESP32) && WOLF_AUDIO_DIAG_QUIET
namespace {

void enterQuietDiagMode() {
  esp_log_level_set("*", ESP_LOG_NONE);
  Serial.setDebugOutput(false);
  Serial.end();
  esp_wifi_stop();
  esp_wifi_deinit();
  esp_bt_controller_disable();
  esp_bt_controller_deinit();
}

}  // namespace
#endif

void setup() {
#if defined(ARDUINO_ARCH_ESP32) && WOLF_AUDIO_DIAG_QUIET
  enterQuietDiagMode();
#endif
  hardware.display.begin(hardware.profile.display.spiHz);
  hardware.display.setRotation(hardware.profile.display.rotation);
  hardware.display.setBacklight(hardware.profile.display.backlightLevel);
#if defined(ARDUINO_ARCH_ESP32) && WOLF_AUDIO_ENABLE
  audioOutput.begin();
#if !WOLF_AUDIO_DIAG_QUIET
  audioOutput.attachSerialMonitor(serialMonitor);
#endif
#endif
#if !WOLF_AUDIO_DIAG_QUIET
  game.attachSerialMonitor(serialMonitor);
  serialMonitor.attachProfiler(game.stageProfiler());
#endif
  game.setup();
}

void loop() {
  game.loop();
}
