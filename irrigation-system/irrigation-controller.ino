#include <ArduinoMqttClient.h>
#include <SPI.h>
#include <WiFiNINA.h>
#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>
#include <Fonts/FreeMonoBold24pt7b.h>

//OLED Screen configuration
#define SCREEN_WIDTH 128 // OLED display width, in pixels
#define SCREEN_HEIGHT 64 // OLED display height, in pixels
Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, -1);

//Wifi credentials
#include "arduino_secrets.h"  // Set your Wifi credentials in the separate file
char ssid[] = SECRET_SSID;    // your network SSID (name)
char pass[] = SECRET_PASS;    // your network password (use for WPA, or use as key for WEP)

//Valve configuration
int mainValvePin = 3;         // I/O PIN to control the main valve
int pins[] = {9, 8, 7, 6, 5, 4}; //Arduino PIN, index is valve

//Reduced output at the back configuration
float intervals[] = {0.16, 0.08, 0.08, 0.04, 0.15, 0.49}; //fraction of active duration, index is step, total should be 1
int steps[] = {2, 4, 6 + 0x30, 6, 1, 5};                  //Order of valves in auto mode (0x0f only, 0xf0 can be used to run two sprinkler groups simultaneously)
int nSteps = 6;                                           //Steps for irrigation program

//Button pins
int pinButtonDecrease = A1;
int pinButtonIncrease = A7;

//MQTT Configuration
const char broker[] = "192.168.2.5"; //MQTT Broker to connect to
int        port     = 1883;          //Port of the broker
const char sprinklerTopic[]  = "watercontrol/setSprinkler";
const char autoTopic[]  = "watercontrol/setAuto";
const char stateTopic[]  = "watercontrol/setState";
const char updateSprinklerTopic[]  = "watercontrol/updateSprinkler";
const char updateAutoTopic[]  = "watercontrol/updateAuto";
const char updateStateTopic[]  = "watercontrol/updateState";
const char updateAutoMinTopic[] = "watercontrol/updateAutoMin";
const char updateAutoMinStepTopic[] = "watercontrol/updateAutoMinStep";
const char updateAutoCyclesTopic[] = "watercontrol/updateAutoCycles";
const char updateAutoCycleTopic[] = "watercontrol/updateAutoCycle";
const char notifyDoneTopic[] = "watercontrol/notifyDone";

//Garage door opener, see https://there.oughta.be/a/smart-garage-door
const char triggerGarageDoorTopic[] = "garage/triggerGarageDoor";
int garageDoorPin = 2;

//End of configuration

bool screenOff = false;

int status = WL_IDLE_STATUS;
long rssi = 0;

int cycles = 0;
int icycle = 0;

WiFiClient wifiClient;
MqttClient mqttClient(wifiClient);

int mqttStatus = 0;

void setup() {
  pinMode(mainValvePin, OUTPUT);
  digitalWrite(mainValvePin, HIGH);
  for (int i = 0; i < 6; i++) {
    pinMode(pins[i], OUTPUT);
    digitalWrite(pins[i], HIGH);
  }
  pinMode(garageDoorPin, OUTPUT);
  digitalWrite(garageDoorPin, HIGH);
  pinMode(pinButtonDecrease, INPUT_PULLUP);
  pinMode(pinButtonIncrease, INPUT_PULLUP);
  digitalWrite(pinButtonDecrease, INPUT_PULLUP);
  digitalWrite(pinButtonIncrease, INPUT_PULLUP);
  attachInterrupt(digitalPinToInterrupt(pinButtonDecrease), buttonDecreaseInterrupt, FALLING);
  attachInterrupt(digitalPinToInterrupt(pinButtonIncrease), buttonIncreaseInterrupt, FALLING);
  
  Serial.begin(115200);      // initialize serial communication

  if (!display.begin(SSD1306_SWITCHCAPVCC, 0x3C)) {
    Serial.println("Warning: Display not available!");
  }
  delay(2000);
  prepareDisplayCoordinates();
  
  mqttClient.onMessage(onMqttMessage);
  Serial.println("Water control");
  wakeIfAsleep();
}

bool sprinkler = false;
int autoDuration = 0;
int state = 0;
int remaining = 0;
unsigned long lastStep;
int autoStep = -1;

volatile bool buttonDecreasePressed = false;
volatile bool buttonIncreasePressed = false;
volatile unsigned long lastDecreasePress = 0;
volatile unsigned long lastIncreasePress = 0;

void process() {
  processButtonPress();
  checkNextStep();
  updateDisplay(false);
}

void loop() {
  ensureMqtt();
  mqttClient.poll();
  process();
}

void buttonDecreaseInterrupt() {
  unsigned long now = millis();
  if (now-lastDecreasePress > 200 && now-lastStep > 500) { //The comparison to lastStep filters mistriggers from power fluctuations when the relis are switched 
    for (int i = 0; i < 10; i++) {
      if (digitalRead(pinButtonDecrease) == HIGH)
        return;
      delay(10);
    }
    lastDecreasePress = now;
    buttonDecreasePressed = true;
  }
}

void buttonIncreaseInterrupt() {
  unsigned long now = millis();
  if (now-lastIncreasePress > 200 && now-lastStep > 500) {
    for (int i = 0; i < 10; i++) {
      if (digitalRead(pinButtonIncrease) == HIGH)
        return;
      delay(10);
    }
    lastIncreasePress = now;
    buttonIncreasePressed = true;
  }
}

void processButtonPress() {
  
  if (buttonIncreasePressed) {
    if (wakeIfAsleep()) {
      buttonIncreasePressed = false;
      return;
    }
    if (sprinkler) {
      if (autoDuration < 60) {
        setAutoDuration(60, true);
      } else if (autoDuration < 300) {
        setAutoDuration(autoDuration+30, true);
      }
    } else {
      if (autoDuration < 60) {
        setAutoDuration(60, true);
      } else if (autoDuration < 300) {
        setAutoDuration(autoDuration+60, true);
      } else {
        setSprinkler(true);
        setAutoDuration(60, true);
      }
    }
    buttonIncreasePressed = false;
  }
  
  if (buttonDecreasePressed) {
    if (wakeIfAsleep()) {
      buttonDecreasePressed = false;
      return;
    }
    if (sprinkler) {
      if (autoDuration > 60) {
        setAutoDuration(autoDuration-30, true);
      } else {
        setSprinkler(false);
        setState(0);
        setAutoDuration(300, true);
      }
    } else {
      if (autoDuration > 60) {
        setAutoDuration(autoDuration-60, true);
      } else {
        setAutoDuration(0, true);
      }
    }
    buttonDecreasePressed = false;
  }
}

String zeroPaddedNumber(int in) {
  if (in < 10)
    return "0"+String(in);
  else
    return String(in);
}

int displayRemaining = -1;
int displayState = -1;
int displayAutoDuration = -1;
int displayStatus = -1;
long displayRssi = -1;
int displayMqttStatus = -1;
bool displaySprinkler = true;

int centerX1, centerXM, centerX2, centerY, centerYM;

void prepareDisplayCoordinates() {
  int16_t x1, y1;
  uint16_t w, h;
  display.setFont(&FreeMonoBold24pt7b);
  display.setTextSize(1);
  display.getTextBounds(":", 0, 50, &x1, &y1, &w, &h);
  centerXM = (128-w)/2-x1;
  centerYM = (64-h)/2-(y1-50);
  display.getTextBounds("00", 0, 50, &x1, &y1, &w, &h);
  centerX1 = 63-x1-w-8;
  centerX2 = 64-x1+8;
  centerY = (64-h)/2-(y1-50);
}

String translateIndex(int index) {
  switch (index) {
    case 0: return "Regner aus";
            break;
    case 1: return "Zaun Strasse";
            break;
    case 2: return "Hinten Ecke";
            break;
    case 3: return "Kiesweg";
            break;
    case 4: return "Durchgang";
            break;
    case 5: return "Terasse Mitte";
            break;
    case 6: return "Terasse Ecke";
            break;
  }
}

long lastActivity = 0;

bool wakeIfAsleep() {
  lastActivity = millis();
  if (screenOff) {
    screenOff = false;
    updateDisplay(true);
    return true;
  }
  return false;
}

void updateDisplay(bool force) {
  if (screenOff)
    return;
  if (autoDuration == 0 && state == 0 && millis() - lastActivity > 60000) {
    screenOff = true;
    display.clearDisplay();
    display.display(); 
    return;
  }
  
  if (!force && displayRemaining == remaining && displayState == state && displayAutoDuration == autoDuration && displayStatus == status && displayRssi == rssi && displayMqttStatus == mqttStatus && displaySprinkler == sprinkler)
    return;
  display.clearDisplay();
  
  display.setFont(&FreeMonoBold24pt7b);
  display.setTextSize(1);
  display.setTextColor(WHITE);
  display.setCursor(centerXM, centerYM-4);
  display.println(":");
  display.setCursor(centerX1, centerY-4);
  if (autoDuration > 0)
    display.println(zeroPaddedNumber(remaining / 60));
  else
    display.println("--");
  display.setCursor(centerX2, centerY-4);
  if (autoDuration > 0)
    display.println(zeroPaddedNumber(remaining % 60));
  else
    display.println("--");
  displayRemaining = remaining;

  display.setFont();
  display.setTextSize(1);
  display.setTextColor(WHITE);

  display.setCursor(0, 0);
  if (autoDuration > 0)
    display.println("Gesamt: "+ zeroPaddedNumber(autoDuration / 60) + ":" + zeroPaddedNumber(autoDuration % 60));
  else
    display.println("Automatik aus");
  displayAutoDuration = autoDuration;
  
  if (status == WL_CONNECTED) {
    int bars;
    if (rssi <= -100) {
        bars = 0;
    } else if (rssi >= -55) {
        bars = 4;
    } else {
        bars = (int)((float)(100 + rssi) * 4. / 45.);
    }
    for (int i = 0; i < bars; i++)
      display.drawFastVLine(121+2*i, 6-2*i, 2*i+2, WHITE);
  } else {
    display.setCursor(122, 0);
    display.println("X");
  }
  displayStatus = status;
  displayRssi = rssi;

  display.setCursor(90, 0);
  if (!mqttStatus)
    display.println("MQTT");
  displayMqttStatus = mqttStatus;

  if (sprinkler) {

    display.setCursor(0, 56);
    if ((state & 0xf0) > 0) {
      display.println(translateIndex(state & 0x0f) + "/" + translateIndex((state >> 4) & 0x0f));
    } else {
      display.println(translateIndex(state & 0x0f));
    }
    displayState = state;
  
    display.drawFastHLine(1, 48, 126, WHITE);
    display.drawFastHLine(1, 52, 126, WHITE);
    display.drawFastVLine(0, 49, 3, WHITE);
    display.drawFastVLine(127, 49, 3, WHITE);
    float progress;
    if (autoDuration > 0)
      progress = (float)cycles*(float)(autoDuration-remaining)/(float)autoDuration - (float)icycle;
    else
      progress = 0;
    display.drawFastHLine(0, 49, progress*128, WHITE);
    display.drawFastHLine(0, 50, progress*128, WHITE);
    display.drawFastHLine(0, 51, progress*128, WHITE);
    float x = 0.0;
    for (int i = 0; i < nSteps-1; i++) {
      x += intervals[i];
      display.drawFastVLine(round(x*127), 49, 3, x>progress ? WHITE : BLACK);
    }

  } else {

    display.setCursor(0, 48);
    display.setTextSize(2);
    if (state == 0 && autoDuration > 0)
      display.println("   Offen");
    else if (state == 0)
      display.println(" Ventil zu");
    else if ((state & 0xf0) > 0) {
      display.println(translateIndex(state & 0x0f) + "/" + translateIndex((state >> 4) & 0x0f));
    } else {
      display.println(translateIndex(state & 0x0f));
    }
    displayState = state;
    
  }
  displaySprinkler = sprinkler;

  display.display(); 
}

void setState(int newState) {
  lastStep = millis();

  state = newState;
  Serial.print("State: ");
  Serial.println(state);

  if (autoDuration <= 0) {
    digitalWrite(mainValvePin, HIGH);
  } else {
    digitalWrite(mainValvePin, LOW);
  }
  
  for (int i = 0; i < 6; i++) {
    if (sprinkler) {
      if (((state & 0x0f) > 0) && i == (state & 0x0f)-1)
        digitalWrite(pins[i], LOW);
      else if (((state & 0xf0) > 0) && i == ((state >> 4) & 0x0f)-1)
        digitalWrite(pins[i], LOW);
      else
        digitalWrite(pins[i], HIGH);
    } else 
      digitalWrite(pins[i], HIGH);
  }
  
  mqttClient.beginMessage(updateStateTopic);
  mqttClient.print(state);
  mqttClient.endMessage();
}

void setStep(int newStep) {
  autoStep = newStep;
  Serial.print("Auto step: ");
  Serial.println(autoStep);
  if (autoStep >= 0)
    if (sprinkler)
      setState(steps[autoStep]);
  else
    setState(0);
}

void setSprinkler(bool active) {
  sprinkler = active;
  mqttClient.beginMessage(updateSprinklerTopic);
  if (sprinkler)
    mqttClient.print(1);
  else 
    mqttClient.print(0);
  mqttClient.endMessage();
}

void setAutoDuration(int newDuration, bool userIntent) {
  if (newDuration > 500)
    newDuration = 0;
  autoDuration = newDuration;
  setCycles((int)(autoDuration / 120) + 1);
  setCycle(0);
  Serial.print("Auto duration: ");
  Serial.println(autoDuration);
  
  mqttClient.beginMessage(updateAutoTopic);
  mqttClient.print(autoDuration);
  mqttClient.endMessage();
  if (autoDuration <= 0) {
    setRemaining(0, 0);
    setSprinkler(false);
    setState(0);
    setCycles(0);
    setCycle(0);
  }
  if (userIntent) {
    if (autoDuration <= 0) {
      setStep(-1);
    } else {
      mqttClient.beginMessage(notifyDoneTopic);
      mqttClient.print(0);
      mqttClient.endMessage();
      if (state == 0)
        setStep(0);
    }
  }
}

void setRemaining(int newRemaining, int remainingInStep) {
  if (remaining == newRemaining)
    return;
  remaining = newRemaining;
  Serial.print(remaining);
  Serial.println(" remaining.");
  mqttClient.beginMessage(updateAutoMinTopic);
  mqttClient.print(remaining);
  mqttClient.endMessage();
  mqttClient.beginMessage(updateAutoMinStepTopic);
  mqttClient.print(remainingInStep);
  mqttClient.endMessage();
}

void setCycles(int newCycles) {
  cycles = newCycles;
  mqttClient.beginMessage(updateAutoCyclesTopic);
  mqttClient.print(cycles);
  mqttClient.endMessage();
}

void setCycle(int newCycle) {
  icycle = newCycle;
  mqttClient.beginMessage(updateAutoCycleTopic);
  mqttClient.print(icycle);
  mqttClient.endMessage();
}

void checkNextStep() {
  if (autoDuration <= 0 || autoStep < 0)
    return;
  wakeIfAsleep();
  unsigned long currentMillis = millis();
  int timePassed = (currentMillis - lastStep) / 1000;
  int remainingInStep = intervals[autoStep]*autoDuration*60/cycles - timePassed;
  if (remainingInStep < 0) {
    if (autoStep + 1 < nSteps) {
      setStep(autoStep+1);
    } else if (icycle+1 < cycles) {
      setCycle(icycle+1);
      setStep(0);
    } else {
      setAutoDuration(0, false);
      setSprinkler(false);
      setStep(-1);
      setRemaining(0, 0);
      setCycle(0);
      setCycles(0);
      mqttClient.beginMessage(notifyDoneTopic);
      mqttClient.print(1);
      mqttClient.endMessage();
    }
  } else {
    int remainingSum = remainingInStep;
    for (int i = autoStep+1; i < nSteps; i++)
      remainingSum += intervals[i]*autoDuration*60/cycles;
    remainingSum += (cycles - icycle - 1)*autoDuration*60/cycles;
    setRemaining(remainingSum/60+1, remainingInStep/60);
  }
}

void ensureWifi() {
  status = WiFi.status();
  while (status != WL_CONNECTED) {
    process();
    Serial.print("Wifi not connected, connecting to: ");
    Serial.println(ssid);                   // print the network name (SSID);

    status = WiFi.begin(ssid, pass);

    int pauses = 0;
    while (status != WL_CONNECTED && pauses < 20) {
      delay(500);
      pauses++;
      process();
      status = WiFi.status();
    }
  }
  printWifiStatus();                        // you're connected now, so print out the status
}

void ensureMqtt() {
  mqttStatus = mqttClient.connected();
    
  while (!mqttStatus) {
    process();
    Serial.println("mqtt not connected.");
    ensureWifi();
    Serial.println("Connecting to mqtt broker...");
    mqttStatus = mqttClient.connect(broker, port);
    if (mqttStatus) {
      Serial.println("Success. Subscribing.");
      mqttClient.subscribe(sprinklerTopic);
      mqttClient.subscribe(autoTopic);
      mqttClient.subscribe(stateTopic);
      mqttClient.subscribe(triggerGarageDoorTopic);
    } else {
      for (int i = 0; i < 20; i++) {
        delay(500);
        process();
      }
    }
  }
}

void onMqttMessage(int messageSize) {
  String topic = mqttClient.messageTopic();
  if (topic == autoTopic) {
    wakeIfAsleep();
    String inBuffer;
    while (mqttClient.available()) {
      inBuffer += (char)mqttClient.read();
    }
    int newAuto = inBuffer.toInt();
    Serial.print("Received auto duration: ");
    Serial.println(newAuto);
    setAutoDuration(newAuto, true);
  } else if (topic == stateTopic) {
    wakeIfAsleep();
    String inBuffer;
    while (mqttClient.available()) {
      inBuffer += (char)mqttClient.read();
    }
    int newState = inBuffer.toInt();
    Serial.print("Received state: ");
    Serial.println(newState);
    setState(newState);
  } else if (topic == sprinklerTopic) {
    wakeIfAsleep();
    String inBuffer;
    while (mqttClient.available()) {
      inBuffer += (char)mqttClient.read();
    }
    int newState = inBuffer.toInt();
    Serial.print("Received sprinkler: ");
    Serial.println(newState);
    if (newState > 0)
      setSprinkler(true);
    else
      setSprinkler(false);
    if (autoDuration > 0) {
      setState(0);
      setAutoDuration(autoDuration, true);
    } else if (newState > 0)
      setAutoDuration(180, true);
    else
      setAutoDuration(0, true);
  } else if (topic == triggerGarageDoorTopic) {
    digitalWrite(garageDoorPin, LOW);
    delay(250);
    digitalWrite(garageDoorPin, HIGH);
  }
}

void printWifiStatus() {
  // print the SSID of the network you're attached to:
  Serial.print("SSID: ");
  Serial.println(WiFi.SSID());

  // print your board's IP address:
  IPAddress ip = WiFi.localIP();
  Serial.print("IP Address: ");
  Serial.println(ip);

  // print the received signal strength:
  rssi = WiFi.RSSI();
  Serial.print("signal strength (RSSI):");
  Serial.print(rssi);
  Serial.println(" dBm");

}
