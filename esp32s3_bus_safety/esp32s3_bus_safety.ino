#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>
#include <WiFi.h>
#include <HTTPClient.h>
#include <esp_system.h>
#include <WiFiClientSecure.h>
#include <PubSubClient.h>
#include <ArduinoJson.h>
#include <time.h>
#include <Preferences.h>
#include "inputs.h"
#if __has_include("bus_config.h")
#include "bus_config.h"
#else
#include "config.example.h"
#endif

struct TelemetrySnapshot {
  bool ignition;
  bool pir;
  bool ready;
  bool alarm;
  bool checking;
  bool fan;
  bool buzzer;
  uint16_t passengers;
  uint32_t inspection;
  uint32_t boarded;
  uint32_t alighted;
  uint32_t remaining;
};
QueueHandle_t telemetryQueue;
QueueHandle_t commandQueue;
struct RemoteCommand { int code; int people; char id[40]; };
QueueHandle_t ackQueue;
bool remoteFanOn = false;
uint32_t remoteFanStarted = 0;
bool buzzerSilenced = false;
uint32_t buzzerSilencedAt = 0;
uint16_t aiPeople = 0;
uint32_t aiUpdated = 0;
uint32_t inspectionSequence = 0;
uint32_t bootId = 0;
uint32_t totalBoarded = 0;
uint32_t totalAlighted = 0;
Preferences storage;

// Network I/O runs outside loop(): sensor sampling and local alarms keep running.
void networkTask(void *) {
  WiFi.mode(WIFI_STA);
  WiFi.setAutoReconnect(true);
  configTime(0, 0, "pool.ntp.org", "time.nist.gov");
  WiFiClientSecure tls;
  tls.setCACert(MQTT_ROOT_CA);
  PubSubClient mqtt(tls);
  mqtt.setServer(MQTT_HOST, MQTT_PORT);
  mqtt.setBufferSize(1024);
  mqtt.setSocketTimeout(2);
  mqtt.setCallback([&](char *topic, byte *bytes, unsigned int length) {
    JsonDocument command;
    if (deserializeJson(command, bytes, length)) return;
    if (String(command["device_id"] | "") != DEVICE_ID) return;
    if (time(nullptr) > command["expires_at"].as<long>()) return;
    String action = command["action"] | "";
    int code = action == "FAN_ON" ? 1 : (action == "FAN_AUTO" ? 0 : (action == "AI_PRESENCE" ? 2 : (action == "BUZZER_SILENCE" ? 3 : (action == "BUZZER_AUTO" ? 4 : -1))));
    if (code < 0 || !commandQueue) return;
    RemoteCommand received;
    received.code = code;
    received.people = command["person_count"] | 0;
    strlcpy(received.id, command["command_id"] | "", sizeof(received.id));
    if (!strlen(received.id)) return;
    xQueueOverwrite(commandQueue, &received);
  });
  uint32_t lastMqttAttempt = millis() - 10000;
  uint32_t lastAttempt = millis() - 10000;
  for (;;) {
    TelemetrySnapshot sample;
    if (xQueueReceive(telemetryQueue, &sample, pdMS_TO_TICKS(1000)) != pdTRUE) continue;
    if (WiFi.status() != WL_CONNECTED) {
      if (millis() - lastAttempt >= 10000) {
        WiFi.begin(WIFI_SSID, WIFI_PASSWORD);
        lastAttempt = millis();
      }
      continue;
    }
    if (strlen(MQTT_HOST) && !mqtt.connected() && millis() - lastMqttAttempt >= 10000 && time(nullptr) > 1700000000) {
      lastMqttAttempt = millis();
      if (mqtt.connect(DEVICE_ID, MQTT_USERNAME, MQTT_PASSWORD)) mqtt.subscribe((String("bus/") + DEVICE_ID + "/commands").c_str(), 1);
    }
    mqtt.loop();
    RemoteCommand applied;
    if (mqtt.connected() && ackQueue && xQueuePeek(ackQueue, &applied, 0) == pdTRUE) {
      String ack = String("{\"device_id\":\"") + DEVICE_ID + "\",\"command_id\":\"" + applied.id + "\"}";
      if (mqtt.publish((String("bus/") + DEVICE_ID + "/acks").c_str(), ack.c_str())) xQueueReceive(ackQueue, &applied, 0);
    }
    char body[512];
    char inspection[48];
    snprintf(inspection, sizeof(inspection), "%lu-%lu", (unsigned long)bootId, (unsigned long)sample.inspection);
    snprintf(body, sizeof(body),
      "{\"device_id\":\"%s\",\"state\":\"%s\",\"ignition_on\":%s,\"passenger_count\":%u,\"pir_detected\":%s,\"pir_ready\":%s,\"fan_on\":%s,\"inspection_id\":%s}",
      DEVICE_ID, sample.alarm ? "WARNING" : (sample.checking ? "CHECK" : "NORMAL"),
      sample.ignition ? "true" : "false", sample.passengers,
      sample.pir ? "true" : "false", sample.ready ? "true" : "false",
      sample.fan ? "true" : "false", "null");
    // Replace null with the stable inspection token only after a physical inspection.
    String payload(body);
    if (sample.inspection > 0) payload.replace("\"inspection_id\":null", String("\"inspection_id\":\"") + inspection + "\"");
    JsonDocument telemetry;
    deserializeJson(telemetry, payload);
    telemetry["total_boarded"] = sample.boarded;
    telemetry["total_alighted"] = sample.alighted;
    telemetry["check_remaining"] = sample.remaining;
    telemetry["buzzer_on"] = sample.buzzer;
    payload = "";
    serializeJson(telemetry, payload);
    if (strlen(MQTT_HOST)) {
      if (mqtt.connected()) mqtt.publish((String("bus/") + DEVICE_ID + "/telemetry").c_str(), payload.c_str(), false);
      continue;
    }
    WiFiClient client;
    HTTPClient http;
    http.setConnectTimeout(1500);
    http.setTimeout(1500);
    if (http.begin(client, String(API_BASE_URL) + "/api/telemetry")) {
      http.addHeader("Content-Type", "application/json");
      http.addHeader("X-Device-Key", DEVICE_API_KEY);
      int result = http.POST(payload);
      if (result < 200 || result >= 300) Serial.printf("Telemetry failed: %d\n", result);
      http.end();
    }
  }
}

// ESP32-S3 bus safety prototype. Pin map follows the supplied wiring notes.
// The two LM393 modules in the photos are used as sequential door sensors.
constexpr uint8_t PIN_IR_1 = 4;
constexpr uint8_t PIN_PIR = 5;
constexpr uint8_t PIN_IR_2 = 6;
constexpr uint8_t PIN_OLED_SDA = 8;
constexpr uint8_t PIN_OLED_SCL = 9;
constexpr uint8_t PIN_IGNITION = 10;
constexpr uint8_t PIN_SAFETY_BUTTON = 11;
constexpr uint8_t PIN_SAFETY_BUTTON_2 = 7;  // Optional second KY-004 from the updated wiring notes.
constexpr uint8_t PIN_BUZZER = 12;
constexpr uint8_t PIN_RELAY = 13;
constexpr uint8_t PIN_LED_RED = 14;
constexpr uint8_t PIN_LED_GREEN = 15;
constexpr uint8_t PIN_LED_BLUE = 16;

constexpr uint8_t OLED_ADDRESS = 0x3C;
constexpr uint32_t PIR_WARMUP_MS = 30000;
constexpr uint32_t INPUT_DEBOUNCE_MS = 40;
constexpr uint32_t IR_SEQUENCE_TIMEOUT_MS = 1500;
constexpr uint32_t CABIN_CHECK_MS = 15000;
constexpr uint32_t DISPLAY_REFRESH_MS = 250;
constexpr uint32_t SERIAL_REFRESH_MS = 1000;

// Adjust these if a module's output logic is opposite to the common wiring.
constexpr uint8_t IR_BLOCKED_LEVEL = LOW;  // LM393 obstacle modules commonly pull OUT low.
constexpr bool SAFETY_BUTTON_ACTIVE_LOW = true;
constexpr bool RELAY_ACTIVE_LOW = true;    // Common for 5V relay boards.
constexpr bool RGB_COMMON_ANODE = false;   // Wokwi setup in the checklist is common cathode.

constexpr int SCREEN_WIDTH = 128;
constexpr int SCREEN_HEIGHT = 64;
Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, -1);

enum DoorSequence : uint8_t { DOOR_NONE, IR1_FIRST, IR2_FIRST };

DebouncedInput ir1Input = {false, false, 0};
DebouncedInput ir2Input = {false, false, 0};
DebouncedInput buttonInput = {false, false, 0};
DebouncedInput button2Input = {false, false, 0};

bool displayReady = false;
bool alarmLatched = false;
bool cabinCheckPending = false;
bool previousIgnitionOn = false;
uint32_t cabinCheckStartedMs = 0;
uint32_t lastDisplayMs = 0;
uint32_t lastSerialMs = 0;
uint16_t passengerCount = 0;
DoorSequence doorSequence = DOOR_NONE;
uint32_t doorSequenceStartedMs = 0;
bool doorSecondSeen = false;
bool doorTurnedBack = false;
bool doorWaitClear = false;

bool isActiveLevel(uint8_t pin, uint8_t activeLevel) {
  return digitalRead(pin) == activeLevel;
}

bool updateDebouncedInput(uint8_t pin, uint8_t activeLevel,
                          DebouncedInput &input, uint32_t now) {
  const bool rawActive = isActiveLevel(pin, activeLevel);
  if (rawActive != input.rawActive) {
    input.rawActive = rawActive;
    input.lastChangeMs = now;
  }
  if (rawActive != input.stableActive &&
      (now - input.lastChangeMs) >= INPUT_DEBOUNCE_MS) {
    input.stableActive = rawActive;
    return input.stableActive;  // report only the transition into active
  }
  return false;
}

void writeActiveLevel(uint8_t pin, bool active, bool activeLow) {
  const bool outputHigh = active ? !activeLow : activeLow;
  digitalWrite(pin, outputHigh ? HIGH : LOW);
}

void writeLed(uint8_t pin, bool on) {
  const bool outputHigh = RGB_COMMON_ANODE ? !on : on;
  digitalWrite(pin, outputHigh ? HIGH : LOW);
}

void setRgb(bool red, bool green, bool blue) {
  writeLed(PIN_LED_RED, red);
  writeLed(PIN_LED_GREEN, green);
  writeLed(PIN_LED_BLUE, blue);
}

void applyOutputs() {
  if (alarmLatched) {
    setRgb(true, false, false);
    writeActiveLevel(PIN_BUZZER, true, false);
    writeActiveLevel(PIN_RELAY, true, RELAY_ACTIVE_LOW);
  } else if (cabinCheckPending) {
    setRgb(false, false, true);
    writeActiveLevel(PIN_BUZZER, false, false);
    writeActiveLevel(PIN_RELAY, false, RELAY_ACTIVE_LOW);
  } else {
    setRgb(false, true, false);
    writeActiveLevel(PIN_BUZZER, false, false);
    writeActiveLevel(PIN_RELAY, false, RELAY_ACTIVE_LOW);
  }
  // Remote FAN_AUTO never overrides an active local alarm.
  if (remoteFanOn) writeActiveLevel(PIN_RELAY, true, RELAY_ACTIVE_LOW);
  if (buzzerSilenced) writeActiveLevel(PIN_BUZZER, false, false);
}

bool ignitionIsOn() {
  return digitalRead(PIN_IGNITION) == HIGH;
}

void startCabinCheck(const __FlashStringHelper *reason, uint32_t now) {
  if (alarmLatched || cabinCheckPending) return;
  cabinCheckPending = true;
  cabinCheckStartedMs = now;
  Serial.print(F("Cabin check started: "));
  Serial.println(reason);
  applyOutputs();
}

void raiseAlarm(const __FlashStringHelper *reason) {
  cabinCheckPending = false;
  alarmLatched = true;
  storage.putBool("alarm", true);
  Serial.print(F("ALARM: "));
  Serial.println(reason);
  applyOutputs();
}

void clearAfterInspection(bool pirDetected) {
  if (pirDetected || (aiPeople > 0 && millis() - aiUpdated < 15000)) {
    Serial.println(F("Button pressed, but PIR still detects motion; alarm remains."));
    return;
  }
  cabinCheckPending = false;
  alarmLatched = false;
  passengerCount = 0;
  storage.putUShort("count", passengerCount);
  storage.putBool("alarm", false);
  ++inspectionSequence;
  doorSequence = DOOR_NONE;
  Serial.println(F("Cabin check confirmed. Passenger count reset to 0."));
  applyOutputs();
}

void changePassengerCount(int delta) {
  if (delta > 0) {
    ++totalBoarded;
    if (passengerCount < 999) ++passengerCount;
    Serial.print(F("Passenger entered. Count="));
    Serial.println(passengerCount);
  } else if (passengerCount > 0) {
    ++totalAlighted;
    --passengerCount;
    Serial.print(F("Passenger exited. Count="));
    Serial.println(passengerCount);
  } else {
    Serial.println(F("Exit detected while count is already 0; count unchanged."));
  }
  storage.putUShort("count", passengerCount);
  storage.putUInt("boarded", totalBoarded);
  storage.putUInt("alighted", totalAlighted);
}

void processDoorEvents(bool ir1Blocked, bool ir2Blocked, uint32_t now) {
  const bool first = ir1Input.stableActive;
  const bool second = ir2Input.stableActive;
  if (doorWaitClear) {
    if (!first && !second) doorWaitClear = false;
    return;
  }
  if (doorSequence != DOOR_NONE &&
      (now - doorSequenceStartedMs) > IR_SEQUENCE_TIMEOUT_MS) {
    doorSequence = DOOR_NONE;
    doorSecondSeen = false;
    doorWaitClear = first || second;
    return;
  }

  // If both sensors trigger in the same loop, direction is ambiguous. Ignore it.
  if (ir1Blocked && ir2Blocked) {
    doorSequence = DOOR_NONE;
    doorWaitClear = true;
    Serial.println(F("Both door sensors triggered together; direction ignored."));
    return;
  }

  if (doorSequence == DOOR_NONE) {
    if (!ir1Blocked && !ir2Blocked) return;
    doorSequence = ir1Blocked ? IR1_FIRST : IR2_FIRST;
    doorSequenceStartedMs = now;
    doorSecondSeen = false;
    doorTurnedBack = false;
  } else if ((doorSequence == IR1_FIRST && ir2Blocked) ||
             (doorSequence == IR2_FIRST && ir1Blocked)) {
    doorSecondSeen = true;
  }
  // Count only once the full crossing has cleared both beams. Returning to the
  // initial beam after reaching the second beam cancels a turnaround.
  if (doorSecondSeen) {
    if (doorSequence == IR1_FIRST && first && !second) doorTurnedBack = true;
    if (doorSequence == IR2_FIRST && second && !first) doorTurnedBack = true;
    if (!first && !second) {
      if (!doorTurnedBack) changePassengerCount(doorSequence == IR1_FIRST ? 1 : -1);
      doorSequence = DOOR_NONE;
      doorSecondSeen = false;
    }
  }
}

void drawStatus(bool ignitionOn, bool pirDetected, bool pirReady, uint32_t now) {
  if (!displayReady) return;

  display.clearDisplay();
  display.setTextColor(SSD1306_WHITE);
  display.setTextSize(1);
  display.setCursor(0, 0);
  display.println(F("SMART BUS #01"));
  display.println(F("----------------"));

  if (alarmLatched) {
    display.setTextSize(2);
    display.println(F("WARNING!"));
    display.setTextSize(1);
    display.print(F("PEOPLE: "));
    display.println(passengerCount);
    display.println(F("FAN: ON / BUZZER: ON"));
    display.println(F("Check cabin, press key"));
  } else if (cabinCheckPending) {
    const uint32_t elapsed = now - cabinCheckStartedMs;
    const uint32_t remaining = elapsed < CABIN_CHECK_MS
        ? (CABIN_CHECK_MS - elapsed + 999) / 1000
        : 0;
    display.setTextSize(2);
    display.println(F("CHECK BUS"));
    display.setTextSize(1);
    display.print(F("TIME LEFT: "));
    display.print(remaining);
    display.println(F("s"));
    display.print(F("PEOPLE: "));
    display.println(passengerCount);
    display.println(F("Press rear button"));
  } else {
    display.print(F("IGNITION: "));
    display.println(ignitionOn ? F("ON") : F("OFF"));
    display.print(F("PASSENGERS: "));
    display.println(passengerCount);
    display.print(F("PIR: "));
    if (!pirReady) {
      const uint32_t remaining = now < PIR_WARMUP_MS
          ? (PIR_WARMUP_MS - now + 999) / 1000
          : 0;
      display.print(F("WARMUP "));
      display.print(remaining);
      display.println(F("s"));
    } else {
      display.println(pirDetected ? F("MOTION") : F("CLEAR"));
    }
    display.println(F("STATE: NORMAL"));
  }
  display.display();
}

void printStatus(bool ignitionOn, bool pirDetected, bool pirReady) {
  Serial.print(F("STATE="));
  Serial.print(alarmLatched ? F("WARNING") : (cabinCheckPending ? F("CHECK") : F("NORMAL")));
  Serial.print(F(",IGNITION="));
  Serial.print(ignitionOn ? F("ON") : F("OFF"));
  Serial.print(F(",PASSENGERS="));
  Serial.print(passengerCount);
  Serial.print(F(",PIR="));
  Serial.print(pirDetected ? 1 : 0);
  Serial.print(F(",PIR_READY="));
  Serial.print(pirReady ? 1 : 0);
  Serial.print(F(",FAN="));
  Serial.println(alarmLatched ? 1 : 0);
}

void setup() {
  Serial.begin(115200);
  storage.begin("bus-safety", false);
  passengerCount = storage.getUShort("count", 0);
  if (passengerCount > 999) passengerCount = 999;
  alarmLatched = storage.getBool("alarm", false);
  totalBoarded = storage.getUInt("boarded", 0);
  totalAlighted = storage.getUInt("alighted", 0);
  bootId = esp_random();
  telemetryQueue = xQueueCreate(1, sizeof(TelemetrySnapshot));
  commandQueue = xQueueCreate(1, sizeof(RemoteCommand));
  ackQueue = xQueueCreate(1, sizeof(RemoteCommand));
  if (telemetryQueue) xTaskCreate(networkTask, "network", 8192, nullptr, 1, nullptr);

  // Initialize actuators in their inactive state before enabling output mode.
  digitalWrite(PIN_BUZZER, LOW);
  pinMode(PIN_BUZZER, OUTPUT);
  digitalWrite(PIN_RELAY, RELAY_ACTIVE_LOW ? HIGH : LOW);
  pinMode(PIN_RELAY, OUTPUT);
  digitalWrite(PIN_LED_RED, RGB_COMMON_ANODE ? HIGH : LOW);
  digitalWrite(PIN_LED_GREEN, RGB_COMMON_ANODE ? HIGH : LOW);
  digitalWrite(PIN_LED_BLUE, RGB_COMMON_ANODE ? HIGH : LOW);
  pinMode(PIN_LED_RED, OUTPUT);
  pinMode(PIN_LED_GREEN, OUTPUT);
  pinMode(PIN_LED_BLUE, OUTPUT);

  pinMode(PIN_IR_1, INPUT_PULLUP);
  pinMode(PIN_IR_2, INPUT_PULLUP);
  pinMode(PIN_PIR, INPUT);
  pinMode(PIN_IGNITION, INPUT_PULLDOWN);  // Switch connects GPIO10 to 3V3.
  pinMode(PIN_SAFETY_BUTTON,
          SAFETY_BUTTON_ACTIVE_LOW ? INPUT_PULLUP : INPUT_PULLDOWN);
  pinMode(PIN_SAFETY_BUTTON_2,
          SAFETY_BUTTON_ACTIVE_LOW ? INPUT_PULLUP : INPUT_PULLDOWN);

  const uint32_t now = millis();
  ir1Input.rawActive = ir1Input.stableActive = isActiveLevel(PIN_IR_1, IR_BLOCKED_LEVEL);
  ir2Input.rawActive = ir2Input.stableActive = isActiveLevel(PIN_IR_2, IR_BLOCKED_LEVEL);
  buttonInput.rawActive = buttonInput.stableActive =
      isActiveLevel(PIN_SAFETY_BUTTON, SAFETY_BUTTON_ACTIVE_LOW ? LOW : HIGH);
  button2Input.rawActive = button2Input.stableActive =
      isActiveLevel(PIN_SAFETY_BUTTON_2, SAFETY_BUTTON_ACTIVE_LOW ? LOW : HIGH);
  ir1Input.lastChangeMs = ir2Input.lastChangeMs = now;
  buttonInput.lastChangeMs = button2Input.lastChangeMs = now;

  Wire.begin(PIN_OLED_SDA, PIN_OLED_SCL);
  displayReady = display.begin(SSD1306_SWITCHCAPVCC, OLED_ADDRESS);
  if (!displayReady) Serial.println(F("OLED not found at 0x3C; check wiring/address."));

  previousIgnitionOn = ignitionIsOn();
  applyOutputs();
  Serial.println(F("Firmware ready. Passenger count/alarm restored from NVS."));
  Serial.println(F("Wait 30 seconds for HC-SR501 warm-up."));
}

void loop() {
  const uint32_t now = millis();
  RemoteCommand remoteCommand;
  if (commandQueue && xQueueReceive(commandQueue, &remoteCommand, 0) == pdTRUE) {
    if (remoteCommand.code == 2) {
      aiPeople = constrain(remoteCommand.people, 0, 999);
      aiUpdated = now;
    } else if (remoteCommand.code == 3 || remoteCommand.code == 4) {
      buzzerSilenced = remoteCommand.code == 3;
      buzzerSilencedAt = now;
    } else {
      remoteFanOn = remoteCommand.code == 1;
      remoteFanStarted = now;
    }
    applyOutputs();
    if (ackQueue) xQueueOverwrite(ackQueue, &remoteCommand);
  }
  if (remoteFanOn && now - remoteFanStarted > 300000) {
    remoteFanOn = false;
    applyOutputs();
  }
  if (buzzerSilenced && now - buzzerSilencedAt >= 30000) {
    buzzerSilenced = false;
    applyOutputs();
  }
  const bool ignitionOn = ignitionIsOn();
  const bool pirDetected = digitalRead(PIN_PIR) == HIGH;
  const bool pirReady = now >= PIR_WARMUP_MS;

  const bool ir1Event = updateDebouncedInput(PIN_IR_1, IR_BLOCKED_LEVEL, ir1Input, now);
  const bool ir2Event = updateDebouncedInput(PIN_IR_2, IR_BLOCKED_LEVEL, ir2Input, now);
  processDoorEvents(ir1Event, ir2Event, now);

  const bool button1Pressed = updateDebouncedInput(
      PIN_SAFETY_BUTTON,
      SAFETY_BUTTON_ACTIVE_LOW ? LOW : HIGH,
      buttonInput,
      now);
  const bool button2Pressed = updateDebouncedInput(
      PIN_SAFETY_BUTTON_2,
      SAFETY_BUTTON_ACTIVE_LOW ? LOW : HIGH,
      button2Input,
      now);
  if ((button1Pressed || button2Pressed) && (alarmLatched || cabinCheckPending)) {
    clearAfterInspection(pirDetected);
  }

  if (ignitionOn != previousIgnitionOn) {
    Serial.print(F("Ignition changed to "));
    Serial.println(ignitionOn ? F("ON") : F("OFF"));
    previousIgnitionOn = ignitionOn;
    if (ignitionOn && cabinCheckPending) {
      cabinCheckPending = false;
      Serial.println(F("Cabin check cancelled; ignition is ON."));
      applyOutputs();
    }
  }

  // With passengers counted, or motion detected, turning off the vehicle
  // starts a 15-second rear-cabin confirmation window.
  if (!ignitionOn && !alarmLatched && !cabinCheckPending &&
      (passengerCount > 0 || (pirReady && pirDetected) || (aiPeople > 0 && now - aiUpdated < 15000))) {
    startCabinCheck(passengerCount > 0
                        ? F("passenger count is not zero")
                        : F("PIR motion while ignition is OFF"),
                    now);
  }

  if (cabinCheckPending && (now - cabinCheckStartedMs) >= CABIN_CHECK_MS) {
    raiseAlarm(F("cabin check timed out"));
  }

  // Motion arriving during or after the check also requires the alarm response.
  if (pirReady && !ignitionOn && pirDetected && !cabinCheckPending && !alarmLatched) {
    startCabinCheck(F("PIR motion while ignition is OFF"), now);
  }

  if (now - lastDisplayMs >= DISPLAY_REFRESH_MS) {
    drawStatus(ignitionOn, pirDetected, pirReady, now);
    lastDisplayMs = now;
  }
  if (now - lastSerialMs >= SERIAL_REFRESH_MS) {
    printStatus(ignitionOn, pirDetected, pirReady);
    uint32_t remaining = cabinCheckPending && now - cabinCheckStartedMs < CABIN_CHECK_MS ? (CABIN_CHECK_MS - (now - cabinCheckStartedMs) + 999) / 1000 : 0;
    TelemetrySnapshot sample = {ignitionOn, pirDetected, pirReady, alarmLatched, cabinCheckPending, alarmLatched || remoteFanOn, alarmLatched && !buzzerSilenced, passengerCount, inspectionSequence, totalBoarded, totalAlighted, remaining};
    if (telemetryQueue) xQueueOverwrite(telemetryQueue, &sample);
    lastSerialMs = now;
  }
  delay(1);
}
