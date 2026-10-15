#include <Wire.h>
#include <U8g2lib.h>
#include <ESP32Servo.h>
#include <BLEDevice.h>
#include <BLEServer.h>
#include <BLEUtils.h>

// SmartDoor OLED + BLE firmware
// OLED: SH1106 128x64 I2C, SDA=21, SCL=22
// Servo signal GPIO 4. Use a separate regulated 5V supply for the servo,
// and connect its ground to ESP32 GND. Do not power a buzzing servo from 3V3.

static const char* DEVICE_NAME = "ESP32_DOOR_LOCK";
static const char* SERVICE_UUID = "12345678-1234-1234-1234-123456789abc";
static const char* CHARACTERISTIC_UUID = "abcdefab-1234-1234-1234-abcdefabcdef";

static const int SERVO_PIN = 4;
static const int LOCKED_ANGLE = 0;
static const int UNLOCKED_ANGLE = 90;
static const unsigned long AUTO_LOCK_MS = 3500;
static const unsigned long EVENT_SCREEN_MS = 5000;

U8G2_SH1106_128X64_NONAME_F_HW_I2C u8g2(U8G2_R0, U8X8_PIN_NONE);
Servo lockServo;

BLEServer* bleServer = nullptr;
BLECharacteristic* commandCharacteristic = nullptr;

// A BLE callback only queues the command; the main loop updates the OLED/servo.
char pendingCommand[64] = {0};
portMUX_TYPE commandMux = portMUX_INITIALIZER_UNLOCKED;
volatile bool commandPending = false;
volatile bool bleConnected = false;
volatile bool bleStateChanged = false;

bool unlocked = false;
unsigned long unlockStartedAt = 0;
bool eventScreenActive = false;
unsigned long eventScreenUntil = 0;
String screenTitle = "SYSTEM READY";
String screenSource = "WAITING FOR AUTH";
String screenDetail = "Use Face ID / Fingerprint";
String lastAuthSource = "NONE";

class DoorServerCallbacks : public BLEServerCallbacks {
  void onConnect(BLEServer* server) override {
    bleConnected = true;
    bleStateChanged = true;
  }

  void onDisconnect(BLEServer* server) override {
    bleConnected = false;
    bleStateChanged = true;
    BLEDevice::startAdvertising();
  }
};

class DoorCommandCallbacks : public BLECharacteristicCallbacks {
  void onWrite(BLECharacteristic* characteristic) override {
    // This ESP32 Arduino BLE version returns Arduino String from getValue().
    String command = characteristic->getValue();
    command.trim();
    if (command.length() == 0) return;

    char commandBuffer[64] = {0};
    command.toCharArray(commandBuffer, sizeof(commandBuffer));
    portENTER_CRITICAL(&commandMux);
    strncpy(pendingCommand, commandBuffer, sizeof(pendingCommand) - 1);
    pendingCommand[sizeof(pendingCommand) - 1] = '\0';
    commandPending = true;
    portEXIT_CRITICAL(&commandMux);
  }
};

void showEvent(const String& title, const String& source, const String& detail,
               unsigned long duration = EVENT_SCREEN_MS) {
  screenTitle = title;
  screenSource = source;
  screenDetail = detail;
  eventScreenActive = true;
  eventScreenUntil = millis() + duration;
}

void showGranted(const String& source) {
  lastAuthSource = source;
  lockServo.write(UNLOCKED_ANGLE);
  unlocked = true;
  unlockStartedAt = millis();
  showEvent("ACCESS GRANTED", "SOURCE: " + source, "DOOR UNLOCKED", AUTO_LOCK_MS);
}

void showDenied(const String& source) {
  lastAuthSource = source;
  // A denied authentication never moves the servo.
  showEvent("ID IS WRONG!", "SOURCE: " + source, "SECURITY ALERT SENT");
}

void showRecordingRequested(const String& source) {
  lastAuthSource = source;
  showEvent("ID IS WRONG!", "SOURCE: " + source, "RECORDING REQUESTED", 6000);
}

void processCommand(String command) {
  command.trim();
  command.toUpperCase();

  if (command == "FACE_GRANTED") {
    showGranted("FACE ID");
  } else if (command == "FINGER_GRANTED") {
    showGranted("FINGERPRINT");
  } else if (command == "FACE_DENIED") {
    showDenied("FACE ID");
  } else if (command == "FINGER_DENIED") {
    showDenied("FINGERPRINT");
  } else if (command == "FACE_RECORDING") {
    showRecordingRequested("FACE ID");
  } else if (command == "FINGER_RECORDING") {
    showRecordingRequested("FINGERPRINT");
  } else if (command == "FACE_RECORD_FAILED") {
    showEvent("ID IS WRONG!", "SOURCE: FACE ID", "PC RECORDING FAILED", 6000);
  } else if (command == "FINGER_RECORD_FAILED") {
    showEvent("ID IS WRONG!", "SOURCE: FINGERPRINT", "PC RECORDING FAILED", 6000);
  } else if (command == "LOCK") {
    lockServo.write(LOCKED_ANGLE);
    unlocked = false;
    showEvent("DOOR LOCKED", "MANUAL LOCK", "LOCK POSITION SET", 2500);
  } else if (command == "GRANTED") {
    // Backward-compatible with the earlier Android build.
    showGranted("AUTHENTICATION");
  } else if (command == "DENIED") {
    // Backward-compatible with the earlier Android build.
    showDenied("UNKNOWN ID");
  } else {
    showEvent("UNKNOWN COMMAND", command.substring(0, 18), "NO ACTION TAKEN", 2500);
  }
}

void drawCentered(const String& text, int y) {
  int width = u8g2.getStrWidth(text.c_str());
  int x = (128 - width) / 2;
  if (x < 0) x = 0;
  u8g2.drawStr(x, y, text.c_str());
}

void drawUI() {
  u8g2.clearBuffer();
  u8g2.setFont(u8g2_font_5x8_tf);
  u8g2.drawStr(2, 8, "SMARTDOOR SECURITY");
  u8g2.drawStr(103, 8, bleConnected ? "BLE+" : "BLE-");
  u8g2.drawHLine(0, 12, 128);

  if (eventScreenActive && (long)(millis() - eventScreenUntil) >= 0) {
    eventScreenActive = false;
  }

  if (eventScreenActive) {
    u8g2.setFont(u8g2_font_helvB12_tf);
    drawCentered(screenTitle, 31);
    u8g2.setFont(u8g2_font_6x10_tf);
    drawCentered(screenSource, 46);
    drawCentered(screenDetail, 60);
  } else {
    u8g2.setFont(u8g2_font_helvB12_tf);
    drawCentered(unlocked ? "DOOR UNLOCKED" : "SYSTEM READY", 31);
    u8g2.setFont(u8g2_font_6x10_tf);
    if (!bleConnected) {
      drawCentered("Waiting for Android app", 46);
      drawCentered("Open app and connect BLE", 60);
    } else {
      drawCentered("BLE CONNECTED", 46);
      drawCentered("Face ID / Fingerprint", 60);
    }
  }
  u8g2.sendBuffer();
}

void setup() {
  Serial.begin(115200);
  Wire.begin(21, 22);
  u8g2.begin();
  showEvent("SMARTDOOR", "STARTING SYSTEM", "PLEASE WAIT", 1500);
  drawUI();

  lockServo.setPeriodHertz(50);
  lockServo.attach(SERVO_PIN, 500, 2400);
  lockServo.write(LOCKED_ANGLE);
  delay(250);

  BLEDevice::init(DEVICE_NAME);
  bleServer = BLEDevice::createServer();
  bleServer->setCallbacks(new DoorServerCallbacks());
  BLEService* service = bleServer->createService(SERVICE_UUID);
  commandCharacteristic = service->createCharacteristic(
    CHARACTERISTIC_UUID,
    BLECharacteristic::PROPERTY_READ |
    BLECharacteristic::PROPERTY_WRITE |
    BLECharacteristic::PROPERTY_WRITE_NR
  );
  commandCharacteristic->setValue("READY");
  commandCharacteristic->setCallbacks(new DoorCommandCallbacks());
  service->start();

  BLEAdvertising* advertising = BLEDevice::getAdvertising();
  advertising->addServiceUUID(SERVICE_UUID);
  advertising->setScanResponse(true);
  BLEDevice::startAdvertising();

  Serial.println("SmartDoor ready; advertising as ESP32_DOOR_LOCK.");
  showEvent("SYSTEM READY", "DOOR LOCKED", "WAITING FOR AUTH", 1800);
  drawUI();
}

void loop() {
  char commandBuffer[64] = {0};
  portENTER_CRITICAL(&commandMux);
  if (commandPending) {
    strncpy(commandBuffer, pendingCommand, sizeof(commandBuffer) - 1);
    pendingCommand[0] = '\0';
    commandPending = false;
  }
  portEXIT_CRITICAL(&commandMux);
  String command = String(commandBuffer);

  if (command.length() > 0) {
    Serial.print("BLE command: ");
    Serial.println(command);
    processCommand(command);
    drawUI();
  }

  if (unlocked && millis() - unlockStartedAt >= AUTO_LOCK_MS) {
    lockServo.write(LOCKED_ANGLE);
    unlocked = false;
    showEvent("DOOR LOCKED", "LAST: " + lastAuthSource, "AUTO-LOCK COMPLETE", 2200);
    drawUI();
  }

  if (bleStateChanged) {
    bleStateChanged = false;
    if (!eventScreenActive) drawUI();
  }

  static unsigned long lastRedraw = 0;
  if (millis() - lastRedraw >= 200) {
    lastRedraw = millis();
    drawUI();
  }
  delay(10);
}
