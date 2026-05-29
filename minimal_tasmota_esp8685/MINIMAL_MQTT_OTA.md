# ESP8685 Minimal MQTT OTA

Target environment:

- `tasmota32c3-minimal-mqttota`

Build:

```powershell
& "$env:USERPROFILE\.platformio\penv\Scripts\platformio.exe" run -e tasmota32c3-minimal-mqttota
```

Main output:

- `.pio/build/tasmota32c3-minimal-mqttota/firmware.bin`
- `build_output/firmware/tasmota32c3-minimal-mqttota.bin`
- `build_output/firmware/tasmota32c3-minimal-mqttota.factory.bin`

Binary roles:

- `firmware.bin`: raw OTA application image
- `tasmota32c3-minimal-mqttota.bin`: copied/renamed OTA application image
- `tasmota32c3-minimal-mqttota.factory.bin`: merged full-flash image for UART flashing, not for app OTA

Current build result:

- Flash used: `776658` bytes

Partition layout for this profile:

- Follows the SmartPlug dual OTA layout
- Build target partition is `app0`
- Dual OTA partitions are `app0 (ota_0)` and `app1 (ota_1)`
- Tasmota `FIRMWARE_SAFEBOOT` build mode is not used in this profile

Included:

- Wi-Fi / AP manager
- MQTT
- Button / Relay / LED
- HTTP OTA triggered by MQTT

Removed or minimized:

- Berry features for this profile
- Rules / Script
- TLS certificate table for this profile
- WS2812 / light extras

Default GPIO mapping applied automatically on first boot if unset:

- Relay: `GPIO4`
- LED: `GPIO6`
- Button: `GPIO20`

Smart plug MQTT topics:

- `smart_plug/<uid>/command`
- `smart_plug/<uid>/status`
- `smart_plug/<tasmota_topic>/command`

Examples:

```text
smart_plug/3C0F021851A4/command -> on
smart_plug/3C0F021851A4/command -> off
smart_plug/3C0F021851A4/command -> status
smart_plug/3C0F021851A4/command -> ota http://your-server/firmware.bin
```

JSON command example:

```json
{"cmd":"ota","url":"http://your-server/firmware.bin"}
```

Stock Tasmota MQTT OTA still works too:

```text
cmnd/tasmota_XXXXXX/OtaUrl -> http://your-server/firmware.bin
cmnd/tasmota_XXXXXX/Upgrade -> 1
```

Web server policy:

- Compiled in to preserve Wi-Fi manager / AP onboarding
- Default runtime mode is `WebServer 0`
- You can re-enable it over MQTT and the device will reboot:

```text
smart_plug/<uid>/command -> webserver 1
smart_plug/<uid>/command -> webserver 2
smart_plug/<uid>/command -> webserver 0
```
