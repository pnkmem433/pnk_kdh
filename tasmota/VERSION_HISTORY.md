# Tasmota Smart Plug Version History

This folder tracks the ESP8285 smart-plug firmware iterations discussed in this workspace.

## Version order

1. `0.5.0`
   - First OTA-aware test build used during this session.
   - Introduced embedded version fingerprint format:
     - `SMARTPLUG_FW_VERSION:x.y.z`
   - Historical source snapshot was not committed before this folder was added to Git.

2. `0.5.1`
   - Follow-up test build after the initial `0.5.0` OTA attempt.
   - Used the same embedded version system and OTA flow while adjusting visible LED behavior.
   - Historical source snapshot was not committed before this folder was added to Git.

3. `0.5.2`
   - Current source in `src/main.cpp`
   - `FW_VER_MAJOR=0`, `FW_VER_MINOR=5`, `FW_VER_PATCH=2`
   - Uses the current setup flow:
     - boot blink test
     - Wi-Fi begin
     - OTA update check
     - UUID/MQTT setup

## Current version source of truth

The active firmware version is defined in:

- `src/main.cpp`

The OTA server base URL is defined in:

- `platformio.ini`

## Notes

- This project currently targets ESP8285 through PlatformIO.
- Earlier `0.5.0` and `0.5.1` source states were discussed and tested in the workspace, but were not preserved as committed Git snapshots before this folder was staged into the parent repository.
