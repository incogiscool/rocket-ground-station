// #include <SPI.h>
// #include <Adafruit_GFX.h>
// #include <Adafruit_ST7789.h>

// #define LCD_MOSI 23
// #define LCD_SCLK 22
// #define LCD_CS   17
// #define LCD_DC   18
// #define LCD_RST  19
// #define LCD_BL   21

// Adafruit_ST7789 lcd = Adafruit_ST7789(LCD_CS, LCD_DC, LCD_MOSI, LCD_SCLK, LCD_RST);

// void setup() {
//   Serial.begin(115200);
//   Serial.println("ST7789 test.");

//   pinMode(LCD_BL, OUTPUT);
//   analogWrite(LCD_BL, 255);

//   lcd.init(240, 320);
//   lcd.setRotation(3);
//   lcd.fillScreen(ST77XX_BLACK);
//   lcd.setTextColor(ST77XX_WHITE);
//   lcd.setTextSize(2);
//   lcd.setCursor(40, 40);
//   lcd.print("Hello World!");
// }

// void loop() {
// }

// #include <SPI.h>
// #include <Adafruit_GFX.h>
// #include <Adafruit_ST7789.h>

// #define LCD_MOSI 23
// #define LCD_SCLK 22
// #define LCD_CS   17
// #define LCD_DC   18
// #define LCD_RST  19
// #define LCD_BL   21

// #define STALE_TIMEOUT_MS 2000  // mark data stale after 2s with no packet

// Adafruit_ST7789 lcd = Adafruit_ST7789(LCD_CS, LCD_DC, LCD_MOSI, LCD_SCLK, LCD_RST);

// // --- Telemetry struct (populate from your radio parser) ---
// struct Telemetry {
//   float    lat        = 45.4215f;
//   float    lon        = -75.6972f;
//   uint8_t  fixType    = 3;      // 0=none, 2=2D, 3=3D
//   uint8_t  numSats    = 9;
//   int16_t  rssi       = -74;    // dBm
//   float    altitude   = 312.0f; // m
//   float    apogee     = 488.0f; // m  (trailing max altitude)
//   float    accel      = 2.4f;   // g
//   float    speed      = 143.0f; // m/s
// };

// Telemetry telem;
// uint32_t  lastPacketMs = 0;
// bool      wasStale     = false;

// // ---- Colour palette ----
// #define C_BG       ST77XX_BLACK
// #define C_WHITE    ST77XX_WHITE
// #define C_YELLOW   0xFFE0   // 16-bit yellow
// #define C_GREEN    0x07E0
// #define C_RED      0xF800
// #define C_CYAN     0x07FF
// #define C_GRAY     0x8410
// #define C_ORANGE   0xFD20

// void setup() {
//   Serial.begin(115200);

//   pinMode(LCD_BL, OUTPUT);
//   analogWrite(LCD_BL, 255);

//   lcd.init(240, 320);
//   lcd.setRotation(3);   // landscape — 320 wide, 240 tall
//   lcd.fillScreen(C_BG);

//   lastPacketMs = millis();
//   drawAll();
// }

// void loop() {
//   // --- Replace this block with your real radio read ---
//   if (Serial.available()) {
//     // Example: parse CSV "lat,lon,fix,sats,rssi,alt,apogee,accel,speed\n"
//     // parseIncoming();
//     lastPacketMs = millis();
//   }

//   bool stale = (millis() - lastPacketMs) > STALE_TIMEOUT_MS;
//   if (stale != wasStale) {
//     wasStale = stale;
//     drawStaleBanner(stale);
//   }

//   drawAll();
//   delay(250);
// }

// // ============================================================
// void drawAll() {
//   drawGPS();
//   drawRSSI();
//   drawBottomRow();
// }

// // ---- TOP ROW: GPS coords + fix/sats ----
// void drawGPS() {
//   // Clear top region
//   lcd.fillRect(0, 0, 320, 46, C_BG);

//   // Left: lat/lon
//   lcd.setTextSize(1);
//   lcd.setTextColor(C_GRAY);
//   lcd.setCursor(4, 4);
//   lcd.print("GPS POSITION");

//   lcd.setTextSize(2);
//   lcd.setTextColor(C_WHITE);
//   lcd.setCursor(4, 16);

//   char buf[32];
//   // Lat
//   dtostrf(abs(telem.lat), 7, 4, buf);
//   lcd.print(buf);
//   lcd.print(telem.lat >= 0 ? " N " : " S ");
//   // Lon
//   dtostrf(abs(telem.lon), 7, 4, buf);
//   lcd.print(buf);
//   lcd.print(telem.lon >= 0 ? " E" : " W");

//   // Right: fix type + sats
//   lcd.setTextSize(1);
//   lcd.setTextColor(C_GRAY);
//   lcd.setCursor(250, 4);
//   lcd.print("FIX / SATS");

//   uint16_t fixColor = (telem.fixType == 3) ? C_GREEN :
//                       (telem.fixType == 2) ? C_ORANGE : C_RED;
//   lcd.setTextSize(2);
//   lcd.setTextColor(fixColor);
//   lcd.setCursor(250, 16);
//   if      (telem.fixType == 3) lcd.print("3D");
//   else if (telem.fixType == 2) lcd.print("2D");
//   else                          lcd.print("--");

//   lcd.setTextColor(C_WHITE);
//   lcd.print("/");
//   lcd.print(telem.numSats);

//   // Divider line
//   lcd.drawFastHLine(0, 46, 320, C_GRAY);
// }

// // ---- MIDDLE: RSSI ----
// void drawRSSI() {
//   lcd.fillRect(0, 48, 320, 120, C_BG);

//   // Label
//   lcd.setTextSize(1);
//   lcd.setTextColor(C_GRAY);
//   lcd.setCursor(4, 52);
//   lcd.print("RSSI");

//   // Value — large
//   char buf[16];
//   snprintf(buf, sizeof(buf), "%d dBm", telem.rssi);

//   lcd.setTextSize(4);
//   uint16_t rssiColor = (telem.rssi > -80) ? C_YELLOW :
//                        (telem.rssi > -100) ? C_ORANGE : C_RED;
//   lcd.setTextColor(rssiColor);

//   // Centre the text
//   int16_t x1, y1; uint16_t w, h;
//   lcd.getTextBounds(buf, 0, 0, &x1, &y1, &w, &h);
//   lcd.setCursor((320 - w) / 2, 80);
//   lcd.print(buf);

//   lcd.drawFastHLine(0, 150, 320, C_GRAY);
// }

// // ---- BOTTOM ROW: alt, apogee, accel, speed ----
// void drawBottomRow() {
//   lcd.fillRect(0, 152, 320, 88, C_BG);

//   struct { const char* label; float val; const char* unit; uint16_t col; } fields[4] = {
//     { "ALT",    telem.altitude, "m",   C_WHITE  },
//     { "APOGEE", telem.apogee,   "m",   C_CYAN   },
//     { "ACCEL",  telem.accel,    "g",   C_WHITE  },
//     { "SPEED",  telem.speed,    "m/s", C_WHITE  },
//   };

//   int colW = 320 / 4;
//   for (int i = 0; i < 4; i++) {
//     int x = i * colW;
//     // Label
//     lcd.setTextSize(1);
//     lcd.setTextColor(C_GRAY);
//     lcd.setCursor(x + 4, 156);
//     lcd.print(fields[i].label);

//     // Value
//     char buf[16];
//     dtostrf(fields[i].val, 5, 1, buf);
//     lcd.setTextSize(2);
//     lcd.setTextColor(fields[i].col);
//     lcd.setCursor(x + 4, 170);
//     lcd.print(buf);

//     // Unit
//     lcd.setTextSize(1);
//     lcd.setTextColor(C_GRAY);
//     lcd.setCursor(x + 4, 196);
//     lcd.print(fields[i].unit);

//     // Vertical divider (not after last)
//     if (i < 3) lcd.drawFastVLine(x + colW - 1, 152, 88, C_GRAY);
//   }
// }

// // ---- STALE BANNER (drawn at bottom of screen) ----
// void drawStaleBanner(bool stale) {
//   int y = 215;
//   lcd.fillRect(0, y, 320, 25, C_BG);
//   if (stale) {
//     lcd.fillRect(0, y, 320, 25, C_RED);
//     lcd.setTextSize(1);
//     lcd.setTextColor(C_WHITE);
//     lcd.setCursor(60, y + 8);
//     lcd.print("!! DATA STALE - NO SIGNAL !!");
//   } else {
//     lcd.fillRect(0, y, 320, 25, 0x0320); // dark green
//     lcd.setTextSize(1);
//     lcd.setTextColor(C_GREEN);
//     lcd.setCursor(80, y + 8);
//     lcd.print("DATA LIVE");
//   }
// }


#include <SPI.h>
#include <Adafruit_GFX.h>
#include <Adafruit_ST7789.h>

#define LCD_MOSI 23
#define LCD_SCLK 22
#define LCD_CS   17
#define LCD_DC   18
#define LCD_RST  19
#define LCD_BL   21

#define STALE_TIMEOUT_MS  2000
#define SIM_INTERVAL_MS   250

Adafruit_ST7789 lcd = Adafruit_ST7789(LCD_CS, LCD_DC, LCD_MOSI, LCD_SCLK, LCD_RST);

#define C_BG      ST77XX_BLACK
#define C_WHITE   ST77XX_WHITE
#define C_YELLOW  0xFFE0
#define C_GREEN   0x07E0
#define C_RED     0xF800
#define C_CYAN    0x07FF
#define C_GRAY    0x8410
#define C_ORANGE  0xFD20
#define C_DKGREEN 0x0320

struct Telemetry {
  float   lat      = 45.4215f;
  float   lon      = -75.6972f;
  uint8_t fixType  = 3;
  uint8_t numSats  = 9;
  int16_t rssi     = -74;
  float   altitude = 312.0f;
  float   apogee   = 488.0f;
  float   accel    = 2.4f;
  float   speed    = 143.0f;
};

Telemetry cur, prev;

uint32_t lastPacketMs = 0;
uint32_t lastSimMs    = 0;
bool     wasStale     = false;
bool     firstDraw    = true;

static uint32_t _rng = 42;
float frand(float lo, float hi) {
  _rng ^= _rng << 13; _rng ^= _rng >> 17; _rng ^= _rng << 5;
  return lo + ((_rng & 0xFFFF) / 65535.0f) * (hi - lo);
}

void simulatePacket() {
  cur.lat      += frand(-0.0002f,  0.0002f);
  cur.lon      += frand(-0.0002f,  0.0002f);
  cur.fixType   = (frand(0,1) > 0.05f) ? 3 : 2;
  cur.numSats   = constrain((int)(cur.numSats + frand(-1, 1)), 4, 15);
  cur.rssi      = constrain((int)(cur.rssi + frand(-3, 3)), -120, -40);
  cur.altitude  = max(0.0f, cur.altitude + frand(-5, 8));
  cur.apogee    = max(cur.apogee, cur.altitude);
  cur.accel     = max(0.0f, cur.accel + frand(-0.3f, 0.3f));
  cur.speed     = max(0.0f, cur.speed + frand(-6, 6));
  lastPacketMs  = millis();
}

#define GPS_Y       0
#define GPS_H       46
#define GPS_VAL_X   4
#define GPS_VAL_Y   16
#define FIX_X       250
#define FIX_VAL_Y   16
#define RSSI_Y      48
#define RSSI_H      102
#define RSSI_VAL_Y  80
#define BOT_Y       152
#define BOT_H       88
#define BOT_LABEL_Y 156
#define BOT_VAL_Y   170
#define BOT_UNIT_Y  196
#define COL_W       80
#define BANNER_Y    215
#define BANNER_H    25

void redrawField(uint16_t bgColor, int16_t x, int16_t y,
                 uint8_t textSize, uint16_t color,
                 const char* str, uint16_t eraseW, uint16_t eraseH) {
  lcd.fillRect(x, y, eraseW, eraseH, bgColor);
  lcd.setTextSize(textSize);
  lcd.setTextColor(color);
  lcd.setCursor(x, y);
  lcd.print(str);
}

void drawStaticChrome() {
  lcd.fillScreen(C_BG);
  lcd.setTextSize(1); lcd.setTextColor(C_GRAY);
  lcd.setCursor(4, 4);     lcd.print("GPS POSITION");
  lcd.setCursor(FIX_X, 4); lcd.print("FIX / SATS");
  lcd.drawFastHLine(0, 46, 320, C_GRAY);
  lcd.setTextSize(1); lcd.setTextColor(C_GRAY);
  lcd.setCursor(4, 52); lcd.print("RSSI");
  lcd.drawFastHLine(0, 150, 320, C_GRAY);
  const char* labels[4] = { "ALT", "APOGEE", "ACCEL", "SPEED" };
  const char* units[4]  = { "m",   "m",      "g",     "m/s"   };
  for (int i = 0; i < 4; i++) {
    int x = i * COL_W;
    lcd.setTextSize(1); lcd.setTextColor(C_GRAY);
    lcd.setCursor(x + 4, BOT_LABEL_Y); lcd.print(labels[i]);
    lcd.setCursor(x + 4, BOT_UNIT_Y);  lcd.print(units[i]);
    if (i < 3) lcd.drawFastVLine(x + COL_W - 1, BOT_Y, BOT_H, C_GRAY);
  }
}

void updateGPS() {
  bool latChanged = (cur.lat != prev.lat || cur.lon != prev.lon);
  bool fixChanged = (cur.fixType != prev.fixType || cur.numSats != prev.numSats);

  if (latChanged) {
    char buf[40], latBuf[10], lonBuf[10];
    dtostrf(abs(cur.lat), 7, 4, latBuf);
    dtostrf(abs(cur.lon), 7, 4, lonBuf);
    snprintf(buf, sizeof(buf), "%s%s %s%s",
             latBuf, (cur.lat >= 0 ? " N " : " S "),
             lonBuf, (cur.lon >= 0 ? " E"  : " W"));
    redrawField(C_BG, GPS_VAL_X, GPS_VAL_Y, 2, C_WHITE, buf, 240, 16);
  }

  if (fixChanged) {
    uint16_t fixColor = (cur.fixType == 3) ? C_GREEN :
                        (cur.fixType == 2) ? C_ORANGE : C_RED;
    char buf[8];
    if      (cur.fixType == 3) strcpy(buf, "3D");
    else if (cur.fixType == 2) strcpy(buf, "2D");
    else                       strcpy(buf, "--");
    lcd.fillRect(FIX_X, FIX_VAL_Y, 70, 16, C_BG);
    lcd.setTextSize(2); lcd.setTextColor(fixColor);
    lcd.setCursor(FIX_X, FIX_VAL_Y);
    lcd.print(buf);
    lcd.setTextColor(C_WHITE);
    lcd.print("/");
    lcd.print(cur.numSats);
  }
}

void updateRSSI() {
  if (cur.rssi == prev.rssi) return;
  char buf[16];
  snprintf(buf, sizeof(buf), "%d dBm", cur.rssi);
  uint16_t rssiColor = (cur.rssi > -80) ? C_YELLOW :
                       (cur.rssi > -100) ? C_ORANGE : C_RED;
  lcd.fillRect(0, RSSI_VAL_Y, 320, 32, C_BG);
  lcd.setTextSize(4); lcd.setTextColor(rssiColor);
  int16_t x1, y1; uint16_t w, h;
  lcd.getTextBounds(buf, 0, 0, &x1, &y1, &w, &h);
  lcd.setCursor((320 - w) / 2, RSSI_VAL_Y);
  lcd.print(buf);
}

void updateBottomCell(int i, float newVal, float oldVal, uint16_t color) {
  if (newVal == oldVal) return;
  char buf[16];
  dtostrf(newVal, 5, 1, buf);
  int x = i * COL_W;
  lcd.fillRect(x + 4, BOT_VAL_Y, COL_W - 8, 14, C_BG);
  lcd.setTextSize(2); lcd.setTextColor(color);
  lcd.setCursor(x + 4, BOT_VAL_Y);
  lcd.print(buf);
}

void updateBottomRow() {
  updateBottomCell(0, cur.altitude, prev.altitude, C_WHITE);
  updateBottomCell(1, cur.apogee,   prev.apogee,   C_CYAN);
  updateBottomCell(2, cur.accel,    prev.accel,     C_WHITE);
  updateBottomCell(3, cur.speed,    prev.speed,     C_WHITE);
}

void updateStaleBanner(bool stale) {
  if (stale == wasStale && !firstDraw) return;
  wasStale = stale;
  lcd.fillRect(0, BANNER_Y, 320, BANNER_H, C_BG);
  if (stale) {
    lcd.fillRect(0, BANNER_Y, 320, BANNER_H, C_RED);
    lcd.setTextSize(1); lcd.setTextColor(C_WHITE);
    lcd.setCursor(60, BANNER_Y + 8);
    lcd.print("!! DATA STALE - NO SIGNAL !!");
  } else {
    lcd.fillRect(0, BANNER_Y, 320, BANNER_H, C_DKGREEN);
    lcd.setTextSize(1); lcd.setTextColor(C_GREEN);
    lcd.setCursor(80, BANNER_Y + 8);
    lcd.print("DATA LIVE");
  }
}

void setup() {
  Serial.begin(115200);
  pinMode(LCD_BL, OUTPUT);
  analogWrite(LCD_BL, 255);
  lcd.init(240, 320);
  lcd.setRotation(3);
  lcd.fillScreen(C_BG);
  drawStaticChrome();
  lastPacketMs = millis();
  lastSimMs    = millis();
  firstDraw = true;
  prev.rssi = cur.rssi + 1;
  prev.lat  = cur.lat  + 1;
  updateGPS();
  updateRSSI();
  updateBottomRow();
  updateStaleBanner(false);
  firstDraw = false;
}

void loop() {
  uint32_t now = millis();
  if (Serial.available()) {
    // parseIncoming();
    lastPacketMs = now;
  }
  if (now - lastSimMs >= SIM_INTERVAL_MS) {
    lastSimMs = now;
    simulatePacket();
  }
  bool stale = (now - lastPacketMs) > STALE_TIMEOUT_MS;
  updateStaleBanner(stale);
  updateGPS();
  updateRSSI();
  updateBottomRow();
  prev = cur;
}
