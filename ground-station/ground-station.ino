// Ground station receiver for rocket-os telemetry.
// Dependencies (Arduino Library Manager):
//   - RadioLib
//   - Adafruit GFX Library
//   - Adafruit ST7735 and ST7789 Library

#include <SPI.h>
#include <RadioLib.h>
#include <Adafruit_GFX.h>
#include <Adafruit_ST7789.h>

// --- LCD wiring (ST7789 240x320, software SPI) ---
#define LCD_MOSI 23
#define LCD_SCLK 22
#define LCD_CS   17
#define LCD_DC   18
#define LCD_RST  19
#define LCD_BL   21

// --- SX1262 wiring on ground-station ESP32 (HSPI). Verify against your board. ---
#define SX_SCK   14
#define SX_MISO  12
#define SX_MOSI  13
#define SX_NSS   15
#define SX_DIO1  27
#define SX_NRST  26
#define SX_BUSY  25

#define RF_FREQ_MHZ 915.0

// Must match the rocket's ESP-IDF firmware (main/tasks/transmission.h).
#define CALLSIGN_ID 0x88
#define TELEM_TYPE_SENSOR 0x01
#define TELEM_TYPE_GPS    0x02

#define STALE_TIMEOUT_MS 2000

// Mirror the rocket's packed telemetry structs exactly.
// packet_type is uint32_t on the wire because ESP-IDF GCC does not use
// -fshort-enums, so the enum inside the packed struct occupies 4 bytes.
typedef struct __attribute__((packed)) {
  uint8_t  callsign_id;
  uint16_t seq;
  uint32_t packet_type;
  uint32_t timestamp_ms;
  int16_t  altitude;
  uint16_t heading;
  int8_t   temperature;
  int16_t  speed;
  int16_t  acceleration;
} sensor_telemetry_packet_t;

typedef struct __attribute__((packed)) {
  uint8_t  callsign_id;
  uint16_t seq;
  uint32_t packet_type;
  uint32_t timestamp_ms;
  int32_t  latitude;
  int32_t  longitude;
  int16_t  altitude;
  uint8_t  satellites_tracked;
  uint8_t  fix_quality;
} gps_telemetry_packet_t;

// --- Display state ---
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
  float   lat      = 0.0f;
  float   lon      = 0.0f;
  uint8_t fixType  = 0;
  uint8_t numSats  = 0;
  int16_t rssi     = -127;
  float   altitude = 0.0f;
  float   apogee   = 0.0f;
  float   accel    = 0.0f;
  float   speed    = 0.0f;
};

Telemetry cur, prev;

uint32_t lastPacketMs = 0;
bool     wasStale     = true;
bool     firstDraw    = true;
bool     haveGPS      = false;
bool     haveSensor   = false;

// --- Radio state ---
SPIClass radioSPI(HSPI);
SX1262 radio = new Module(SX_NSS, SX_DIO1, SX_NRST, SX_BUSY, radioSPI);

volatile bool rxFlag = false;
void IRAM_ATTR onRadioIrq() { rxFlag = true; }

// --- UI layout constants (matches reference sketch) ---
#define GPS_VAL_X    4
#define GPS_VAL_Y    16
#define FIX_X        250
#define FIX_VAL_Y    16
#define RSSI_VAL_Y   80
#define BOT_Y        152
#define BOT_H        88
#define BOT_LABEL_Y  156
#define BOT_VAL_Y    170
#define BOT_UNIT_Y   196
#define COL_W        80
#define BANNER_Y     215
#define BANNER_H     25

static void redrawField(uint16_t bgColor, int16_t x, int16_t y,
                        uint8_t textSize, uint16_t color,
                        const char* str, uint16_t eraseW, uint16_t eraseH) {
  lcd.fillRect(x, y, eraseW, eraseH, bgColor);
  lcd.setTextSize(textSize);
  lcd.setTextColor(color);
  lcd.setCursor(x, y);
  lcd.print(str);
}

static void drawStaticChrome() {
  lcd.fillScreen(C_BG);
  lcd.setTextSize(1); lcd.setTextColor(C_GRAY);
  lcd.setCursor(4, 4);     lcd.print("GPS POSITION");
  lcd.setCursor(FIX_X, 4); lcd.print("FIX / SATS");
  lcd.drawFastHLine(0, 46, 320, C_GRAY);
  lcd.setCursor(4, 52);    lcd.print("RSSI");
  lcd.drawFastHLine(0, 150, 320, C_GRAY);
  const char* labels[4] = { "ALT", "APOGEE", "ACCEL", "SPEED" };
  const char* units[4]  = { "m",   "m",      "g",     "m/s"  };
  for (int i = 0; i < 4; i++) {
    int x = i * COL_W;
    lcd.setTextSize(1); lcd.setTextColor(C_GRAY);
    lcd.setCursor(x + 4, BOT_LABEL_Y); lcd.print(labels[i]);
    lcd.setCursor(x + 4, BOT_UNIT_Y);  lcd.print(units[i]);
    if (i < 3) lcd.drawFastVLine(x + COL_W - 1, BOT_Y, BOT_H, C_GRAY);
  }
}

static void updateGPS() {
  bool latChanged = (cur.lat != prev.lat || cur.lon != prev.lon);
  bool fixChanged = (cur.fixType != prev.fixType || cur.numSats != prev.numSats);

  if (latChanged || firstDraw) {
    char buf[40], latBuf[10], lonBuf[10];
    dtostrf(fabs(cur.lat), 7, 4, latBuf);
    dtostrf(fabs(cur.lon), 7, 4, lonBuf);
    snprintf(buf, sizeof(buf), "%s%s %s%s",
             latBuf, (cur.lat >= 0 ? " N " : " S "),
             lonBuf, (cur.lon >= 0 ? " E"  : " W"));
    redrawField(C_BG, GPS_VAL_X, GPS_VAL_Y, 2, C_WHITE, buf, 240, 16);
  }

  if (fixChanged || firstDraw) {
    uint16_t fixColor = (cur.fixType >= 3) ? C_GREEN :
                        (cur.fixType == 2) ? C_ORANGE : C_RED;
    char buf[8];
    if      (cur.fixType >= 3) strcpy(buf, "3D");
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

static void updateRSSI() {
  if (cur.rssi == prev.rssi && !firstDraw) return;
  char buf[16];
  snprintf(buf, sizeof(buf), "%d dBm", cur.rssi);
  uint16_t rssiColor = (cur.rssi > -80)  ? C_YELLOW :
                       (cur.rssi > -100) ? C_ORANGE : C_RED;
  lcd.fillRect(0, RSSI_VAL_Y, 320, 32, C_BG);
  lcd.setTextSize(4); lcd.setTextColor(rssiColor);
  int16_t x1, y1; uint16_t w, h;
  lcd.getTextBounds(buf, 0, 0, &x1, &y1, &w, &h);
  lcd.setCursor((320 - w) / 2, RSSI_VAL_Y);
  lcd.print(buf);
}

static void updateBottomCell(int i, float newVal, float oldVal, uint16_t color) {
  if (newVal == oldVal && !firstDraw) return;
  char buf[16];
  dtostrf(newVal, 5, 1, buf);
  int x = i * COL_W;
  lcd.fillRect(x + 4, BOT_VAL_Y, COL_W - 8, 14, C_BG);
  lcd.setTextSize(2); lcd.setTextColor(color);
  lcd.setCursor(x + 4, BOT_VAL_Y);
  lcd.print(buf);
}

static void updateBottomRow() {
  updateBottomCell(0, cur.altitude, prev.altitude, C_WHITE);
  updateBottomCell(1, cur.apogee,   prev.apogee,   C_CYAN);
  updateBottomCell(2, cur.accel,    prev.accel,    C_WHITE);
  updateBottomCell(3, cur.speed,    prev.speed,    C_WHITE);
}

static void updateStaleBanner(bool stale) {
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

// Decode and merge an incoming packet into `cur`. Returns true if accepted.
static bool processPacket(const uint8_t* buf, size_t len) {
  if (len < 1 + 2 + 4) return false;
  if (buf[0] != CALLSIGN_ID) return false;

  uint32_t packet_type;
  memcpy(&packet_type, buf + 3, sizeof(packet_type));

  if (packet_type == TELEM_TYPE_SENSOR && len >= sizeof(sensor_telemetry_packet_t)) {
    sensor_telemetry_packet_t p;
    memcpy(&p, buf, sizeof(p));

    cur.altitude = (float)p.altitude;
    cur.speed    = (float)p.speed;
    // ADXL345 in ±16G full-resolution mode: 256 LSB/g (4 mg/LSB).
    cur.accel    = (float)p.acceleration / 256.0f;
    if (cur.altitude > cur.apogee) cur.apogee = cur.altitude;
    haveSensor = true;
    return true;
  }

  if (packet_type == TELEM_TYPE_GPS && len >= sizeof(gps_telemetry_packet_t)) {
    gps_telemetry_packet_t p;
    memcpy(&p, buf, sizeof(p));

    cur.lat     = (float)p.latitude  / 1000000.0f;
    cur.lon     = (float)p.longitude / 1000000.0f;
    cur.numSats = p.satellites_tracked;
    // fix_quality high nibble is fix type per rocket firmware comment.
    cur.fixType = (p.fix_quality >> 4) & 0x0F;
    if (cur.fixType == 0) cur.fixType = p.fix_quality & 0x0F;
    haveGPS = true;
    // GPS altitude is a useful fallback before sensor fusion is running.
    if (!haveSensor) {
      cur.altitude = (float)p.altitude;
      if (cur.altitude > cur.apogee) cur.apogee = cur.altitude;
    }
    return true;
  }

  return false;
}

static void initRadio() {
  Serial.println("[radio] initializing SPI...");
  radioSPI.begin(SX_SCK, SX_MISO, SX_MOSI, SX_NSS);

  // Modulation parameters must match rocket-os/main:
  //   SF=7, BW=125 kHz (0x04), CR=4/5 (0x01), LDRO=off.
  // Packet parameters: preamble=12, explicit header, CRC on, IQ not inverted.
  // Sync word: rocket never sets one, so the chip stays at reset default 0x1424,
  // which corresponds to RadioLib's PRIVATE.
  // TCXO voltage=0 disables TCXO — rocket's sx1262.c does not call
  // SetDIO3AsTCXOCtrl, so it runs off the 32 MHz XTAL. RadioLib's default
  // (1.6 V) would enable TCXO and break reception on an XTAL-only module.
  Serial.printf("[radio] begin: %.1f MHz, BW=125k, SF=7, CR=4/5\n", RF_FREQ_MHZ);
  int state = radio.begin(
      RF_FREQ_MHZ, 125.0, 7, 5,
      RADIOLIB_SX126X_SYNC_WORD_PRIVATE,
      22,       // TX power (unused for RX)
      12,       // preamble length
      0.0f,     // tcxoVoltage: 0 = XTAL, match rocket
      false);   // useRegulatorLDO: default DC-DC
  if (state != RADIOLIB_ERR_NONE) {
    Serial.printf("[radio] SX1262 begin FAILED: %d\n", state);
    while (true) { delay(1000); }
  }
  Serial.println("[radio] begin OK");

  state = radio.setCRC(true);
  if (state != RADIOLIB_ERR_NONE) Serial.printf("[radio] setCRC failed: %d\n", state);

  // Most SX1262 breakouts route DIO2 to an RF T/R switch. The rocket leaves
  // it unconfigured (its switch apparently defaults to TX). On the receiver
  // we need the switch in RX, which the chip will drive automatically when
  // this is enabled. Safe no-op if DIO2 isn't wired to a switch.
  state = radio.setDio2AsRfSwitch(true);
  if (state != RADIOLIB_ERR_NONE) Serial.printf("[radio] setDio2AsRfSwitch failed: %d\n", state);

  radio.setDio1Action(onRadioIrq);
  state = radio.startReceive();
  if (state != RADIOLIB_ERR_NONE) {
    Serial.printf("[radio] startReceive FAILED: %d\n", state);
  } else {
    Serial.println("[radio] listening...");
  }
}

void setup() {
  Serial.begin(115200);

  pinMode(LCD_BL, OUTPUT);
  analogWrite(LCD_BL, 255);
  lcd.init(240, 320);
  lcd.setRotation(3);
  drawStaticChrome();
  firstDraw = true;
  updateGPS();
  updateRSSI();
  updateBottomRow();
  updateStaleBanner(true);
  firstDraw = false;

  initRadio();
  lastPacketMs = millis() - STALE_TIMEOUT_MS;
}

void loop() {
  if (rxFlag) {
    rxFlag = false;

    size_t len = radio.getPacketLength();
    uint8_t buf[64];
    if (len > sizeof(buf)) len = sizeof(buf);

    int state = radio.readData(buf, len);
    if (state == RADIOLIB_ERR_NONE) {
      float rssi = radio.getRSSI();
      float snr = radio.getSNR();
      Serial.printf("[rx] %u bytes  RSSI=%.1f dBm  SNR=%.1f dB  hdr=0x%02X type=%u\n",
                    (unsigned)len, rssi, snr,
                    len > 0 ? buf[0] : 0,
                    len >= 7 ? *(uint32_t*)(buf + 3) : 0);
      Serial.print("[rx] raw:");
      for (size_t i = 0; i < len; i++) Serial.printf(" %02X", buf[i]);
      Serial.println();

      if (processPacket(buf, len)) {
        cur.rssi = (int16_t)rssi;
        lastPacketMs = millis();
        Serial.println("[rx] packet accepted");
      } else {
        Serial.println("[rx] packet rejected (callsign/type/length mismatch)");
      }
    } else if (state == RADIOLIB_ERR_CRC_MISMATCH) {
      Serial.println("[rx] CRC mismatch");
    } else {
      Serial.printf("[rx] readData failed: %d\n", state);
    }

    radio.startReceive();
  }

  static uint32_t lastHeartbeat = 0;
  if (millis() - lastHeartbeat > 5000) {
    lastHeartbeat = millis();
    uint32_t since = millis() - lastPacketMs;
    Serial.printf("[hb] alive, %u ms since last accepted packet, RSSI floor=%.1f\n",
                  since, radio.getRSSI());
  }

  bool stale = (millis() - lastPacketMs) > STALE_TIMEOUT_MS;
  updateStaleBanner(stale);
  updateGPS();
  updateRSSI();
  updateBottomRow();
  prev = cur;
}
