#include <FastLED.h>
#include <WiFi.h>
#include "time.h"
#include "esp_sntp.h"

// ---------- Wi-Fi & Time ----------
const char *ssid = "CoolHouse";
const char *password = "w6HSB2S3bb043!";

const char *ntpServer1 = "pool.ntp.org";
const char *ntpServer2 = "time.nist.gov";
const char *time_zone  = "CET-1CEST,M3.5.0,M10.5.0/3"; // Europe/Brussels-like TZ with DST

// ---------- Matrix ----------
#define DATA_PIN 2
#define MATRIX_WIDTH  32
#define MATRIX_HEIGHT 8
#define COLOR_ORDER   GRB
#define MATRIX_TYPE   WS2812B
#define MATRIX_BRIGHTNESS 10
#define NUM_LEDS (MATRIX_WIDTH * MATRIX_HEIGHT)
CRGB leds[NUM_LEDS];

// ---------- Modes ----------
enum Mode {
  MODE_SCAN, MODE_HELLO, MODE_TIME1, MODE_TIME2, MODE_TIME3,
  MODE_SINELON, MODE_CONFETTI,
  MODE_TRANSITION,              // sweep transition (TIME1 -> TIME2)
  MODE_CONFETTI_REVEAL          // confetti reveal transition (TIME2 -> TIME3)
};
Mode mode = MODE_SCAN;
unsigned long modeStart = 0;
uint8_t gHue = 0;

// ---- Transition buffers & state ----
CRGB fromBuf[NUM_LEDS];
CRGB toBuf[NUM_LEDS];

Mode      transitionNext = MODE_SCAN;
unsigned long transitionStart = 0;
const uint16_t TRANSITION_MS = 800;   // duration for sweep transition

// Confetti reveal state
CRGB currBuf[NUM_LEDS];               // starts as fromBuf, evolves toward toBuf
uint8_t flashed[NUM_LEDS];            // 0 = not yet hit by confetti, 1 = hit
uint16_t flashedCount = 0;
const uint8_t CONFETTI_FLIPS_PER_FRAME = 1;  // how many new pixels to flip each frame

// ---------- Mapping ----------
static inline uint16_t XY(uint8_t x, uint8_t y) {
  uint8_t col_from_right = (MATRIX_WIDTH - 1) - x;
  uint16_t base = (uint16_t)col_from_right * MATRIX_HEIGHT;
  return (col_from_right & 1) ? base + (MATRIX_HEIGHT - 1 - y) : base + y;
}

// ---------- Font ----------
#define __ 0
#define X  1
#define _  0
#define ROW5(a,b,c,d,e) ((uint8_t)((a<<4)|(b<<3)|(c<<2)|(d<<1)|(e)))

#define CH_H 0
#define CH_E 1
#define CH_L 2
#define CH_O 3
#define CH_0 4
#define CH_1 5
#define CH_2 6
#define CH_3 7
#define CH_4 8
#define CH_5 9
#define CH_6 10
#define CH_7 11
#define CH_8 12
#define CH_9 13
#define CH_COLON 14

const uint8_t font5x7_rows[][7] = {
  {ROW5(X,_,_,_,X), ROW5(X,_,_,_,X), ROW5(X,_,_,_,X), ROW5(X,X,X,X,X), ROW5(X,_,_,_,X), ROW5(X,_,_,_,X), ROW5(X,_,_,_,X)},
  {ROW5(X,X,X,X,X), ROW5(X,_,_,_,_), ROW5(X,_,_,_,_), ROW5(X,X,X,_,_), ROW5(X,_,_,_,_), ROW5(X,_,_,_,_), ROW5(X,X,X,X,X)},
  {ROW5(X,_,_,_,_), ROW5(X,_,_,_,_), ROW5(X,_,_,_,_), ROW5(X,_,_,_,_), ROW5(X,_,_,_,_), ROW5(X,_,_,_,_), ROW5(X,X,X,X,X)},
  {ROW5(_,X,X,X,_), ROW5(X,_,_,_,X), ROW5(X,_,_,_,X), ROW5(X,_,_,_,X), ROW5(X,_,_,_,X), ROW5(X,_,_,_,X), ROW5(_,X,X,X,_)},

  {ROW5(_,X,X,X,_), ROW5(X,_,_,_,X), ROW5(X,_,_,_,X), ROW5(X,_,_,_,X), ROW5(X,_,_,_,X), ROW5(X,_,_,_,X), ROW5(_,X,X,X,_)},
  {ROW5(__,_,X,_,__), ROW5(_,X,X,_,_), ROW5(__,_,X,_,__), ROW5(__,_,X,_,__), ROW5(__,_,X,_,__), ROW5(__,_,X,_,__), ROW5(_,X,X,X,_)},
  {ROW5(_,X,X,X,_), ROW5(X,_,_,_,X), ROW5(__,_,_,_,X), ROW5(__,_,_,X,_), ROW5(__,_,X,_,__), ROW5(_,X,_,_,__), ROW5(X,X,X,X,X)},
  {ROW5(X,X,X,X,_), ROW5(__,_,_,_,X), ROW5(__,_,_,_,X), ROW5(_,X,X,X,_), ROW5(__,_,_,_,X), ROW5(__,_,_,_,X), ROW5(X,X,X,X,_)},

  {ROW5(__,_,_,X,_), ROW5(__,_,X,X,_), ROW5(__,X,_,X,_), ROW5(X,_,_,X,_), ROW5(X,X,X,X,X), ROW5(__,_,_,X,_), ROW5(__,_,_,X,_)},
  {ROW5(X,X,X,X,X), ROW5(X,_,_,_,_), ROW5(X,X,X,X,_), ROW5(__,_,_,_,X), ROW5(__,_,_,_,X), ROW5(X,_,_,_,X), ROW5(_,X,X,X,_)},
  {ROW5(__,_,X,X,_), ROW5(__,X,_,_,_), ROW5(X,_,_,_,_), ROW5(X,X,X,X,_), ROW5(X,_,_,_,X), ROW5(X,_,_,_,X), ROW5(_,X,X,X,_)},
  {ROW5(X,X,X,X,X), ROW5(__,_,_,_,X), ROW5(__,_,_,X,_), ROW5(__,_,X,_,__), ROW5(__,X,_,_,__), ROW5(__,X,_,_,__), ROW5(__,X,_,_,__)},

  {ROW5(_,X,X,X,_), ROW5(X,_,_,_,X), ROW5(X,_,_,_,X), ROW5(_,X,X,X,_), ROW5(X,_,_,_,X), ROW5(X,_,_,_,X), ROW5(_,X,X,X,_)},
  {ROW5(_,X,X,X,_), ROW5(X,_,_,_,X), ROW5(X,_,_,_,X), ROW5(_,X,X,X,X), ROW5(__,_,_,_,X), ROW5(__,_,_,X,_), ROW5(_,X,X,_,_)},
  {ROW5(__,_,_,_,__), ROW5(__,_,_,_,__), ROW5(__,_,X,_,__), ROW5(__,_,_,_,__), ROW5(__,_,X,_,__), ROW5(__,_,_,_,__), ROW5(__,_,_,_,__)},
};

static inline uint8_t glyphWidth(uint8_t g) { return (g == CH_COLON) ? 3 : 5; }
static inline int8_t  glyphIndex(char c) {
  switch (c) {
    case 'H': return CH_H; case 'E': return CH_E; case 'L': return CH_L; case 'O': return CH_O;
    case '0': return CH_0; case '1': return CH_1; case '2': return CH_2; case '3': return CH_3; case '4': return CH_4;
    case '5': return CH_5; case '6': return CH_6; case '7': return CH_7; case '8': return CH_8; case '9': return CH_9;
    case ':': return CH_COLON; default: return -1;
  }
}
static inline void drawChar5x7(uint8_t x0, uint8_t y0, uint8_t g, const CRGB &color) {
  for (uint8_t r = 0; r < 7; r++) {
    uint8_t bits = font5x7_rows[g][r];
    for (uint8_t c = 0; c < 5; c++) {
      if (bits & (1 << (4 - c))) {
        uint8_t x = x0 + c, y = y0 + (6 - r);
        if (x < MATRIX_WIDTH && y < MATRIX_HEIGHT) leds[XY(x, y)] = color;
      }
    }
  }
}
static inline uint8_t textWidth(const char *s) {
  uint8_t w = 0, n = 0;
  for (const char *p = s; *p; ++p) { int8_t gi = glyphIndex(*p); w += (gi >= 0 ? glyphWidth(gi) : 3); n++; }
  if (n > 1) w += (n - 1);
  return w;
}
static inline void drawText(const char *s, uint8_t y0, const CRGB &color) {
  int16_t x = 0; uint8_t w = textWidth(s);
  if (MATRIX_WIDTH > w) x = (MATRIX_WIDTH - w) / 2;
  for (const char *p = s; *p; ++p) {
    int8_t gi = glyphIndex(*p);
    uint8_t adv = (gi >= 0 ? glyphWidth(gi) : 3);
    if (gi >= 0) drawChar5x7(x, y0, gi, color);
    x += adv + 1;
  }
}

// ---- Draw into a TARGET buffer (for transitions) ----
static inline void drawChar5x7To(CRGB* target, uint8_t x0, uint8_t y0, uint8_t g, const CRGB& color) {
  for (uint8_t r = 0; r < 7; r++) {
    uint8_t bits = font5x7_rows[g][r];
    for (uint8_t c = 0; c < 5; c++) {
      if (bits & (1 << (4 - c))) {
        uint8_t x = x0 + c, y = y0 + (6 - r);
        if (x < MATRIX_WIDTH && y < MATRIX_HEIGHT) target[XY(x, y)] = color;
      }
    }
  }
}
static inline void drawTextTo(CRGB* target, const char* s, uint8_t y0, const CRGB& color) {
  fill_solid(target, NUM_LEDS, CRGB::Black);
  int16_t x = 0; uint8_t w = textWidth(s);
  if (MATRIX_WIDTH > w) x = (MATRIX_WIDTH - w) / 2;
  for (const char* p = s; *p; ++p) {
    int8_t gi = glyphIndex(*p);
    uint8_t adv = (gi >= 0 ? glyphWidth(gi) : 3);
    if (gi >= 0) drawChar5x7To(target, x, y0, gi, color);
    x += adv + 1;
  }
}

// Sweep transition (TIME1 -> TIME2)
static inline void startTransition(const char* fromStr, const CRGB& fromColor,
                                   const char* toStr,   const CRGB& toColor,
                                   Mode nextMode) {
  drawTextTo(fromBuf, fromStr, 0, fromColor);
  drawTextTo(toBuf,   toStr,   0, toColor);
  transitionNext  = nextMode;
  transitionStart = millis();
  mode = MODE_TRANSITION;
}

// Confetti reveal transition (TIME2 -> TIME3)
static inline void startConfettiReveal(const char* fromStr, const CRGB& fromColor,
                                       const char* toStr,   const CRGB& toColor,
                                       Mode nextMode) {
  drawTextTo(fromBuf, fromStr, 0, fromColor);
  drawTextTo(toBuf,   toStr,   0, toColor);
  // currBuf begins as the old image
  for (uint16_t i = 0; i < NUM_LEDS; i++) {
    currBuf[i] = fromBuf[i];
    flashed[i] = 0;
  }
  flashedCount   = 0;
  transitionNext = nextMode;
  modeStart      = millis(); // reuse for any timing if desired
  mode           = MODE_CONFETTI_REVEAL;
}

// ---------- Effects ----------
static inline void sinelon() {
  fadeToBlackBy(leds, NUM_LEDS, 20);
  int pos = beatsin16(13, 0, NUM_LEDS - 1);
  leds[pos] += CHSV(gHue, 255, 192);
}
static inline void confetti() {
  fadeToBlackBy(leds, NUM_LEDS, 10);
  leds[random16(NUM_LEDS)] += CHSV(gHue + random8(64), 200, 255);
}

// ---------- Scan (non-blocking) ----------
uint8_t  scanRow = 0, scanStep = 0;
uint16_t prevIdx = 0;
unsigned long lastScanStepMs = 0;
const uint16_t SCAN_INTERVAL_MS = 40;

// ---------- Modes ----------
static void enterMode(Mode m) {
  mode = m;
  modeStart = millis();
  switch (mode) {
    case MODE_SCAN:
      fill_solid(leds, NUM_LEDS, CRGB::Black);
      scanRow = 0; scanStep = 0; prevIdx = XY(0, 0); lastScanStepMs = 0;
      break;

    case MODE_HELLO:
      fill_solid(leds, NUM_LEDS, CRGB::Black);
      drawText("HELLO", 0, CRGB::Green);
      break;

    case MODE_TIME1: {
      fill_solid(leds, NUM_LEDS, CRGB::Black);
      struct tm ti;
      if (getLocalTime(&ti)) {
        char buf[6]; // "HH:MM"
        strftime(buf, sizeof(buf), "%H:%M", &ti);
        drawText(buf, 0, CRGB::Blue);
      } else {
        drawText("NOCLK", 0, CRGB::Red);
      }
      break;
    }

    case MODE_TIME2:
      fill_solid(leds, NUM_LEDS, CRGB::Black);
      drawText("45:67", 0, CRGB::Orange);
      break;

    case MODE_TIME3:
      fill_solid(leds, NUM_LEDS, CRGB::Black);
      drawText("89:00", 0, CRGB::Purple);
      break;

    case MODE_SINELON:
    case MODE_CONFETTI:
    case MODE_TRANSITION:
    case MODE_CONFETTI_REVEAL:
      // drawn per-frame in loop()
      break;
  }
}

// ---------- Time helpers ----------
void printLocalTime() {
  struct tm ti;
  if (!getLocalTime(&ti)) { Serial.println("No time available (yet)"); return; }
  Serial.println(&ti, "%A, %B %d %Y %H:%M:%S");
}
void timeavailable(struct timeval* /*t*/) {
  Serial.println("Got time adjustment from NTP!");
  printLocalTime();
}

void setup() {
  // --- Serial first (C6) ---
  Serial.begin(115200);
  delay(100);
  Serial.println("\nBooting...");

  // --- LEDs ---
  FastLED.addLeds<MATRIX_TYPE, DATA_PIN, COLOR_ORDER>(leds, NUM_LEDS);
  FastLED.setBrightness(MATRIX_BRIGHTNESS);
  fill_solid(leds, NUM_LEDS, CRGB::Black);
  FastLED.show();
  enterMode(MODE_SCAN);

  // --- Bring up lwIP/netif before any lwIP-related API ---
  WiFi.mode(WIFI_STA);

  // SNTP config (order matters on C6)
  sntp_set_time_sync_notification_cb(timeavailable); // callback on sync
  sntp_set_sync_mode(SNTP_SYNC_MODE_IMMED);          // apply time immediately
  esp_sntp_servermode_dhcp(1);                       // enable DHCP option 42 (safe now)

  // --- Wi-Fi connect ---
  Serial.printf("Connecting to %s", ssid);
  WiFi.begin(ssid, password);

  const unsigned long connectTimeout = 20000; // 20s
  unsigned long t0 = millis();
  while (WiFi.status() != WL_CONNECTED && (millis() - t0) < connectTimeout) {
    delay(250);
    Serial.print('.');
  }
  if (WiFi.status() == WL_CONNECTED) {
    Serial.printf("\nWiFi connected, IP: %s\n", WiFi.localIP().toString().c_str());
  } else {
    Serial.println("\nWiFi connect timeout; continuing without network");
  }

  // --- Time zone + NTP servers (preferred API) ---
  configTzTime(time_zone, ntpServer1, ntpServer2);

  // First print may say "No time available" until SNTP sync completes
  printLocalTime();
}

void loop() {
  bool updated = false;
  unsigned long now = millis();

  // Hue drift for animated effects
  EVERY_N_MILLISECONDS(20) { gHue++; }

  // Periodic time log
  EVERY_N_MILLISECONDS(5000) { printLocalTime(); }

  switch (mode) {

    case MODE_SCAN:
      if (now - lastScanStepMs >= SCAN_INTERVAL_MS) {
        lastScanStepMs = now;
        uint8_t col = (scanRow & 1) ? (MATRIX_WIDTH - 1 - scanStep) : scanStep;
        uint16_t idx = XY(col, scanRow);
        leds[prevIdx] = CRGB::Black;
        leds[idx]     = CRGB::Red;
        prevIdx = idx;
        updated = true;
        if (++scanStep >= MATRIX_WIDTH) {
          scanStep = 0;
          if (++scanRow >= MATRIX_HEIGHT) enterMode(MODE_HELLO), updated = true;
        }
      }
      break;

    case MODE_HELLO:
      if (now - modeStart >= 5000UL) { enterMode(MODE_TIME1); updated = true; }
      break;

    case MODE_TIME1:
      if (now - modeStart >= 5000UL) {
        // Sweep transition FROM current time (blue) TO "45:67" (orange)
        struct tm ti; char buf[6] = "??:??";
        if (getLocalTime(&ti)) strftime(buf, sizeof(buf), "%H:%M", &ti);
        startTransition(buf, CRGB::Blue, "45:67", CRGB::Orange, MODE_TIME2);
      }
      break;

    case MODE_TIME2:
      if (now - modeStart >= 5000UL) {
        // Confetti reveal FROM "45:67" (orange) TO "89:00" (purple)
        startConfettiReveal("45:67", CRGB::Orange, "89:00", CRGB::Purple, MODE_TIME3);
      }
      break;

    case MODE_TIME3:
      if (now - modeStart >= 5000UL) { enterMode(MODE_SINELON); updated = true; }
      break;

    case MODE_SINELON:
      sinelon(); updated = true;
      if (now - modeStart >= 5000UL) { enterMode(MODE_CONFETTI); updated = true; }
      break;

    case MODE_CONFETTI:
      confetti(); updated = true;
      if (now - modeStart >= 5000UL) { enterMode(MODE_SCAN); updated = true; }
      break;

    // ---- Sweep transition composer ----
    case MODE_TRANSITION: {
      float t = (millis() - transitionStart) / float(TRANSITION_MS);
      if (t > 1.0f) t = 1.0f;
      float tt = t * t * (3.0f - 2.0f * t);
      uint8_t sx = uint8_t(tt * (MATRIX_WIDTH - 1));

      for (uint8_t y = 0; y < MATRIX_HEIGHT; y++) {
        for (uint8_t x = 0; x < MATRIX_WIDTH; x++) {
          int i = XY(x, y);
          CRGB base = (x <= sx) ? toBuf[i] : fromBuf[i];
          int dx = abs(int(x) - int(sx));
          uint8_t falloff = (dx == 0) ? 255 : (dx == 1 ? 128 : (dx == 2 ? 64 : 0));
          if (falloff) base += CHSV(gHue, 255, falloff);
          leds[i] = base;
        }
      }
      updated = true;
      if (t >= 1.0f) { enterMode(transitionNext); updated = true; }
      break;
    }

    // ---- Confetti reveal composer ----
    case MODE_CONFETTI_REVEAL: {
      // Start from current image buffer
      for (uint16_t i = 0; i < NUM_LEDS; i++) leds[i] = currBuf[i];

      // Flip a handful of not-yet-flashed pixels
      uint8_t flips = CONFETTI_FLIPS_PER_FRAME;
      uint8_t flashedThisFrame = 0;
      uint16_t attempts = 0;
      while (flips && flashedCount < NUM_LEDS && attempts < NUM_LEDS * 3) {
        attempts++;
        uint16_t j = random16(NUM_LEDS);
        if (flashed[j]) continue;
        flashed[j] = 1;
        flashedCount++;
        flips--;
        flashedThisFrame++;

        // If target has color here, lock it in; otherwise erase to black
        if (toBuf[j]) {
          currBuf[j] = toBuf[j];
        } else {
          currBuf[j] = CRGB::Black;
        }

        // Add a one-frame sparkle pop on this spot
        leds[j] += CHSV(gHue + random8(64), 200, 255);
      }

      // Optional: add a few ephemeral sparkles elsewhere for flavor
      for (uint8_t s = 0; s < 4; s++) {
        uint16_t k = random16(NUM_LEDS);
        if (!flashed[k]) leds[k] += CHSV(gHue + random8(64), 200, 180);
      }

      updated = true;

      // Done when every pixel has been flashed at least once
      if (flashedCount >= NUM_LEDS) {
        // Ensure final image equals the target
        for (uint16_t i = 0; i < NUM_LEDS; i++) currBuf[i] = toBuf[i];
        for (uint16_t i = 0; i < NUM_LEDS; i++) leds[i]    = currBuf[i];
        enterMode(transitionNext); updated = true;
      }
      break;
    }
  }

  if (updated) FastLED.show();
}
