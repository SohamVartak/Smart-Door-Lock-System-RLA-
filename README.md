# SmartDoor Face Security System

A student prototype combining phone-based IP Webcam streaming, Python/OpenCV face recognition, an Android app using the phone's biometric prompt, BLE communication, and an ESP32 with an SH1106 OLED and servo output.

## Download the complete source

**[Download the complete SmartDoor source ZIP](./SmartDoor-Face-Security-System-source.zip)**

The ZIP contains the full source layout:
- `python/smart_door_system.py` — Tkinter dashboard, face registration/training, live recognition, local HTTP API, and security recording.
- `python/requirements.txt` — Python dependencies.
- `android/MainActivity.kt` — Android UI, fingerprint prompt, PC API requests, and BLE commands.
- `android/AndroidManifest.xml` — prototype permissions/network configuration reference.
- `esp32/ESP-DLS/ESP-DLS.ino` — ESP32 BLE/OLED/servo firmware.
- `docs/SETUP.md` — setup and troubleshooting guide.
- `.gitignore` — private data and local build exclusions.

The repository also exposes the README, setup guide, Python requirements, and ESP32 firmware individually. The Python dashboard and Android integration source are included in the downloadable ZIP rather than as separate root-level files.

## Quick start

1. Download and extract the ZIP.
2. Follow `docs/SETUP.md`.
3. Start IP Webcam on the phone and the Python dashboard on the PC.
4. Enter the PC's current IPv4 address in the Android app as `http://YOUR_PC_IPV4:8765`.
5. Upload the ESP32 sketch using Arduino IDE and connect from the Android app.

## Hardware notes

- OLED: SH1106 I2C, SDA GPIO 21, SCL GPIO 22 (verify display controller and address).
- Servo signal: GPIO 4.
- BLE name: `ESP32_DOOR_LOCK`.
- BLE service UUID: `12345678-1234-1234-1234-123456789abc`.
- BLE characteristic UUID: `abcdefab-1234-1234-1234-abcdefabcdef`.

## Privacy and safety

This is a prototype, not production-ready door access control. LBPH recognition is not robust liveness detection and may accept photos/screens or misclassify people. Use a non-critical demo lock, retain manual exit, and test with the actuator disconnected before connecting a physical lock.

Do not commit enrolled face images, recordings, device-specific IP addresses, Android `local.properties`, passwords, API keys, or signing keys. The `.gitignore` excludes common face/recording folders and local build output. Review changes before making this repository public.
