#include <rom/rtc.h>

#ifndef ARDUINO_H
#include <Arduino.h>
#endif 

#ifndef STDLIB_H
#include <stdlib.h>
#endif

#ifndef ARDUINOJSON_H
#include <ArduinoJson.h>
#endif

#ifndef STDIO_H
#include <stdio.h>
#endif


#ifndef _WIFI_H 
#include <WiFi.h>
#include <HTTPClient.h>
#endif

#include <DHT.h>
#include <SPI.h>
#include <Wire.h>


#include <Adafruit_GFX.h>
#include <Adafruit_ILI9341.h>
#include <Adafruit_BMP280.h>

//-- ILI9341 Pin Definitions -----------------------
#define TFT_DC    17
#define TFT_CS    5
#define TFT_RST   16
#define TFT_CLK   18
#define TFT_MOSI  23
#define TFT_MISO  19

//--Sensor Pins -------------------
#define DHT_PIN 4
#define DHT_TYPE DHT22
#define SOIL_PIN 32

#define SOIL_DRY_VALUE 2200
#define SOIL_WET_VALUE 550

#define READ_INTERVAL_MS 3000

//--Colours---------------
#define CLR_BG        ILI9341_BLACK
#define CLR_HEADER    0x1A5F      //blue
#define CLR_TITLE     ILI9341_CYAN
#define CLR_LABEL     ILI9341_WHITE
#define CLR_VALUE     ILI9341_YELLOW
#define CLR_UNIT      ILI9341_GREEN
#define CLR_DIVIDER   0x4208      // grey
#define CLR_ERROR     ILI9341_RED
#define CLR_WARN      ILI9341_ORANGE


Adafruit_ILI9341 tft = Adafruit_ILI9341(TFT_CS, TFT_DC, TFT_MOSI, TFT_CLK, TFT_RST, TFT_MISO);
Adafruit_BMP280 bmp;
DHT dht(DHT_PIN, DHT_TYPE);

// -- Global Sensor Data ---------------------------------------
struct WeatherData {
  float temperature_c;   // from BMP280
  float pressure_hpa;
  float altitude_m;
  float dht_temp_c;      // from AM2302
  float humidity_pct;
  float heat_index_c;
  int   soil_moisture_pct;
  bool  bmp_ok;
  bool  dht_ok;
};

WeatherData data;
unsigned long lastReadTime = 0;
bool displayInitialized = false;
bool unitTempF    = false;   // false = °C,  true = °F
bool unitPressBar = false;   // false = hPa, true = bar
bool unitAltFt    = false;   // false = m,   true = ft

// MQTT CLIENT CONFIG  
static const char* pubtopic      = "620172489";                    // Add your ID number here
static const char* subtopic[]    = {"620172489_sub","/elet2415"};  // Array of Topics(Strings) to subscribe to
static const char* mqtt_server   = "www.yanacreations.com";         // Broker IP address or Domain name as a String 
static uint16_t mqtt_port        = 1883;

// WIFI CREDENTIALS
const char* ssid       = "MonaConnect"; // Add your Wi-Fi ssid
const char* password   = ""; // Add your Wi-Fi password 


// TASK HANDLES 
TaskHandle_t xMQTT_Connect          = NULL; 
TaskHandle_t xNTPHandle             = NULL;  
TaskHandle_t xLOOPHandle            = NULL;  
TaskHandle_t xUpdateHandle          = NULL;
TaskHandle_t xButtonCheckeHandle    = NULL; 

// -- Function Prototypes --------------------------------------
void initDisplay();
void drawStaticUI();
void updateDisplay(const WeatherData &d);
void readSensors(WeatherData &d);
void drawHeader();
int  readSoilPercent(int rawADC);
uint16_t soilColour(int pct);
volatile bool unitChangeRequested = false;

void checkHEAP(const char* Name);   // RETURN REMAINING HEAP SIZE FOR A TASK
void initMQTT(void);                // CONFIG AND INITIALIZE MQTT PROTOCOL
unsigned long getTimeStamp(void);   // GET 10 DIGIT TIMESTAMP FOR CURRENT TIME
void callback(char* topic, byte* payload, unsigned int length);
void initialize(void);
void publish(const WeatherData &d, unsigned long timestamp); // PUBLISH MQTT MESSAGE(PAYLOAD) TO A TOPIC
void vButtonCheck( void * pvParameters );
void vUpdate( void * pvParameters ); 

//############### IMPORT HEADER FILES ##################
#ifndef NTP_H
#include "NTP.h"
#endif

#ifndef MQTT_H
#include "mqtt.h"
#endif

void setup() {
  Serial.begin(115200);
  Serial.println("\n=== ESP32 Weather Station ===");

  // I2C for BMP280
  Wire.begin(21, 22);

  // BMP280 Init
  if (!bmp.begin(0x76)) { 
    if (!bmp.begin(0x77)) {        
      Serial.println("[WARN] BMP280 not found - check wiring/address.");
      data.bmp_ok = false;
    } else {
      data.bmp_ok = true;
    }
  } else {
    data.bmp_ok = true;
  }

  if (data.bmp_ok) {
    bmp.setSampling(Adafruit_BMP280::MODE_NORMAL,
                    Adafruit_BMP280::SAMPLING_X2,
                    Adafruit_BMP280::SAMPLING_X16,
                    Adafruit_BMP280::FILTER_X16,
                    Adafruit_BMP280::STANDBY_MS_500);
    Serial.println("[OK]  BMP280 initialised.");
  }

  // AM2302 / DHT22 Init
  dht.begin();
  data.dht_ok = true;
  Serial.println("[OK]  DHT22/AM2302 initialised.");

  // ADC for Soil Sensor
  analogReadResolution(12);        // 12-bit ADC (0-4095) on ESP32
  analogSetAttenuation(ADC_11db);  // Full 0-3.3V range
  Serial.println("[OK]  ADC configured for soil sensor.");

  // TFT Display Init
  initDisplay();
  drawStaticUI();

  // Initial sensor read
  initialize();
}

void loop() {
   vTaskDelay(1000 / portTICK_PERIOD_MS);
}

void vButtonCheck(void* pvParameters) {
    configASSERT(((uint32_t)pvParameters) == 1);
    for (;;) {
        vTaskDelay(1000 / portTICK_PERIOD_MS);
    }
}

void callback(char* topic, byte* payload, unsigned int length) {
    String msg = "";
    for (unsigned int i = 0; i < length; i++) msg += (char)payload[i];

    StaticJsonDocument<256> doc;
    DeserializationError err = deserializeJson(doc, msg);

    if (!err && doc["cmd"] == "set_units") {
        unitTempF    = (doc["temp_f"].as<int>()    == 1);
        unitPressBar = (doc["press_bar"].as<int>() == 1);
        unitAltFt    = (doc["alt_ft"].as<int>()    == 1);
        unitChangeRequested = true;  // signal vUpdate to redraw
        Serial.println("[CTRL] Unit change queued.");
    }
}

void publish(const WeatherData &d, unsigned long timestamp) {
    if (!mqtt.connected()) return;

    time_t now;
    time(&now);

    JsonDocument doc;
    doc["id"]         = "620172489";
    doc["timestamp"]         = (unsigned long)now;
    doc["temperature_c"]     = round(d.temperature_c * 10) / 10.0;
    doc["pressure_hpa"]      = round(d.pressure_hpa  * 10) / 10.0;
    doc["altitude_m"]        = round(d.altitude_m    * 10) / 10.0;
    doc["dht_temp_c"]        = round(d.dht_temp_c    * 10) / 10.0;
    doc["humidity_pct"]      = round(d.humidity_pct  * 10) / 10.0;
    doc["heat_index_c"]      = round(d.heat_index_c  * 10) / 10.0;
    doc["soil_moisture_pct"] = d.soil_moisture_pct;

    char message[256];
    serializeJson(doc, message);
    mqtt.publish(pubtopic, message);
    Serial.printf("[MQTT] Published: %s\n", message);
}

void vUpdate(void* pvParameters) {
    configASSERT(((uint32_t)pvParameters) == 1);
    for (;;) {
        if (mqtt.connected()) {

            // Handle unit change request from callback
            if (unitChangeRequested) {
                unitChangeRequested = false;
                drawStaticUI();       // safe — only vUpdate touches the display
            }

            readSensors(data);
            updateDisplay(data);

            time_t now;
            time(&now);
            publish(data, (unsigned long)now);

            Serial.printf("BMP  Temp: %.1fC  Press: %.1f hPa  Alt: %.1f m\n",
                          data.temperature_c, data.pressure_hpa, data.altitude_m);
            Serial.printf("DHT  Temp: %.1fC  Hum: %.1f%%  HeatIdx: %.1fC\n",
                          data.dht_temp_c, data.humidity_pct, data.heat_index_c);
            Serial.printf("Soil: %d%%\n\n", data.soil_moisture_pct);
            Serial.printf("Raw ADC: %d\n\n", analogRead(SOIL_PIN)); 
        }
        vTaskDelay(3000 / portTICK_PERIOD_MS);
    }
}

void readSensors(WeatherData &d) {

  // BMP280
  if (d.bmp_ok) {
    d.temperature_c = bmp.readTemperature();
    d.pressure_hpa  = bmp.readPressure() / 100.0F;
    d.altitude_m    = bmp.readAltitude(1013.25F);

    if (isnan(d.temperature_c) || d.pressure_hpa < 300 || d.pressure_hpa > 1100) {
      d.bmp_ok = false;
    }
  }

  // AM2302 / DHT22
  float h = dht.readHumidity();
  float t = dht.readTemperature();

  if (isnan(h) || isnan(t)) {
    d.dht_ok       = false;
    d.dht_temp_c   = 0;
    d.humidity_pct = 0;
    d.heat_index_c = 0;
    Serial.println("[WARN] DHT22 read failed.");
  } else {
    d.dht_ok       = true;
    d.dht_temp_c   = t;
    d.humidity_pct = h;
    d.heat_index_c = dht.computeHeatIndex(t, h, false);
  }

  // Capacitive Soil Moisture – average 10 samples to reduce noise
  long sum = 0;
  const int NUM_SAMPLES = 10;
  for (int i = 0; i < NUM_SAMPLES; i++) {
    sum += analogRead(SOIL_PIN);
    delay(5);
  }
  int rawAvg = sum / NUM_SAMPLES;
  d.soil_moisture_pct = readSoilPercent(rawAvg);
}

int readSoilPercent(int rawADC) {
  int pct = map(rawADC, SOIL_DRY_VALUE, SOIL_WET_VALUE, 0, 100);
  return constrain(pct, 0, 100);
}

void initDisplay() {
  tft.begin();
  tft.setRotation(3);
  tft.fillScreen(CLR_BG);
  displayInitialized = true;
  Serial.println("[OK]  ILI9341 display initialised (320x240 landscape).");
}

void drawHeader() {
  tft.fillRect(0, 0, 320, 40, CLR_HEADER);
  tft.setTextSize(2);
  tft.setTextColor(CLR_TITLE);
  tft.setCursor(30, 10);
  tft.print("ESP32  WEATHER  STATION");
}

void drawStaticUI() {
    if (!displayInitialized) return;
 
    tft.fillScreen(CLR_BG);
    drawHeader();
 
    tft.drawFastVLine(160, 45, 185, CLR_DIVIDER);
 
    tft.setTextSize(1);
    tft.setTextColor(ILI9341_CYAN);
    tft.setCursor(8,   50); tft.print("-- BMP280 --");
    tft.setCursor(168, 50); tft.print("-- AM2302 --");
    tft.setCursor(168, 160); tft.print("-- SOIL --");
 
    tft.setTextColor(CLR_LABEL);
 
    tft.setCursor(8,  68);
    tft.print(unitTempF ? "Air Temp(F):" : "Air Temp   :");
 
    tft.setCursor(8,  98);
    tft.print(unitPressBar ? "Press(bar) :" : "Pressure   :");
 
    tft.setCursor(8,  128);
    tft.print(unitAltFt ? "Alt (ft)   :" : "Altitude   :");
 
    tft.setCursor(168, 68);  tft.print("Humidity   :");
    tft.setCursor(168, 98);
    tft.print(unitTempF ? "DHT Tmp(F) :" : "DHT Temp   :");
    tft.setCursor(168, 128);
    tft.print(unitTempF ? "HeatIdx(F) :" : "Heat Index :");
 
    tft.setCursor(168, 175); tft.print("Moisture   :");
}

void updateDisplay(const WeatherData &d) {
    if (!displayInitialized) return;
 
    auto clearField = [&](int x, int y) {
        tft.fillRect(x, y, 120, 16, CLR_BG);
    };
 
    
    if (d.bmp_ok) {
 
        // Temperature
        clearField(8, 80);
        tft.setCursor(8, 80);
        tft.setTextSize(2);
        tft.setTextColor(CLR_VALUE);
        if (unitTempF) {
            float tf = d.temperature_c * 9.0f / 5.0f + 32.0f;
            tft.print(tf, 1);
            tft.setTextColor(CLR_UNIT);
            tft.print(" F");
        } else {
            tft.print(d.temperature_c, 1);
            tft.setTextColor(CLR_UNIT);
            tft.print(" C");
        }
 
        // Pressure
        clearField(8, 110);
        tft.setCursor(8, 110);
        tft.setTextSize(2);
        tft.setTextColor(CLR_VALUE);
        if (unitPressBar) {
            float bar = d.pressure_hpa / 1000.0f;
            tft.print(bar, 3);
            tft.setTextColor(CLR_UNIT);
            tft.print("bar");
        } else {
            tft.print(d.pressure_hpa, 1);
            tft.setTextColor(CLR_UNIT);
            tft.print("hPa");
        }
 
        // Altitude
        clearField(8, 140);
        tft.setCursor(8, 140);
        tft.setTextSize(2);
        tft.setTextColor(CLR_VALUE);
        if (unitAltFt) {
            float ft = d.altitude_m * 3.28084f;
            tft.print(ft, 1);
            tft.setTextColor(CLR_UNIT);
            tft.print(" ft");
        } else {
            tft.print(d.altitude_m, 1);
            tft.setTextColor(CLR_UNIT);
            tft.print(" m");
        }
 
    } else {
        tft.setTextColor(CLR_ERROR);
        tft.setTextSize(2);
        tft.setCursor(8, 80);  tft.print("BMP280");
        tft.setCursor(8, 100); tft.print("ERROR");
    }
 
    // -- AM2302 values (right column) --
    if (d.dht_ok) {
 
        clearField(168, 80);
        tft.setCursor(168, 80);
        tft.setTextSize(2);
        tft.setTextColor(CLR_VALUE);
        tft.print(d.humidity_pct, 1);
        tft.setTextColor(CLR_UNIT);
        tft.print(" %");
 
        // DHT Temp
        clearField(168, 110);
        tft.setCursor(168, 110);
        tft.setTextSize(2);
        tft.setTextColor(CLR_VALUE);
        if (unitTempF) {
            float tf = d.dht_temp_c * 9.0f / 5.0f + 32.0f;
            tft.print(tf, 1);
            tft.setTextColor(CLR_UNIT);
            tft.print(" F");
        } else {
            tft.print(d.dht_temp_c, 1);
            tft.setTextColor(CLR_UNIT);
            tft.print(" C");
        }
 
        // Heat Index
        clearField(168, 140);
        tft.setCursor(168, 140);
        tft.setTextSize(2);
        float hiDisplay = unitTempF
            ? (d.heat_index_c * 9.0f / 5.0f + 32.0f)
            : d.heat_index_c;
        float hiThreshWarn   = unitTempF ? 80.6f : 27.0f;   
        float hiThreshDanger = unitTempF ? 89.6f : 32.0f;   
        uint16_t hiColour = CLR_VALUE;
        if (hiDisplay >= hiThreshDanger) hiColour = CLR_ERROR;
        else if (hiDisplay >= hiThreshWarn) hiColour = CLR_WARN;
        tft.setTextColor(hiColour);
        tft.print(hiDisplay, 1);
        tft.setTextColor(CLR_UNIT);
        tft.print(unitTempF ? " F" : " C");
 
    } else {
        tft.setTextColor(CLR_ERROR);
        tft.setTextSize(2);
        tft.setCursor(168, 80);  tft.print("DHT");
        tft.setCursor(168, 100); tft.print("ERROR");
    }
 
    tft.fillRect(168, 190, 148, 22, CLR_BG);
    tft.setCursor(168, 190);
    tft.setTextSize(2);
    tft.setTextColor(soilColour(d.soil_moisture_pct));
    tft.print(d.soil_moisture_pct);
    tft.setTextColor(CLR_UNIT);
    tft.print(" %");
 
    int barLen = map(d.soil_moisture_pct, 0, 100, 0, 148);
    barLen = constrain(barLen, 0, 148);
    tft.fillRect(168, 215, 148, 10, CLR_DIVIDER);
    tft.fillRect(168, 215, barLen, 10, soilColour(d.soil_moisture_pct));
}

uint16_t soilColour(int pct) {
  if (pct < 20) return CLR_ERROR;       // Very dry - red
  if (pct < 40) return ILI9341_ORANGE;  // Dry - orange
  if (pct < 70) return ILI9341_GREEN;   // Optimal - green
  return ILI9341_BLUE;                   // Wet - blue
}


