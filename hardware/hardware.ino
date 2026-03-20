#ifndef ARDUINO_H
#include <Arduino.h>
#endif 

#ifndef STDLIB_H
#include <stdlib.h>
#endif

#ifndef ARDUINOJSON_H
#include <ArduinoJson.h>
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
#define DHT_PIN 32
#define DHT_TYPE DHT22
#define SOIL_PIN 14

#define SOIL_DRY_VALUE 2200
#define SOIL_WET_VALUE 480

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

// -- Function Prototypes --------------------------------------
void initDisplay();
void drawStaticUI();
void updateDisplay(const WeatherData &d);
void readSensors(WeatherData &d);
void drawHeader();
int  readSoilPercent(int rawADC);
uint16_t soilColour(int pct);

void setup() {
  Serial.begin(115200);
  Serial.println("\n=== ESP32 Weather Station ===");

  // I2C for BMP280
  Wire.begin(25, 33);

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
  readSensors(data);
  updateDisplay(data);
  lastReadTime = millis();
}

void loop() {
  unsigned long now = millis();
  if (now - lastReadTime >= READ_INTERVAL_MS) {
    readSensors(data);
    updateDisplay(data);
    lastReadTime = now;

    // Serial Monitor output for debugging
    Serial.printf("BMP  Temp: %.1fC  Press: %.1f hPa  Alt: %.1f m\n",
                  data.temperature_c, data.pressure_hpa, data.altitude_m);
    Serial.printf("DHT  Temp: %.1fC  Hum: %.1f%%  HeatIdx: %.1fC\n",
                  data.dht_temp_c, data.humidity_pct, data.heat_index_c);
    Serial.printf("Soil Moisture: %d%%\n\n", data.soil_moisture_pct);
    Serial.printf("Raw ADC: %d\n", analogRead(SOIL_PIN));
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

  // Vertical divider between columns
  tft.drawFastVLine(160, 45, 185, CLR_DIVIDER);

  // Section headings
  tft.setTextSize(1);
  tft.setTextColor(ILI9341_CYAN);
  tft.setCursor(8,   50); tft.print("-- BMP280 --");
  tft.setCursor(168, 50); tft.print("-- AM2302 --");
  tft.setCursor(168, 160); tft.print("-- SOIL --");

  // Static labels
  tft.setTextColor(CLR_LABEL);

  // Left column – BMP280
  tft.setCursor(8,  68);  tft.print("Air Temp   :");
  tft.setCursor(8,  98);  tft.print("Pressure   :");
  tft.setCursor(8,  128); tft.print("Altitude   :");

  // Right column – AM2302
  tft.setCursor(168, 68);  tft.print("Humidity   :");
  tft.setCursor(168, 98);  tft.print("DHT Temp   :");
  tft.setCursor(168, 128); tft.print("Heat Index :");

  // Bottom – Soil
  tft.setCursor(168, 175); tft.print("Moisture   :");
}

void updateDisplay(const WeatherData &d) {
  if (!displayInitialized) return;

  // Wider/taller clear area to match size 2 text (12px tall, ~14px per char)
  auto clearField = [&](int x, int y) {
    tft.fillRect(x, y, 100, 16, CLR_BG);
  };

  // -- BMP280 values (left column) --
  if (d.bmp_ok) {
    clearField(8, 80);
    tft.setCursor(8, 80); 
    tft.setTextSize(2);
    tft.setTextColor(CLR_VALUE);
    tft.print(d.temperature_c, 1);
    tft.setTextColor(CLR_UNIT); 
    tft.print(" C");

    clearField(8, 110);
    tft.setCursor(8, 110); 
    tft.setTextSize(2);
    tft.setTextColor(CLR_VALUE);
    tft.print(d.pressure_hpa, 1);
    tft.setTextColor(CLR_UNIT); 
    tft.print("hPa");

    clearField(8, 140);
    tft.setCursor(8, 140); 
    tft.setTextSize(2);
    tft.setTextColor(CLR_VALUE);
    tft.print(d.altitude_m, 1);
    tft.setTextColor(CLR_UNIT); 
    tft.print(" m");

  } else {
    tft.setTextColor(CLR_ERROR); 
    tft.setTextSize(2);
    tft.setCursor(8, 80); 
    tft.print("BMP280");
    tft.setCursor(8, 100); 
    tft.print("ERROR");
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

    clearField(168, 110);
    tft.setCursor(168, 110); 
    tft.setTextSize(2);
    tft.setTextColor(CLR_VALUE);
    tft.print(d.dht_temp_c, 1);
    tft.setTextColor(CLR_UNIT); 
    tft.print(" C");

    clearField(168, 140);
    tft.setCursor(168, 140); 
    tft.setTextSize(2);
    uint16_t hiColour = CLR_VALUE;
    if (d.heat_index_c >= 32.0) hiColour = CLR_ERROR;
    else if (d.heat_index_c >= 27.0) hiColour = CLR_WARN;
    tft.setTextColor(hiColour);
    tft.print(d.heat_index_c, 1);
    tft.setTextColor(CLR_UNIT); 
    tft.print(" C");

  } else {
    tft.setTextColor(CLR_ERROR); 
    tft.setTextSize(2);
    tft.setCursor(168, 80); 
    tft.print("DHT");
    tft.setCursor(168, 100); 
    tft.print("ERROR");
  }

  // -- Soil Moisture bar + value --
  tft.fillRect(168, 190, 148, 22, CLR_BG);

  tft.setCursor(168, 190);
  tft.setTextSize(2);
  tft.setTextColor(soilColour(d.soil_moisture_pct));
  tft.print(d.soil_moisture_pct);
  tft.setTextColor(CLR_UNIT); 
  tft.print(" %");

  // Progress bar
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


