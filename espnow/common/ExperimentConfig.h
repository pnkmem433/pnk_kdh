#pragma once

#include <Arduino.h>

// Edit only this file when switching test conditions.
namespace ExperimentConfig {

// ---------------------------------------------------------------------------
// Current default profile:
// Pickdown hold test: 1,2,3,4 simultaneous hold
// Change only this file when moving to the next run.
//
// Typical switches for your next runs:
// - Environment: NORMAL -> INTERFERENCE_250M
// - Pattern: ALL_TAG_FIXED_1234 -> next scenario label
// ---------------------------------------------------------------------------

// Master-side test metadata written into CSV.
constexpr char kPhase[] = "PICKDOWN_SCREEN";     // PICKDOWN_SCREEN / PICKUP_SEQ / PICKUP_SIMUL / PICKUP_INTERFERENCE
constexpr char kEnvironment[] = "NORMAL";        // NORMAL / INTERFERENCE_250M
constexpr char kPattern[] = "ALL_TAG_FIXED_1234"; // descriptive only

// Progress target shown on the master serial monitor.
// For pick-down tests use "PICK-DOWN", for remove-side tests use "PICK-UP".
constexpr char kGoalEvent[] = "PICK-DOWN";
constexpr uint32_t kGoalTargetCount = 0;         // use for count-based tests
constexpr uint32_t kGoalDurationMs = 1800000;    // 30 minutes

// Master-side sequential poll interval between slaves.
constexpr uint32_t kMasterPollIntervalMs = 5;    // 5 / 25 / 50

// Slave-side NFC timing.
constexpr uint16_t kNfcPollIntervalMs = 25;      // fixed at 25 for current test plan
constexpr int kRemoveTimeMs = 700;               // 100 / 300 / 500 / 700
constexpr uint8_t kNfcReinitMisses = 20;         // usually keep fixed unless hardware recovery test is needed
constexpr uint32_t kMuxSwitchIntervalMs = 250;   // NFC-1/NFC-2 switching interval on each slave

// ESP-NOW channel sync behavior.
constexpr uint32_t kHeartbeatIntervalMs = 100;   // master heartbeat period during channel discovery
constexpr uint32_t kHeartbeatTimeoutMs = 180000; // do not re-scan unless heartbeats are gone for 3 minutes
constexpr uint32_t kChannelScanIntervalMs = 1200; // stay longer on each channel so the slave can catch heartbeats reliably

}  // namespace ExperimentConfig
