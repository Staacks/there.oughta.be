#define ENABLE_GxEPD2_GFX 0

#include <GxEPD2_BW.h>
#include <GxEPD2_3C.h>
#include <Fonts/FreeSans12pt7b.h>
#include <WiFi.h>
#include <HTTPClient.h>
#include <WebServer.h>
#include <FS.h>
#include <SPIFFS.h>
#include <esp_spiffs.h>
#include "Adafruit_WS2801.h"

extern "C" {
  #include "crypto/base64.h"
}


const int nWifi = 4;
String ssids[nWifi] = {"name1", "name2", "name3", "name4"};
String passwords[nWifi] = {"pass1", "pass2", "", "pass4"}; //Use "" for open networks
int selectedWifi;

WebServer server(80);

int storedImages = 0;
int currentImage = -1;
int showNext = -1;
int partialUpdateCount = 0;

GxEPD2_BW<GxEPD2_750_T7, GxEPD2_750_T7::HEIGHT> display(GxEPD2_750_T7(/*CS=*/ 5, /*DC=*/ 27, /*RST=*/ 26, /*BUSY=*/ 25));

int nLEDs = 6;
Adafruit_WS2801 strip = Adafruit_WS2801(nLEDs, 15, 4); //number of LEDs, data pin, clock pin
float gradientOffset = -1.0;

void setup() {
  Serial.begin(115200);
  Serial.println();
  Serial.println("setup");
  delay(100);
  
  strip.begin();
  strip.show();
  for (int i = 0; i < nLEDs; i++)
    strip.setPixelColor(i, hsvToRgb(i/6.0, 0.8, 1.0));
  strip.show();
  
  display.init(0, true, 2, false);
  display.setRotation(0);
  
  displayText("Hallo.", false);

  randomSeed(analogRead(0));

  Serial.println("Checking file system.");
  SPIFFS.begin(true);
  File dir = SPIFFS.open("/");
  File file;
  while (file = dir.openNextFile()) {
    if(!file.isDirectory()) {
      Serial.print("File ");
      Serial.println(file.name());
      storedImages++;
    }
  }
  dir.close();

  Serial.print("We already have ");
  Serial.print(storedImages);
  Serial.println(" images in memory.");
  printSpiffsInfo();
  
  displayText("Suche bekanntes WLAN-Netz...", true);
  while (!selectWifi()) {
    displayText("Noch kein bekanntes WLAN-Netz gefunden...", true);
  }
  char infoText[100];
  sprintf(infoText, "Verbinde mit \"%s\"...", ssids[selectedWifi]);
  displayText(infoText, true);
  connectToWifi(5);
  while (WiFi.status() != WL_CONNECTED) {
      Serial.println("WiFi not connected. Trying to reconnect");
      WiFi.disconnect();
      WiFi.mode(WIFI_OFF);
      WiFi.mode(WIFI_STA);
      connectToWifi(5);
  }
  sprintf(infoText, "Erfolgreich mit \"%s\" verbunden.", ssids[selectedWifi]);
  displayText(infoText, true);

  server.on("/clear", server_clear);
  server.onNotFound(server_404);
  server.begin();
  
  delay(500);

  displayText("Bereit.", true);
  
  Serial.println("setup complete");
}

unsigned long lastPoll = 0;
unsigned long lastImageChange = 0;

void loop() {
  if (WiFi.status() != WL_CONNECTED) {
      Serial.println("WiFi not connected. Try to reconnect");
      WiFi.disconnect();
      WiFi.mode(WIFI_OFF);
      WiFi.mode(WIFI_STA);
      connectToWifi(5);
  }
  server.handleClient();
  unsigned long now = millis();
  if (now - lastPoll > 5000) {
    pollImages();
    lastPoll = millis();
  }
  if (storedImages > 0) {
    if (showNext >= 0) {
      if (now - lastImageChange > 3000) {
        showImage(showNext);
        showNext = -1;
        lastImageChange = millis();
      }
    } else {
      if (now - lastImageChange > 20000) {
        int nextImage = currentImage;
        while (nextImage == currentImage && storedImages > 1)
          nextImage = random(storedImages);
        showImage(nextImage);
        lastImageChange = millis();
      }
    }
  }
}

uint32_t hsvToRgb(double h, double s, double v) {
  double r, g, b;

  auto i = (int)(h * 6);
  auto f = h * 6 - i;
  auto p = v * (1 - s);
  auto q = v * (1 - f * s);
  auto t = v* (1 - (1 - f) * s);

  switch (i % 6) {
  case 0: r = v, g = t , b = p;
    break;
  case 1: r = q , g = v , b = p;
    break;
  case 2: r = p , g = v , b = t;
    break;
  case 3: r = p , g = q , b = v;
    break;
  case 4: r = t , g = p , b = v;
    break;
  case 5: r = v , g = p , b = q;
    break;
  }

  return ((uint8_t)(r * 255) << 16) | ((uint8_t)(g * 255) << 8) | ((uint8_t)(b * 255));
}

void printSpiffsInfo() {
  size_t total = 0, used = 0;
  if (esp_spiffs_info(NULL, &total, &used)) {
      Serial.println("Error: Could not get SPIFFS info.");
  } else {
      Serial.print("SPIFFS using ");
      Serial.print(used);
      Serial.print(" of ");
      Serial.print(total);
      Serial.println(" bytes.");
  }
}

void displayText(String text, bool partialUpdate) {
  display.setFont(&FreeSans12pt7b);
  display.setTextColor(GxEPD_BLACK);
  int16_t tbx, tby;
  uint16_t tbw, tbh;
  display.getTextBounds(text, 0, 0, &tbx, &tby, &tbw, &tbh);
  uint16_t x = ((display.width() - tbw) / 2) - tbx;
  uint16_t y = ((display.height() - tbh) / 2) - tby;
  display.setFullWindow();
  display.fillScreen(GxEPD_WHITE);
  display.setCursor(x, y);
  display.print(text);
  display.display(partialUpdate);
}

bool selectWifi() { 
  selectedWifi = nWifi;
  int n = WiFi.scanNetworks();
  Serial.print("Found ");
  Serial.print(n);
  Serial.println(" networks:");
  for (int i = 0; i < n; i++) {
    Serial.print("    ");
    Serial.println(WiFi.SSID(i));
    for (int j = 0; j < selectedWifi; j++) {
      if (ssids[j] == WiFi.SSID(i)) {
        selectedWifi = j;
        break;
      }
    }
  }
  if (selectedWifi < nWifi) {
    Serial.print("Selected ");
    Serial.println(ssids[selectedWifi]);
    return true;
  } else {
    Serial.println("No known Wifi found.");
    return false;
  }
}

void connectToWifi(int timeout) {
  if (passwords[selectedWifi] != "")
    WiFi.begin(ssids[selectedWifi].c_str(), passwords[selectedWifi].c_str());
  else
    WiFi.begin(ssids[selectedWifi].c_str());
  WiFi.setHostname("Guestbook");
  int totalWait = 0;
  while (WiFi.status() != WL_CONNECTED) {
    delay(1000);
    totalWait++;
    if (timeout > 0 && totalWait > timeout)
      return;
  }
  Serial.print("Connected to ");
  Serial.println(ssids[selectedWifi]);
  Serial.print("IP-Address: ");
  Serial.println(WiFi.localIP());
}

void server_clear() {
  Serial.println("Server: Clear requested.");

  for (int i = 0; i < storedImages; i++) {
    char filename[10];
    sprintf(filename, "/%d.bin", i);
    Serial.print(filename);
    if (SPIFFS.remove(filename)) {
      Serial.println(" deleted.");
    } else {
      Serial.println(" error.");
    }
  }
  
  storedImages = 0;
  server.send(200, "text/html", "<html><body>All images deleted.</body></html>");
  printSpiffsInfo();
}

void server_404() {
  Serial.println("Server: 404.");
  server.send(404, "text/html", "<html><body><a href=\"https://MYURL\">MYURL</a></body></html>"); 
}

void pollImages() {
  Serial.println("Polling.");
  int imagesOnServer = 0;
  {
    
    //Submit status (SSID, IP, number of images, storage stats), so we can conveniently check if the device is properly running during the party
    char url[200];
    size_t total = 0, used = 0;
    esp_spiffs_info(NULL, &total, &used);
    IPAddress ip = WiFi.localIP();
    sprintf(url, "https://MYURL/poll.php?ssid=%s&ip=%d.%d.%d.%d&images=%d&storageused=%d&storagetotal=%d", "ERROR", ip[0], ip[1], ip[2], ip[3], storedImages, used, total);
    Serial.print("Polling ");
    Serial.println(url);

    HTTPClient http;
    http.begin(url);
    int result = http.GET();
    if (result == 200) {
      imagesOnServer = http.getString().toInt();
    } else {
      Serial.print("Error on HTTP request: ");
      Serial.println(result);
    }
    http.end();
  }
  if (imagesOnServer > storedImages) {
    Serial.println("Found new images.");
    for (int i = storedImages; i < imagesOnServer; i++)
      if (!pollImage(i))
        break;
  }
}

bool pollImage(int id) {
  size_t outputLength = 0;
  unsigned char * decoded;
  char url[50];
  sprintf(url, "https://MYURL/poll.php?get=%d", id);
  Serial.print("Requesting image from ");
  Serial.println(url);
  {
    HTTPClient http;
    http.begin(url);
    int result = http.GET();
    if (result == 200) {
      String response = http.getString();
      Serial.println("Got something. Decoding...");
      decoded = base64_decode((const unsigned char *)response.c_str(), response.length(), &outputLength);
    } else {
      Serial.print("Error on HTTP request: ");
      Serial.println(result);
    }
    http.end();
  }

  if (outputLength == 0) {
    Serial.print("Could not fetch image ");
    Serial.println(id);
    return false;
  }
  
  char filename[10];
  sprintf(filename, "/%d.bin", id);
  Serial.print("Saving to ");
  Serial.print(filename);
  Serial.println("...");

  File file = SPIFFS.open(filename, FILE_WRITE);
  if (!file) {
        Serial.println("Failed!");
        return false;
  }
  if (!file.write(decoded, outputLength)) {
    Serial.println("Write failed");
    file.close();
    free(decoded);
    return false;
  }
  file.close();
  free(decoded);
  
  Serial.print("Success. We now have ");
  Serial.print(storedImages);
  Serial.println(" images.");
  showNext = id;
  storedImages = id + 1;

  printSpiffsInfo();
  return true;
}

void showImage(int id) {
  currentImage = id;
  char filename[10];
  sprintf(filename, "/%d.bin", id);
  Serial.print("Showing image ");
  Serial.print(filename);
  Serial.println("...");

  File file = SPIFFS.open(filename);
  if (!file) {
        Serial.println("Failed!");
        return;
  }

  display.setFullWindow();
  display.fillScreen(GxEPD_WHITE);
  int line = 0;
  while (file.available()) {
    byte repetitions = file.read();
    int x = 0;
    if (repetitions == 0) { //The next 100 bytes simply encode pixels directly
      for (int j = 0; j < 100; j++) {
        if (!file.available())
          break;
        byte pixelData = file.read();
        for (byte mask = B10000000; mask > 0; mask >>= 1) {
          if (pixelData & mask)
            display.drawPixel(x, line, GxEPD_BLACK);
          x++;
        }
      }
    } else { //The next [repetitions] bytes encode number of identical pixels, starting with white (which of course may be zero)
      bool white = true;
      for (int j = 0; j < repetitions; j++) {
        if (!file.available())
          break;
        byte pixelCount = file.read();
        if (white) { // White
          x += pixelCount;
        } else { // Black
          for (int xi = 0; xi < pixelCount; xi++) {
            display.drawPixel(x, line, GxEPD_BLACK);
            x++;
          } 
        }
        if (pixelCount < 255)
          white = !white;
      }
    }
    line++;
  }
  file.close();

  //Fade out old LED colors
  for (int t = 0; t <= 1000; t+=20) {
    for (int i = 0; i < nLEDs; i++)
      strip.setPixelColor(i, hsvToRgb(gradientOffset < 0 ? i/6.0 : gradientOffset + i/18.0, 0.8, (1000-t)/1000.0));
    strip.show();
    delay(20);
  }
  
  display.display(partialUpdateCount < 5);
  if (partialUpdateCount >= 5)
    partialUpdateCount = 0;
  else
    partialUpdateCount++;

  //Fade in new LED colors
  if (gradientOffset < 0)
    gradientOffset = 0.0;
  gradientOffset += 0.25 + random(500)/1000.0;
  for (int t = 0; t <= 1000; t+=20) {
    for (int i = 0; i < nLEDs; i++)
      strip.setPixelColor(i, hsvToRgb(gradientOffset + i/18.0, 0.8, (t)/1000.0));
    strip.show();
    delay(20);
  }
}
