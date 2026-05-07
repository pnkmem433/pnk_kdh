#pragma once

#include <Arduino.h>

// Edit only this file when switching test conditions.
namespace ExperimentConfig {

// ---------------------------------------------------------------------------
// Current default profile:
// Pickup Test 1 (manual pickup events during a stable pickdown hold) - candidate A
// Change only this file when moving to the next run.
//
// Typical switches for your next runs:
// - kRemoveTimeMs: 500 -> 700
// - Environment: NORMAL -> INTERFERENCE_250M
// ---------------------------------------------------------------------------

// Master-side test metadata written into CSV.
constexpr char kPhase[] = "PICKUP_SEQ";          // PICKDOWN_SCREEN / PICKUP_SEQ / PICKUP_SIMUL / PICKUP_INTERFERENCE
constexpr char kEnvironment[] = "NORMAL";        // NORMAL / INTERFERENCE_250M
constexpr char kPattern[] = "MANUAL_PICKUP_DURING_HOLD"; // descriptive only; analyze by Slave_ID for pickup test 1

// Progress target shown on the master serial monitor.
// For pick-down tests use "PICK-DOWN", for remove-side tests use "PICK-UP".
constexpr char kGoalEvent[] = "PICK-UP";
constexpr uint32_t kGoalTargetCount = 120;       // pickup test 1 total target across all slaves
constexpr uint32_t kGoalDurationMs = 0;          // use for time-based pickdown screening

// Master-side sequential poll interval between slaves.
constexpr uint32_t kMasterPollIntervalMs = 25;    // 5 / 25 / 50

// Slave-side NFC timing.
constexpr uint16_t kNfcPollIntervalMs = 25;      // fixed at 25 for current test plan
constexpr int kRemoveTimeMs = 500;               // 100 / 300 / 500 / 700
constexpr uint8_t kNfcReinitMisses = 20;         // usually keep fixed unless hardware recovery test is needed
constexpr uint32_t kMuxSwitchIntervalMs = 250;   // NFC-1/NFC-2 switching interval on each slave

// ESP-NOW channel sync behavior.
constexpr uint32_t kHeartbeatIntervalMs = 100;   // master heartbeat period during channel discovery
constexpr uint32_t kHeartbeatTimeoutMs = 180000; // do not re-scan unless heartbeats are gone for 3 minutes
constexpr uint32_t kChannelScanIntervalMs = 1200; // stay longer on each channel so the slave can catch heartbeats reliably

}  // namespace ExperimentConfig
