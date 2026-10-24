#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>

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

struct DebouncedInput {
  bool rawActive;
  bool stableActive;
  uint32_t lastChangeMs;
};

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
  Serial.print(F("ALARM: "));
  Serial.println(reason);
  applyOutputs();
}

void clearAfterInspection(bool pirDetected) {
  if (pirDetected) {
    Serial.println(F("Button pressed, but PIR still detects motion; alarm remains."));
    return;
  }
  cabinCheckPending = false;
  alarmLatched = false;
  passengerCount = 0;
  doorSequence = DOOR_NONE;
  Serial.println(F("Cabin check confirmed. Passenger count reset to 0."));
  applyOutputs();
}

void changePassengerCount(int delta) {
  if (delta > 0) {
    if (passengerCount < 999) ++passengerCount;
    Serial.print(F("Passenger entered. Count="));
    Serial.println(passengerCount);
  } else if (passengerCount > 0) {
    --passengerCount;
    Serial.print(F("Passenger exited. Count="));
    Serial.println(passengerCount);
  } else {
    Serial.println(F("Exit detected while count is already 0; count unchanged."));
  }
}

void processDoorEvents(bool ir1Blocked, bool ir2Blocked, uint32_t now) {
  if (doorSequence != DOOR_NONE &&
      (now - doorSequenceStartedMs) > IR_SEQUENCE_TIMEOUT_MS) {
    doorSequence = DOOR_NONE;
  }

  // If both sensors trigger in the same loop, direction is ambiguous. Ignore it.
  if (ir1Blocked && ir2Blocked) {
    doorSequence = DOOR_NONE;
    Serial.println(F("Both door sensors triggered together; direction ignored."));
    return;
  }

  if (ir1Blocked) {
    if (doorSequence == IR2_FIRST) {
      changePassengerCount(-1);  // IR2 then IR1: exit
      doorSequence = DOOR_NONE;
    } else if (doorSequence == DOOR_NONE) {
      doorSequence = IR1_FIRST;
      doorSequenceStartedMs = now;
    }
  }

  if (ir2Blocked) {
    if (doorSequence == IR1_FIRST) {
      changePassengerCount(1);   // IR1 then IR2: enter
      doorSequence = DOOR_NONE;
    } else if (doorSequence == DOOR_NONE) {
      doorSequence = IR2_FIRST;
      doorSequenceStartedMs = now;
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
  pinMode(PIN_IGNITION, INPUT);  // Switch connects GPIO10 to 3V3 or GND.
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
  Serial.println(F("Firmware ready. Passenger count starts at 0."));
  Serial.println(F("Wait 30 seconds for HC-SR501 warm-up."));
}

void loop() {
  const uint32_t now = millis();
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
      (passengerCount > 0 || (pirReady && pirDetected))) {
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
    lastSerialMs = now;
  }
}
