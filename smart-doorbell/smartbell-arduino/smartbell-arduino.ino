#include <ESP8266WiFi.h>
#include <PubSubClient.h>
#include <ArduinoOTA.h>

#include "arduino_secrets.h" 
const char* ssid = SECRET_SSID;
const char* pass = SECRET_PASS;

// Important: Passwords and OTA
// This code is designed to run on a secure private network. Normally, the
// device should be connected to your secured local network and only allows
// for an OTA update if you enable this function via MQTT (see command below)
// This is a problem if you want to change your Wifi password or if your MQTT
// broker moves to a different address and you want to update the smart bell
// with new credentials or addresses. Therefore, if the device cannot connect
// to your Wifi or cannot connect to the MQTT broker for a few times it will
// start its own Wifi with the credentials below. You probably want to change
// them to be unique to your device and you want to save them somewhere in
// case you need to recover the smart bell. In this fallback mode it will only
// allow an OTA update so you can set new credentials. After a minute it will
// try again to connect to your regular network, so it will not stay down too
// long after a short outage of your infrastructure.

const char* ssidFallback = "SmartBell";
const char* passFallback = "NoneOfYourBusiness";

const char* mqttTopicOTA = "smartbell/ota";
const char* mqttTopicHousePhone = "smartbell/housephone";
const char* mqttTopicHousePhoneSet = "smartbell/housephone/set";
const char* mqttTopicOpener = "smartbell/opener";
const char* mqttTopicRinging = "smartbell/ringing";

const byte pinDisconnect = 12;
const byte pinOpener = 4;
const byte pinRinging = 14;

WiFiClient wifiClient;
PubSubClient client(wifiClient);

boolean otaEnabled;
boolean isRinging;
boolean phoneEnabled;

unsigned long phoneOffTime;
unsigned long ringDebounce;

void setup() {
  pinMode(pinDisconnect, OUTPUT);
  pinMode(pinOpener, OUTPUT);
  pinMode(pinRinging, INPUT_PULLUP);

  digitalWrite(pinDisconnect, LOW); //PCB layout is such the house phone IS CONNECTED when low (saving power as this is the normal state)
  digitalWrite(pinOpener, LOW); //PCB layout is such that opener is not triggering when low
  
  Serial.begin(115200);

  ArduinoOTA.setHostname("SmartBell");
  
  client.setServer(MQTT_BROKER, 1883);
  client.setCallback(mqttCallback);
}

void loop() {
  //Normal state: Connect to Wifi infrastructure and MQTT broker
  int wifiStatus = WiFi.begin(ssid, pass);
  int retries = 0;
  unsigned long timeout;
  while (retries < 3) {
    retries++;
    Serial.print("Connecting to Wifi, try ");
    Serial.println(retries);
    
    timeout = millis();
    while (wifiStatus != WL_CONNECTED && millis()-timeout < 20000) {
      delay(100);
      wifiStatus = WiFi.status();
    }
    while (wifiStatus == WL_CONNECTED) {
      Serial.println("Connected. Trying MQTT...");
      timeout = millis();
      while (!client.connected() && millis()-timeout < 10000) {
        client.connect("SmartBell");
        delay(1000);
      }
      if (!client.connected()) {
        Serial.print("Failed.");
        break;
      }
      Serial.println("MQTT connected");
      retries = 0;
      normalSetup();
      Serial.println("Entering normal loop.");
      while (client.connected() && WiFi.status() == WL_CONNECTED) {
        normalLoop();
        yield();
      }
      Serial.println("Disconnected.");
    }
  }
  client.disconnect();
  WiFi.disconnect();
  
  //Fallback: Offer a hotspot to allow for an OTA update
  Serial.println("Setting up AP as fallback.");
  setHousePhone(true); //The bell should work if we are not properly connected to the infrastructure
  if (WiFi.softAP(ssidFallback, passFallback)) {
    ArduinoOTA.begin();
    timeout = millis();
    while (millis() - timeout < 60000) {
      ArduinoOTA.handle();
      yield();
    }
  }
  Serial.println("Let's see if our Wifi is back...");
  ESP.restart();
}

void normalSetup() {
  Serial.println("Setup for normal mode.");
  isRinging = false;
  ringDebounce = millis();
  otaEnabled = false;
  setHousePhone(true);
  client.subscribe(mqttTopicOTA);
  client.subscribe(mqttTopicHousePhoneSet);
  client.subscribe(mqttTopicOpener);
  Serial.println("Done.");
}

void normalLoop() {
  client.loop();
  checkRinging();
  if (otaEnabled) {
    ArduinoOTA.handle();
  }
  if (!phoneEnabled and millis() - phoneOffTime > 1000*60*60*3) //Reset after three hours, so it will not stay off indefinetly if the controlling system fails
    setHousePhone(true);
}

void setHousePhone(bool enabled) {
  Serial.print("Set house phone to ");
  Serial.println(enabled);
  if (enabled)
    digitalWrite(pinDisconnect, LOW);
  else {
    phoneOffTime = millis();
    digitalWrite(pinDisconnect, HIGH);
  }
  phoneEnabled = enabled;
  client.publish(mqttTopicHousePhone, enabled ? "1" : "0");
}

void triggerOpener() {
  Serial.println("Triggering opener");
  if (!phoneEnabled) {
    digitalWrite(pinDisconnect, LOW);
    delay(500);
  }
  digitalWrite(pinOpener, HIGH);
  delay(500); //This is just "pushing the button". The doorbell system itself triggers the opener for several seconds regardless of how long the button is being pressed.
  digitalWrite(pinOpener, LOW);
  if (!phoneEnabled) {
    digitalWrite(pinDisconnect, HIGH);
  }
}

bool checkRinging() {
  bool nowRinging = (digitalRead(pinRinging) == LOW); //Pulled low by optocoupler if ringing

  if (nowRinging && !isRinging && millis() - ringDebounce > 3000) {
    ringDebounce = millis();
    Serial.println("Palim, palim!");
    client.publish(mqttTopicRinging, "");
  }
  isRinging = nowRinging;
}

void mqttCallback(char* topic, byte* payload, unsigned int length) {
    if (strcmp(topic, mqttTopicOTA) == 0) {
      otaEnabled = payload[0] == '1';
      if (otaEnabled)
        ArduinoOTA.begin();
      else
        ESP.restart();
      Serial.print("OTA update set to ");
      Serial.println(otaEnabled);
    } else if (strcmp(topic, mqttTopicHousePhoneSet) == 0) {
      setHousePhone(payload[0] != '0');
    } else if (strcmp(topic, mqttTopicOpener) == 0) {
      triggerOpener();
    }
}
