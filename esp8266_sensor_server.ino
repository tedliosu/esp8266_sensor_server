
#include <ESP8266WiFi.h>
#include <ESPAsyncTCP.h>
#include <ESPAsyncWebServer.h>
#include <LittleFS.h>
#include <Wire.h> 
#include <LiquidCrystal_I2C.h>
#include <DHT.h>
#include "WiFiSecrets.h"

// Timer variables
const unsigned long sendDelay = 4000;
const unsigned long sensorDelay = 2000;
const unsigned long msgPrintDelay = 1000;
// Builtin LED pin number
const uint8_t builtinLed = 0;
// IP address beginning location on LCD display.
const uint8_t ipBeginRow = 1;
const uint8_t ipBeginCol = 0;
// LCD SDA and SCL pin numbers
const uint8_t lcdSdaPin = 1;
const uint8_t lcdSclPin = 3;
// size of individual readings buffer
const uint8_t indReadBuffSize = 6;
// Max number of websocket clients
const std::size_t maxNumClients = 3;
// Code to send to client representing num clients connected has exceeded maximum
const uint16_t maxNumCliExCode = 4050;
// More timer variables
unsigned long lastTimeStampSinceBroadcast = 0;
unsigned long lastTimeSinceSensorPolled = 0;
// Set the LCD address to 0x27 for a 16 chars and 2 line display
LiquidCrystal_I2C lcd(0x27, 16, 2);
// Pin 2, DHT22 type sensor
DHT dht(2, DHT22);
// Create AsyncWebServer object on port 80
AsyncWebServer server(80);
// Create a WebSocket object connecting to uri "ws://[IP-OF-ESP8266-HERE]/wbskt"
AsyncWebSocket websktConn("/wbskt");
char allReadings[35] = {'\0'};
float temprt = 0;
float humid = 0;

// Obtains sensor readings via global variables
void getSensorReadings() {
  char temprtReading[indReadBuffSize] = {'\0'};
  char humidReading[indReadBuffSize] = {'\0'};
  if (millis() - lastTimeSinceSensorPolled > sensorDelay) {
    temprt = dht.readTemperature();
    humid = dht.readHumidity();
    lastTimeSinceSensorPolled = millis();
  }
  if (!isnan(temprt)) {
    snprintf_P(temprtReading, sizeof(temprtReading), PSTR("%.2f"), temprt);
  } else {
    snprintf_P(temprtReading, sizeof(temprtReading), PSTR("%s"), PSTR("\"nan\""));
  }
  if (!isnan(humid)) {
    snprintf_P(humidReading, sizeof(humidReading), PSTR("%.2f"), humid);
  } else {
    snprintf_P(humidReading, sizeof(humidReading), PSTR("%s"), PSTR("\"nan\""));
  }
  memset(allReadings, 0, sizeof(allReadings));
  strcat_P(allReadings, PSTR("{ \"temp\": "));
  strcat(allReadings, temprtReading);
  strcat_P(allReadings, PSTR(", \"humd\": "));
  strcat(allReadings, humidReading);
  strcat_P(allReadings, PSTR(" }\r\n"));
}

// Event handler that prevents too many clients from connecting at once.
void onEvent(AsyncWebSocket *server, AsyncWebSocketClient *client, AwsEventType type, void *arg, uint8_t *data, size_t len) {
  switch (type) {
    case WS_EVT_CONNECT:
      // Plus one stands for this new client connecting
      if (server->getClients().size() + 1 > maxNumClients) {
        client->text(F("{ \"temp\": \"CONN_DENIED\", \"humd\": \"CONN_DENIED\" }\r\n"));
        client->close(maxNumCliExCode);
      }
      break;
    case WS_EVT_DISCONNECT:
    case WS_EVT_DATA:
    case WS_EVT_PONG:
    case WS_EVT_ERROR:
      break;
  }
}

void initFS() {
  if (!LittleFS.begin()) {
    lcd.print("FS mount ERR!");
    while (true) {}
  } else {
    lcd.print("FS mount OK!");
  }
  delay(msgPrintDelay);
}

void initWiFi() {
  WiFi.mode(WIFI_STA);
  WiFi.begin(ssid, password);
  while (WiFi.status() != WL_CONNECTED) {
    lcd.print('.');
    delay(msgPrintDelay);
  }
}


void setup() {
  pinMode(builtinLed, OUTPUT);
  digitalWrite(builtinLed, LOW);
  lcd.begin(lcdSdaPin, lcdSclPin);
  lcd.backlight();
  dht.begin();
  initFS();
  lcd.clear();
  initWiFi();
  websktConn.onEvent(onEvent);
  server.addHandler(&websktConn);
    // Web Server Root URL
  server.on("/", HTTP_GET, [](AsyncWebServerRequest *request) {
    request->send(LittleFS, "/index.html", "text/html");
  });
  /*
   * Have server serve root statically regardless of whatever
   * requests are sent from clients.
   */
  server.serveStatic("/", LittleFS, "/");
  // Start server
  server.begin();
  // Display IP address to connect to
  lcd.clear();
  lcd.print("Local IP:");
  lcd.setCursor(ipBeginCol, ipBeginRow);
  lcd.print(WiFi.localIP().toString());
}

void loop() {
  if ((millis() - lastTimeStampSinceBroadcast > sendDelay)) {
    digitalWrite(builtinLed, HIGH);
    getSensorReadings();
    websktConn.textAll(allReadings);
    lastTimeStampSinceBroadcast = millis();
    digitalWrite(builtinLed, LOW);
  }
  websktConn.cleanupClients();
}
