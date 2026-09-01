#include <WiFi.h>
#include <WebServer.h>

// =====================================================
// Wi-Fi
// =====================================================
const char* ssid = "YOUR_HOTSPOT_NAME";
const char* password = "YOUR_HOTSPOT_PASSWORD";

// =====================================================
// ESP32-CAM AI Thinker flash LED
// GPIO 4 = onboard flash LED
// =====================================================
const int FLASH_LED_PIN = 4;

WebServer server(80);

int motorSpeed = 170;
int flashBrightness = 0;

// Last movement command
String lastMovement = "S";

// =====================================================
// Web control page
// =====================================================
const char MAIN_PAGE[] PROGMEM = R"rawliteral(
<!DOCTYPE html>
<html>
<head>
  <meta name="viewport" content="width=device-width, initial-scale=1">
  <title>Smart Agriculture Robot</title>

  <style>
    * {
      box-sizing: border-box;
    }

    body {
      margin: 0;
      padding: 15px;
      background: #061944;
      color: white;
      font-family: Arial, sans-serif;
      text-align: center;
    }

    .container {
      max-width: 430px;
      margin: auto;
    }

    h2 {
      color: #00ff66;
      margin: 8px 0 15px;
    }

    h3 {
      margin-top: 0;
    }

    .card {
      background: #09265d;
      margin: 14px 0;
      padding: 18px;
      border-radius: 14px;
      box-shadow: 0 4px 12px rgba(0, 0, 0, 0.3);
    }

    .row {
      display: flex;
      justify-content: center;
      align-items: center;
      flex-wrap: wrap;
    }

    button {
      min-width: 90px;
      min-height: 54px;
      margin: 5px;
      padding: 12px 14px;
      border: none;
      border-radius: 8px;
      color: white;
      font-size: 15px;
      font-weight: bold;
      cursor: pointer;
    }

    button:active {
      transform: scale(0.94);
    }

    .forward {
      background: #1597e5;
    }

    .back {
      background: #ba12df;
    }

    .left,
    .right {
      background: #f29b18;
    }

    .stop {
      background: #f21e4b;
    }

    .pump {
      background: #168b55;
    }

    .auto {
      background: #7f42c7;
    }

    label {
      display: block;
      margin-top: 14px;
      margin-bottom: 6px;
      text-align: left;
      font-weight: bold;
      color: #e7efff;
    }

    input[type="range"] {
      width: 100%;
      height: 8px;
      accent-color: #22a7ff;
    }

    .value {
      color: #00ff66;
      font-weight: bold;
    }

    .note {
      color: #c7d4ed;
      font-size: 13px;
      line-height: 1.4;
    }
  </style>
</head>

<body>
  <div class="container">
    <h2>Smart Agriculture Robot</h2>

    <div class="card">
      <h3>Movement</h3>

      <div class="row">
        <button class="forward" onclick="moveRobot('F')">
          FORWARD
        </button>
      </div>

      <div class="row">
        <button class="left" onclick="moveRobot('L')">
          LEFT
        </button>

        <button class="stop" onclick="moveRobot('S')">
          STOP
        </button>

        <button class="right" onclick="moveRobot('R')">
          RIGHT
        </button>
      </div>

      <div class="row">
        <button class="back" onclick="moveRobot('B')">
          BACK
        </button>
      </div>
    </div>

    <div class="card">
      <label for="speedSlider">
        Motor Speed:
        <span class="value" id="speedValue">170</span>
      </label>

      <input
        id="speedSlider"
        type="range"
        min="0"
        max="255"
        value="170"
        oninput="changeSpeed(this.value)"
      >

      <label for="flashSlider">
        Flash LED:
        <span class="value" id="flashValue">0</span>
      </label>

      <input
        id="flashSlider"
        type="range"
        min="0"
        max="255"
        value="0"
        oninput="changeFlash(this.value)"
      >
    </div>

    <div class="card">
      <h3>Robot Controls</h3>

      <div class="row">
        <button class="pump" onclick="sendCommand('PUMP_ON')">
          PUMP ON
        </button>

        <button class="stop" onclick="sendCommand('PUMP_OFF')">
          PUMP OFF
        </button>
      </div>

      <div class="row">
        <button class="auto" onclick="sendCommand('AUTO')">
          AUTO MODE
        </button>

        <button class="stop" onclick="sendCommand('MANUAL')">
          MANUAL MODE
        </button>
      </div>

      <p class="note">
        Reverse karte waqt object detect hone par buzzer aur fake disease LED blink karegi.
      </p>
    </div>
  </div>

  <script>
    function moveRobot(command) {
      fetch('/move?command=' + command)
        .then(response => response.text())
        .then(data => console.log(data))
        .catch(error => console.log(error));
    }

    function sendCommand(command) {
      fetch('/command?value=' + command)
        .then(response => response.text())
        .then(data => console.log(data))
        .catch(error => console.log(error));
    }

    function changeSpeed(value) {
      document.getElementById('speedValue').innerText = value;

      fetch('/speed?value=' + value)
        .then(response => response.text())
        .then(data => console.log(data))
        .catch(error => console.log(error));
    }

    function changeFlash(value) {
      document.getElementById('flashValue').innerText = value;

      fetch('/flash?value=' + value)
        .then(response => response.text())
        .then(data => console.log(data))
        .catch(error => console.log(error));
    }
  </script>
</body>
</html>
)rawliteral";

// =====================================================
// Send command to Arduino Uno
// =====================================================
void sendToArduino(String command) {
  Serial.println(command);
}

// =====================================================
// Home page
// =====================================================
void handleRoot() {
  server.send(200, "text/html", MAIN_PAGE);
}

// =====================================================
// Movement handler
// =====================================================
void handleMove() {
  if (!server.hasArg("command")) {
    server.send(400, "text/plain", "Command missing");
    return;
  }

  String command = server.arg("command");

  if (command == "F" ||
      command == "B" ||
      command == "L" ||
      command == "R" ||
      command == "S") {

    lastMovement = command;
    sendToArduino(command);

    server.send(200, "text/plain", "Movement command sent: " + command);
  } else {
    server.send(400, "text/plain", "Invalid movement command");
  }
}

// =====================================================
// Pump, auto/manual, buzzer and light commands
// =====================================================
void handleCommand() {
  if (!server.hasArg("value")) {
    server.send(400, "text/plain", "Command missing");
    return;
  }

  String command = server.arg("value");

  if (command == "PUMP_ON" ||
      command == "PUMP_OFF" ||
      command == "AUTO" ||
      command == "MANUAL" ||
      command == "BUZZER_ON" ||
      command == "BUZZER_OFF" ||
      command == "LIGHT_ON" ||
      command == "LIGHT_OFF") {

    sendToArduino(command);
    server.send(200, "text/plain", "Command sent: " + command);
  } else {
    server.send(400, "text/plain", "Invalid command");
  }
}

// =====================================================
// Motor speed slider
// =====================================================
void handleSpeed() {
  if (!server.hasArg("value")) {
    server.send(400, "text/plain", "Speed value missing");
    return;
  }

  motorSpeed = server.arg("value").toInt();
  motorSpeed = constrain(motorSpeed, 0, 255);

  // Send speed to Arduino Uno
  sendToArduino("SPEED:" + String(motorSpeed));

  server.send(
    200,
    "text/plain",
    "Motor speed set to " + String(motorSpeed)
  );
}

// =====================================================
// Flash LED slider
// =====================================================
void handleFlash() {
  if (!server.hasArg("value")) {
    server.send(400, "text/plain", "Flash value missing");
    return;
  }

  flashBrightness = server.arg("value").toInt();
  flashBrightness = constrain(flashBrightness, 0, 255);

  // PWM brightness control for GPIO 4 flash LED
  analogWrite(FLASH_LED_PIN, flashBrightness);

  server.send(
    200,
    "text/plain",
    "Flash brightness set to " + String(flashBrightness)
  );
}

// =====================================================
// Setup
// =====================================================
void setup() {
  pinMode(FLASH_LED_PIN, OUTPUT);
  analogWrite(FLASH_LED_PIN, 0);

  // Serial sends commands to Arduino Uno
  Serial.begin(115200);

  delay(1000);

  WiFi.mode(WIFI_STA);
  WiFi.begin(ssid, password);

  // Do not print debug text through Serial,
  // because Serial is being used for Arduino commands.
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
  }

  server.on("/", handleRoot);
  server.on("/move", handleMove);
  server.on("/command", handleCommand);
  server.on("/speed", handleSpeed);
  server.on("/flash", handleFlash);

  server.begin();
}

// =====================================================
// Main loop
// =====================================================
void loop() {
  server.handleClient();
}