#include <Arduino.h>
#include <TaskRunner.h>
#include <nfc.h>

#include "../../common/ExperimentConfig.h"

TaskRunner nfcTask({
  .name = "nfc_loop",
  .stackSize = 8192,
  .priority = 1,
  .core = -1,
});

TaskRunner nfcTask2({
  .name = "nfc_loop2",
  .stackSize = 8192,
  .priority = 1,
  .core = -1,
});

TaskRunner muxTask({
  .name = "nfc_mux",
  .stackSize = 4096,
  .priority = 1,
  .core = -1,
});

NfcReader nfcReader({
  .pin = {
    .SCK = D8,
    .MISO = D9,
    .MOSI = D10,
    .SS = D3,
    .RST = D2,
    .IRQ = -1,
  },
  .task = &nfcTask,
  .settings = {
    .miss = ExperimentConfig::kRemoveTimeMs,
    .irqMode = false,
    .pollIntervalMs = ExperimentConfig::kNfcPollIntervalMs,
    .reinitMisses = ExperimentConfig::kNfcReinitMisses,
  },
});

NfcReader nfcReader2({
  .pin = {
    .SCK = D8,
    .MISO = D9,
    .MOSI = D10,
    .SS = D4,
    .RST = -1,
    .IRQ = -1,
  },
  .task = &nfcTask2,
  .settings = {
    .miss = ExperimentConfig::kRemoveTimeMs,
    .irqMode = false,
    .pollIntervalMs = ExperimentConfig::kNfcPollIntervalMs,
    .reinitMisses = ExperimentConfig::kNfcReinitMisses,
  },
});

void setup();
void loop();
