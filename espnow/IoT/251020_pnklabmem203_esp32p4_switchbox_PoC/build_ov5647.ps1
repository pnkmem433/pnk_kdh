# ESP-IDF Build Script for OV5647 Camera
Set-Location "d:\04.pretests-iot\251020_pnklabmem203_esp32p4_switchbox_PoC"

# Load ESP-IDF environment
& "c:\Users\User\.vscode\extensions\espressif.esp-idf-extension-1.10.2\export.ps1"

# Build project
idf.py build
