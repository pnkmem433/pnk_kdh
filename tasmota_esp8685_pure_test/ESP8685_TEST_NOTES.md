# ESP8685 Pure Tasmota Test

This folder is a dedicated Tasmota-based working copy for the custom ESP8685
(ESP32-C3) smart-plug board.

Current confirmed board facts:
- Serial flashing via external USB-TTL works
- `esptool` detects the chip as `ESP8685 (QFN28)`
- The chip reports `Embedded Flash 4MB (XMC)`
- The official `tasmota32c3.factory.bin` boots far enough to bring up Wi-Fi

Intent of this folder:
- keep a separate Tasmota source tree for the ESP8685 / ESP32-C3 board
- validate stock-like Tasmota boot/AP/WebUI first
- then edit Tasmota code here and rebuild for the same board
- later test OTA with locally built images from this same worktree

What changed from the copied Tasmota source:
- kept `tasmota/user_config_override.h` intentionally minimal
- added local work envs:
  - `tasmota32c3-work-safeboot`
  - `tasmota32c3-work`
  - `tasmota32c3-work-dev`
- set `platformio_override.ini` default env to `tasmota32c3-work`
- kept the older `esp32c3ser_2M` experiment only as a reference

Why these changes are needed:
- the board is flashed over UART / USB-TTL, but the official `tasmota32c3`
  factory image has already been proven to boot, so the local work env now
  tracks the standard `esp32c3` target for build artifact compatibility
- the original 2MB experiment is no longer the main path because the real chip
  has already reported 4MB flash and the official 4MB factory image boots
- this folder should now behave like a normal ESP32-C3 Tasmota worktree that is
  safe to modify and rebuild

How this folder should be used:
1. Initial serial flash / recovery
   - official factory image can be written with `esptool`
   - reference file:
     `downloads/tasmota32c3.factory.bin`
2. Local source modification
   - edit Tasmota code in this folder
   - build `tasmota32c3-work` for normal testing
   - build `tasmota32c3-work-dev` if a separate dev image name is helpful
3. Later OTA validation
   - after WebUI/Wi-Fi behavior is confirmed
   - use locally built OTA images from this folder or Tasmota OTA URLs as needed

Recommended build flow:
1. Build `tasmota32c3-work-safeboot`
2. Build `tasmota32c3-work`
3. Use those artifacts for repeated testing after code changes
