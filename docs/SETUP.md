# SmartDoor setup guide

## 1. Python dashboard

Use Python 3.11 or 3.12 if possible for library compatibility. From the repository root:

```powershell
py -3.11 -m venv .venv
.\.venv\Scripts\Activate.ps1
python -m pip install --upgrade pip
pip install -r python\requirements.txt
python python\smart_door_system.py
```

If your existing environment uses a working OpenCV build with `cv2.face`, keep it. The face recognizer needs OpenCV contrib modules (`cv2.face`). Register your face from the dashboard. Keep enrolled data in the local `faces/` folder and recordings in the local `recordings/` folder; do not upload them.

On the phone, start IP Webcam and use its actual `/video` URL in the PC dashboard. Replace the example phone address in the dashboard with the current address shown by IP Webcam. Allow background activity for IP Webcam on Samsung if needed.

## 2. PC API and Android URL

The Python dashboard's local API listens on port `8765`. Find the PC's IPv4 address with `ipconfig`. In the Android app's PC Security Server URL, enter:

`http://YOUR_PC_IPV4:8765`

Replace `YOUR_PC_IPV4` with the actual IPv4 address on the same reachable Wi-Fi/LAN as the phone. Example only: `http://192.168.1.100:8765`. If Windows Firewall blocks the phone, allow the Python server TCP port 8765 on a trusted private network only.

Test locally from the PC browser with `http://127.0.0.1:8765/status`; test from the phone browser with `http://YOUR_PC_IPV4:8765/status`.

## 3. Android app

Copy `android/MainActivity.kt` to the matching source package in your existing Android Studio project and merge `android/AndroidManifest.xml` permissions/settings into that project's manifest rather than deleting project-specific content. Ensure the BLE service and characteristic UUIDs match the firmware. Build and install from Android Studio.

The fingerprint prompt authenticates a fingerprint enrolled on the phone. The prototype does not independently identify different people through their fingerprints.

## 4. ESP32 / Arduino IDE

Open `esp32/ESP-DLS/ESP-DLS.ino`. Install `U8g2` and `ESP32Servo` through Library Manager; install the Espressif ESP32 board package and select the board and COM port matching the module. The firmware uses:

- OLED: SH1106 I2C, SDA GPIO 21, SCL GPIO 22 (controller and I2C address must match the display).
- Servo signal: GPIO 4.
- BLE name: `ESP32_DOOR_LOCK`.
- BLE service UUID: `12345678-1234-1234-1234-123456789abc`.
- BLE characteristic UUID: `abcdefab-1234-1234-1234-abcdefabcdef`.

Power a servo or electric lock from a separately rated supply and use an appropriate driver; never drive a lock directly from an ESP32 GPIO. Join grounds where required by the driver circuit. Adjust actuator positions for the mechanism and test without a physical door first.

## 5. Authentication and recording flow

The Android app checks the PC face API and sends source-aware BLE commands such as `FACE_GRANTED`, `FACE_DENIED`, `FINGER_GRANTED`, `FINGER_DENIED`, and recording status commands. The ESP32 uses them to update the OLED and control the prototype servo. Video recording is performed by the Python PC dashboard, not by the ESP32. Recording depends on camera availability, disk space, and encoder support.

## Troubleshooting

- `cv2.face` missing: install `opencv-contrib-python` in the active environment and avoid installing conflicting OpenCV wheel variants side by side.
- Phone cannot reach PC API: verify phone and PC can communicate on the same LAN, confirm the current PC IPv4 address, keep the Python server running, and check Windows Firewall on a trusted network.
- OLED blank: check SH1106 versus SSD1306 controller, SDA/SCL wiring, supply voltage, and I2C address.
- BLE not found: verify firmware upload, device name, BLE UUIDs, and Android permissions.
