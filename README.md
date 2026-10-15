# SmartDoor Face Security System

A student prototype combining phone-based IP Webcam streaming, Python/OpenCV face recognition, an Android app using the phone's biometric prompt, BLE communication, and an ESP32 with an SH1106 OLED and servo output.

## Repository contents

- `python/smart_door_system.py` — Tkinter dashboard, face registration/training, live recognition, local HTTP API, and security recording.
- `python/requirements.txt` — Python dependencies.
- `android/MainActivity.kt` — Android UI, fingerprint prompt, PC API requests, and BLE commands.
- `android/AndroidManifest.xml` — manifest reference for prototype permissions and network configuration.
- `esp32/ESP-DLS/ESP-DLS.ino` — ESP32 BLE/OLED/servo firmware.
- `docs/SETUP.md` — installation and operating guide.
- `SmartDoor-Face-Security-System-source.zip` — complete source bundle with the same project files.
- `.gitignore` — excludes local face samples, security recordings, IDE/build output, and local configuration.

> The Android folder contains the integration source files available for this handoff, not a complete Android Studio/Gradle project. Copy them into the existing `SmartDoorLock` Android Studio project; include the remaining Gradle wrapper/build files from that working project if you want a clean clone to build independently.

## Privacy and safety

This is a prototype, not production-ready door access control. LBPH recognition is not robust liveness detection and may accept photos/screens or misclassify people. Use a non-critical demo lock, retain manual exit, and test with the actuator disconnected before connecting a physical lock.

Do not commit enrolled face images, recordings, device-specific IP addresses, Android `local.properties`, passwords, API keys, or signing keys. Review changes before making this repository public.
