// =====================================================
// AgroRover X - Arduino Uno Complete Code
// =====================================================
//
// Controls:
// 1. L298N motor driver
// 2. IR obstacle sensor
// 3. Fake plant-disease LED
// 4. Buzzer
// 5. Soil moisture sensor
// 6. Water pump relay
// 7. Optional light
//
// Commands received from ESP32-CAM:
// F, B, L, R, S
// SPEED:170
// PUMP_ON, PUMP_OFF
// AUTO, MANUAL
// BUZZER_ON, BUZZER_OFF
// LIGHT_ON, LIGHT_OFF
//
// Important:
// Fake disease LED is only a presentation/demo feature.
// It does not actually detect plant disease using AI/ML.
// =====================================================


// =====================================================
// L298N motor driver pins
// =====================================================
const byte ENA = 5;
const byte IN1 = 6;
const byte IN2 = 7;

const byte IN3 = 8;
const byte IN4 = 9;
const byte ENB = 10;


// =====================================================
// Sensor and output pins
// =====================================================
const byte IR_PIN = 2;

// Fake disease indication LED
const byte FAKE_DISEASE_LED = 3;

const byte BUZZER_PIN = 11;
const byte PUMP_RELAY_PIN = 12;
const byte LIGHT_PIN = 13;

const byte SOIL_PIN = A0;


// =====================================================
// Configuration
// =====================================================
int motorSpeed = 170;

// Change this after checking your soil sensor readings
int dryThreshold = 700;

// Most IR obstacle modules output LOW when object is detected
const byte OBJECT_DETECTED = LOW;

// Most relay modules are active LOW
const byte RELAY_ON = LOW;
const byte RELAY_OFF = HIGH;


// =====================================================
// Robot state
// =====================================================
bool autoMode = false;
bool pumpIsOn = false;

char currentMovement = 'S';

String serialCommand = "";


// =====================================================
// Timing variables
// =====================================================
unsigned long lastIRCheck = 0;
unsigned long lastAutoCheck = 0;
unsigned long lastAlertToggle = 0;
unsigned long wateringStartTime = 0;

const unsigned long IR_CHECK_INTERVAL = 40;
const unsigned long AUTO_CHECK_INTERVAL = 250;
const unsigned long ALERT_INTERVAL = 220;

const unsigned long WATERING_TIME = 2000;


// =====================================================
// Alert state
// =====================================================
bool alertState = false;


// =====================================================
// Auto mode state
// =====================================================
bool objectAlreadyHandled = false;
bool wateringNow = false;


// =====================================================
// Setup
// =====================================================
void setup() {
  // Motor driver pins
  pinMode(ENA, OUTPUT);
  pinMode(IN1, OUTPUT);
  pinMode(IN2, OUTPUT);

  pinMode(IN3, OUTPUT);
  pinMode(IN4, OUTPUT);
  pinMode(ENB, OUTPUT);

  // Sensor
  pinMode(IR_PIN, INPUT);
  pinMode(SOIL_PIN, INPUT);

  // Outputs
  pinMode(FAKE_DISEASE_LED, OUTPUT);
  pinMode(BUZZER_PIN, OUTPUT);
  pinMode(PUMP_RELAY_PIN, OUTPUT);
  pinMode(LIGHT_PIN, OUTPUT);

  // Safe initial state
  stopMotors();
  pumpOff();
  buzzerOff();
  fakeDiseaseLedOff();
  digitalWrite(LIGHT_PIN, LOW);

  // Must match ESP32 Serial speed
  Serial.begin(115200);

  delay(500);
}


// =====================================================
// Main loop
// =====================================================
void loop() {
  readESP32Command();

  checkReverseObstacleAlert();

  if (autoMode) {
    runAutoMode();
  }
}


// =====================================================
// Receive commands from ESP32-CAM
// =====================================================
void readESP32Command() {
  while (Serial.available() > 0) {
    char receivedChar = Serial.read();

    if (receivedChar == '\n') {
      serialCommand.trim();

      if (serialCommand.length() > 0) {
        executeCommand(serialCommand);
      }

      serialCommand = "";
    }
    else if (receivedChar != '\r') {
      serialCommand += receivedChar;
    }
  }
}


// =====================================================
// Execute ESP32 commands
// =====================================================
void executeCommand(String command) {
  command.trim();

  // ---------------- Movement ----------------
  if (command == "F") {
    autoMode = false;
    stopReverseAlert();

    currentMovement = 'F';
    forward();
  }

  else if (command == "B") {
    autoMode = false;

    currentMovement = 'B';
    backward();
  }

  else if (command == "L") {
    autoMode = false;
    stopReverseAlert();

    currentMovement = 'L';
    turnLeft();
  }

  else if (command == "R") {
    autoMode = false;
    stopReverseAlert();

    currentMovement = 'R';
    turnRight();
  }

  else if (command == "S") {
    autoMode = false;

    currentMovement = 'S';
    stopMotors();
    stopReverseAlert();
  }

  // ---------------- Motor speed ----------------
  else if (command.startsWith("SPEED:")) {
    int newSpeed = command.substring(6).toInt();

    motorSpeed = constrain(newSpeed, 0, 255);

    // Apply new speed to current movement
    applyCurrentMovement();
  }

  // ---------------- Pump ----------------
  else if (command == "PUMP_ON") {
    autoMode = false;
    pumpOn();
  }

  else if (command == "PUMP_OFF") {
    autoMode = false;
    pumpOff();
  }

  // ---------------- Modes ----------------
  else if (command == "AUTO") {
    autoMode = true;

    currentMovement = 'S';
    stopMotors();

    objectAlreadyHandled = false;
    wateringNow = false;
    pumpOff();
  }

  else if (command == "MANUAL") {
    autoMode = false;

    currentMovement = 'S';
    stopMotors();
    pumpOff();
    stopReverseAlert();
  }

  // ---------------- Buzzer ----------------
  else if (command == "BUZZER_ON") {
    buzzerOn();
  }

  else if (command == "BUZZER_OFF") {
    buzzerOff();
  }

  // ---------------- Light ----------------
  else if (command == "LIGHT_ON") {
    digitalWrite(LIGHT_PIN, HIGH);
  }

  else if (command == "LIGHT_OFF") {
    digitalWrite(LIGHT_PIN, LOW);
  }
}


// =====================================================
// Apply current movement after speed change
// =====================================================
void applyCurrentMovement() {
  if (currentMovement == 'F') {
    forward();
  }
  else if (currentMovement == 'B') {
    backward();
  }
  else if (currentMovement == 'L') {
    turnLeft();
  }
  else if (currentMovement == 'R') {
    turnRight();
  }
  else {
    stopMotors();
  }
}


// =====================================================
// Motor control functions
// =====================================================
void forward() {
  analogWrite(ENA, motorSpeed);
  analogWrite(ENB, motorSpeed);

  digitalWrite(IN1, HIGH);
  digitalWrite(IN2, LOW);

  digitalWrite(IN3, HIGH);
  digitalWrite(IN4, LOW);
}


void backward() {
  analogWrite(ENA, motorSpeed);
  analogWrite(ENB, motorSpeed);

  digitalWrite(IN1, LOW);
  digitalWrite(IN2, HIGH);

  digitalWrite(IN3, LOW);
  digitalWrite(IN4, HIGH);
}


void turnLeft() {
  analogWrite(ENA, motorSpeed);
  analogWrite(ENB, motorSpeed);

  digitalWrite(IN1, LOW);
  digitalWrite(IN2, HIGH);

  digitalWrite(IN3, HIGH);
  digitalWrite(IN4, LOW);
}


void turnRight() {
  analogWrite(ENA, motorSpeed);
  analogWrite(ENB, motorSpeed);

  digitalWrite(IN1, HIGH);
  digitalWrite(IN2, LOW);

  digitalWrite(IN3, LOW);
  digitalWrite(IN4, HIGH);
}


void stopMotors() {
  analogWrite(ENA, 0);
  analogWrite(ENB, 0);

  digitalWrite(IN1, LOW);
  digitalWrite(IN2, LOW);

  digitalWrite(IN3, LOW);
  digitalWrite(IN4, LOW);
}


// =====================================================
// Reverse obstacle alert
// =====================================================
//
// When:
// - Robot is moving backward
// - IR sensor detects an object
//
// Then:
// - Buzzer beeps
// - Fake disease LED blinks
//
// This is only a demo indication.
// =====================================================
void checkReverseObstacleAlert() {
  unsigned long currentTime = millis();

  if (currentTime - lastIRCheck < IR_CHECK_INTERVAL) {
    return;
  }

  lastIRCheck = currentTime;

  bool objectDetected =
    digitalRead(IR_PIN) == OBJECT_DETECTED;

  if (currentMovement == 'B' && objectDetected) {
    startReverseAlert();
  }
  else {
    stopReverseAlert();
  }
}


void startReverseAlert() {
  unsigned long currentTime = millis();

  if (currentTime - lastAlertToggle >= ALERT_INTERVAL) {
    lastAlertToggle = currentTime;

    alertState = !alertState;

    digitalWrite(BUZZER_PIN, alertState);
    digitalWrite(FAKE_DISEASE_LED, alertState);
  }
}


void stopReverseAlert() {
  alertState = false;

  digitalWrite(BUZZER_PIN, LOW);
  digitalWrite(FAKE_DISEASE_LED, LOW);
}


// =====================================================
// Automatic mode
// =====================================================
void runAutoMode() {
  unsigned long currentTime = millis();

  if (currentTime - lastAutoCheck < AUTO_CHECK_INTERVAL) {
    return;
  }

  lastAutoCheck = currentTime;

  bool objectDetected =
    digitalRead(IR_PIN) == OBJECT_DETECTED;

  // No object: keep moving forward
  if (!objectDetected) {
    objectAlreadyHandled = false;

    if (!wateringNow) {
      currentMovement = 'F';
      forward();
    }

    return;
  }

  // Object detected: stop
  currentMovement = 'S';
  stopMotors();

  // Fake alert in auto mode
  startAutoAlert();

  // Read soil sensor once for this object
  if (!objectAlreadyHandled && !wateringNow) {
    objectAlreadyHandled = true;

    int soilValue = analogRead(SOIL_PIN);

    // For this sensor setup, high value means dry
    if (soilValue > dryThreshold) {
      wateringNow = true;
      wateringStartTime = currentTime;
      pumpOn();
    }
  }

  // Stop pump after watering time
  if (wateringNow &&
      currentTime - wateringStartTime >= WATERING_TIME) {

    wateringNow = false;
    pumpOff();
  }
}


void startAutoAlert() {
  unsigned long currentTime = millis();

  if (currentTime - lastAlertToggle >= ALERT_INTERVAL) {
    lastAlertToggle = currentTime;

    alertState = !alertState;

    digitalWrite(BUZZER_PIN, alertState);
    digitalWrite(FAKE_DISEASE_LED, alertState);
  }
}


// =====================================================
// Pump functions
// =====================================================
void pumpOn() {
  digitalWrite(PUMP_RELAY_PIN, RELAY_ON);
  pumpIsOn = true;
}


void pumpOff() {
  digitalWrite(PUMP_RELAY_PIN, RELAY_OFF);
  pumpIsOn = false;
}


// =====================================================
// Buzzer functions
// =====================================================
void buzzerOn() {
  digitalWrite(BUZZER_PIN, HIGH);
}


void buzzerOff() {
  digitalWrite(BUZZER_PIN, LOW);
}


// =====================================================
// Fake disease LED functions
// =====================================================
void fakeDiseaseLedOff() {
  digitalWrite(FAKE_DISEASE_LED, LOW);
}