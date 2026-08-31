#include <Arduino.h>
#include <Wire.h>

// Compact, buildable wheel-control firmware extracted from the rover firmware.
// The full production image additionally contains encoder balancing and BNO08x
// telemetry. Pin assignments below are the verified assignments on the rover.

static constexpr int RIGHT_I2C_SDA_PIN = 8;
static constexpr int RIGHT_I2C_SCL_PIN = 9;
static constexpr int LEFT_I2C_SDA_PIN = 41;
static constexpr int LEFT_I2C_SCL_PIN = 42;
static constexpr uint8_t PCF8591_ADDRESS = 0x48;
static constexpr uint8_t PCF8591_DAC_ENABLE = 0x40;

static constexpr int LEFT_REVERSE_RELAY_PIN = 21;
static constexpr int RIGHT_REVERSE_RELAY_PIN = 11;
static constexpr uint32_t I2C_SPEED_HZ = 100000;
static constexpr uint32_t COMMAND_TIMEOUT_MS = 2000;
static constexpr uint32_t RAMP_INTERVAL_MS = 10;
static constexpr uint32_t RAMP_TO_ZERO_TIMEOUT_MS = 1500;
static constexpr uint8_t RAMP_STEP = 5;
static constexpr uint8_t DEFAULT_MAX_RAW = 96;
static constexpr uint8_t MAX_APPROVED_RAW = 193;

TwoWire rightI2c = TwoWire(0);
TwoWire leftI2c = TwoWire(1);

struct DacChannel {
  const char *name;
  TwoWire *bus;
  uint8_t current;
  uint8_t target;
  bool present;
};

DacChannel leftDac = {"left", &leftI2c, 0, 0, false};
DacChannel rightDac = {"right", &rightI2c, 0, 0, false};

bool armed = false;
bool faulted = false;
bool leftReverse = false;
bool rightReverse = false;
uint8_t maxRaw = DEFAULT_MAX_RAW;
uint32_t lastCommandMs = 0;
uint32_t lastRampMs = 0;
String inputLine;

void applyRelays() {
  digitalWrite(LEFT_REVERSE_RELAY_PIN, leftReverse ? HIGH : LOW);
  digitalWrite(RIGHT_REVERSE_RELAY_PIN, rightReverse ? HIGH : LOW);
}

bool writeDac(DacChannel &dac, uint8_t value) {
  dac.bus->beginTransmission(PCF8591_ADDRESS);
  dac.bus->write(PCF8591_DAC_ENABLE);
  dac.bus->write(value);

  uint8_t result = dac.bus->endTransmission();

  if (result != 0) {
    Serial.printf("I2C error: %s DAC, code=%u\n", dac.name, result);
    return false;
  }

  dac.current = value;
  return true;
}

bool probeDac(DacChannel &dac) {
  dac.bus->beginTransmission(PCF8591_ADDRESS);
  dac.present = dac.bus->endTransmission() == 0;
  return dac.present;
}

void forceStop(const char *reason) {
  armed = false;
  faulted = true;

  leftDac.target = 0;
  rightDac.target = 0;

  leftReverse = false;
  rightReverse = false;

  applyRelays();

  writeDac(leftDac, 0);
  writeDac(rightDac, 0);

  Serial.printf("STOP: %s\n", reason);
}

uint8_t clippedRaw(int value) {
  if (value <= 0) {
    return 0;
  }

  return static_cast<uint8_t>(
      min(value, static_cast<int>(maxRaw))
  );
}

bool rampOne(DacChannel &dac) {
  if (dac.current == dac.target) {
    return true;
  }

  uint8_t next = dac.current;

  if (dac.current < dac.target) {
    next = min(
        static_cast<int>(dac.target),
        static_cast<int>(dac.current) + static_cast<int>(RAMP_STEP)
    );
  } else {
    next = max(
        static_cast<int>(dac.target),
        static_cast<int>(dac.current) - static_cast<int>(RAMP_STEP)
    );
  }

  return writeDac(dac, next);
}

void rampUpdate() {
  uint32_t now = millis();

  if (now - lastRampMs < RAMP_INTERVAL_MS) {
    return;
  }

  lastRampMs = now;

  if (!rampOne(leftDac) || !rampOne(rightDac)) {
    forceStop("DAC ramp write failed");
  }
}

bool rampToZero() {
  leftDac.target = 0;
  rightDac.target = 0;

  uint32_t started = millis();

  while (leftDac.current != 0 || rightDac.current != 0) {
    rampUpdate();

    if (faulted || millis() - started > RAMP_TO_ZERO_TIMEOUT_MS) {
      forceStop("could not confirm DAC zero");
      return false;
    }

    delay(1);
  }

  return true;
}

void safeDisarm(const char *reason) {
  armed = false;

  leftDac.target = 0;
  rightDac.target = 0;

  bool leftZero = writeDac(leftDac, 0);
  bool rightZero = writeDac(rightDac, 0);

  leftReverse = false;
  rightReverse = false;

  applyRelays();

  if (!leftZero || !rightZero) {
    faulted = true;
    Serial.println("STOP: DAC zero write failed during disarm");
  }

  Serial.printf("Disarmed: %s\n", reason);
}

bool parseInt(const String &token, int &value) {
  if (token.length() == 0) {
    return false;
  }

  char *end = nullptr;
  long parsed = strtol(token.c_str(), &end, 10);

  if (*end != '\0') {
    return false;
  }

  value = static_cast<int>(parsed);
  return true;
}

String tokenAt(const String &line, int wanted) {
  int index = 0;
  int start = -1;

  for (int i = 0; i <= line.length(); ++i) {
    bool separator =
        i == line.length() ||
        isspace(static_cast<unsigned char>(line[i]));

    if (!separator && start < 0) {
      start = i;
    }

    if (separator && start >= 0) {
      if (index == wanted) {
        return line.substring(start, i);
      }

      ++index;
      start = -1;
    }
  }

  return "";
}

void printStatus() {
  probeDac(leftDac);
  probeDac(rightDac);

  Serial.printf(
      "STATUS armed=%s faulted=%s max=%u "
      "left=%u/%u relay=%s present=%s "
      "right=%u/%u relay=%s present=%s\n",
      armed ? "yes" : "no",
      faulted ? "yes" : "no",
      maxRaw,
      leftDac.current,
      leftDac.target,
      leftReverse ? "reverse" : "forward",
      leftDac.present ? "yes" : "no",
      rightDac.current,
      rightDac.target,
      rightReverse ? "reverse" : "forward",
      rightDac.present ? "yes" : "no"
  );
}

void setRelayCommand(const String &side, int value) {
  if (value != 0 && value != 1) {
    Serial.println("Usage: relay <left|right|both> <0|1>");
    return;
  }

  if (value == 1 && faulted) {
    Serial.println("Rejected: clear fault before enabling reverse");
    return;
  }

  armed = false;

  if (!rampToZero()) {
    return;
  }

  if (side == "left" || side == "both") {
    leftReverse = value == 1;
  }

  if (side == "right" || side == "both") {
    rightReverse = value == 1;
  }

  applyRelays();

  Serial.printf(
      "Relays left=%s right=%s\n",
      leftReverse ? "reverse" : "forward",
      rightReverse ? "reverse" : "forward"
  );
}

void printHelp() {
  Serial.println("Commands:");
  Serial.println("  status");
  Serial.println("  arm | disarm | zero | clearfault");
  Serial.println("  max <0..193>");
  Serial.println("  drive <left 0..193> <right 0..193>");
  Serial.println("  relay <left|right|both> <0|1>");
}

void handleCommand(String line) {
  line.trim();
  line.toLowerCase();

  String command = tokenAt(line, 0);

  if (command.length() == 0) {
    return;
  }

  if (command == "help" || command == "?") {
    printHelp();
    return;
  }

  if (command == "status") {
    printStatus();
    return;
  }

  if (command == "zero") {
    leftDac.target = 0;
    rightDac.target = 0;

    lastCommandMs = millis();

    Serial.println("Zero requested");
    return;
  }

  if (command == "disarm") {
    safeDisarm("operator command");
    return;
  }

  if (command == "clearfault") {
    if (
        armed ||
        leftDac.current != 0 ||
        rightDac.current != 0 ||
        leftReverse ||
        rightReverse
    ) {
      Serial.println("Rejected: clearfault requires safe zero");
      return;
    }

    if (
        !probeDac(leftDac) ||
        !probeDac(rightDac) ||
        !writeDac(leftDac, 0) ||
        !writeDac(rightDac, 0)
    ) {
      Serial.println("Rejected: both DAC modules must respond");
      return;
    }

    faulted = false;

    Serial.println("Fault cleared at safe zero");
    return;
  }

  if (command == "arm") {
    if (faulted) {
      Serial.println("Rejected: clear fault before arm");
      return;
    }

    if (
        !probeDac(leftDac) ||
        !probeDac(rightDac) ||
        !writeDac(leftDac, 0) ||
        !writeDac(rightDac, 0)
    ) {
      forceStop("arm requires both DAC modules at zero");
      return;
    }

    leftDac.target = 0;
    rightDac.target = 0;

    armed = true;
    lastCommandMs = millis();

    Serial.println("Armed at zero throttle");
    return;
  }

  if (command == "max") {
    int value = 0;

    if (
        !parseInt(tokenAt(line, 1), value) ||
        value < 0 ||
        value > MAX_APPROVED_RAW
    ) {
      Serial.println("Usage: max <0..193>");
      return;
    }

    maxRaw = static_cast<uint8_t>(value);

    leftDac.target = min(leftDac.target, maxRaw);
    rightDac.target = min(rightDac.target, maxRaw);

    Serial.printf("Max raw set to %u\n", maxRaw);
    return;
  }

  if (command == "drive") {
    int left = 0;
    int right = 0;

    if (
        !parseInt(tokenAt(line, 1), left) ||
        !parseInt(tokenAt(line, 2), right) ||
        left < 0 ||
        right < 0 ||
        left > MAX_APPROVED_RAW ||
        right > MAX_APPROVED_RAW
    ) {
      Serial.println("Usage: drive <left 0..193> <right 0..193>");
      return;
    }

    if (!armed && (left > 0 || right > 0)) {
      Serial.println("Rejected: throttle requires arm");
      return;
    }

    leftDac.target = clippedRaw(left);
    rightDac.target = clippedRaw(right);

    lastCommandMs = millis();

    Serial.printf(
        "Targets left=%u right=%u\n",
        leftDac.target,
        rightDac.target
    );

    return;
  }

  if (command == "relay") {
    String side = tokenAt(line, 1);
    int value = 0;

    if (
        (side != "left" &&
         side != "right" &&
         side != "both") ||
        !parseInt(tokenAt(line, 2), value)
    ) {
      Serial.println("Usage: relay <left|right|both> <0|1>");
      return;
    }

    setRelayCommand(side, value);
    return;
  }

  Serial.println("Unknown command. Type help.");
}

void readSerial() {
  while (Serial.available() > 0) {
    char value = static_cast<char>(Serial.read());

    if (value == '\r') {
      continue;
    }

    if (value == '\n') {
      handleCommand(inputLine);
      inputLine = "";
    } else if (inputLine.length() < 120) {
      inputLine += value;
    }
  }
}

void setup() {
  leftReverse = false;
  rightReverse = false;

  digitalWrite(LEFT_REVERSE_RELAY_PIN, LOW);
  digitalWrite(RIGHT_REVERSE_RELAY_PIN, LOW);

  pinMode(LEFT_REVERSE_RELAY_PIN, OUTPUT);
  pinMode(RIGHT_REVERSE_RELAY_PIN, OUTPUT);

  applyRelays();

  Serial.begin(115200);
  delay(800);

  rightI2c.begin(
      RIGHT_I2C_SDA_PIN,
      RIGHT_I2C_SCL_PIN,
      I2C_SPEED_HZ
  );

  leftI2c.begin(
      LEFT_I2C_SDA_PIN,
      LEFT_I2C_SCL_PIN,
      I2C_SPEED_HZ
  );

  if (
      !probeDac(leftDac) ||
      !probeDac(rightDac) ||
      !writeDac(leftDac, 0) ||
      !writeDac(rightDac, 0)
  ) {
    forceStop("safe boot failed");
  }

  Serial.println(
      "ESP32-S3 rover wheel demo: zero throttle, disarmed"
  );

  printStatus();
  printHelp();
}

void loop() {
  readSerial();
  rampUpdate();

  if (
      armed &&
      millis() - lastCommandMs > COMMAND_TIMEOUT_MS
  ) {
    safeDisarm("command watchdog timeout");
  }
}
