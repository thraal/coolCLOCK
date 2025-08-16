# coolCLOCK

## Essence

- DIY Wi-Fi connected clock using an ESP32-C6 and a 32×8 WS2812B LED matrix  
- Dual purpose: **Clock** (time via NTP) and **Lamp** (solid/ambient colors)  
- Optional **Party Mode** with animated effects and transitions  
- **Buttons** for switching modes, colors, brightness, and party toggles  
- **Persistent settings** stored on ESP32 (brightness, colors, modes)  

---

## Requirements

### Hardware

- ESP32-C6-WROOM-1 development board  
- WS2812B 32 × 8 LED matrix (256 LEDs)  
- Stable 5 V power supply (**YwRobot 545043** module, shared between ESP32 and LED matrix)  
- 330-470 Ω resistor in series with LED DIN  
- ≥1000 µF capacitor across 5 V / GND near LED matrix  
- 1-2 push buttons with pull-ups  
- USB cable for flashing and debugging  
- Optional: 3D printed case or diffuser for softer visuals  

### Software

- [Arduino IDE](https://www.arduino.cc/en/software) or PlatformIO  
- ESP32 board support (Espressif)  
- Libraries:  
  - [FastLED](https://github.com/FastLED/FastLED)  
  - `WiFi.h`, `time.h`, `esp_sntp.h` (ESP32 core)  
  - `Preferences.h` (ESP32 NVS storage)  

---

## Hardware Wiring

The setup uses a **YwRobot 545043** regulated power supply to provide **5 V** for both the WS2812B LED matrix and the ESP32-C6 board.

### Power

- **YwRobot 5V → WS2812B 5V**  
- **YwRobot 5V → ESP32 5V (VIN)**  
- **YwRobot GND → WS2812B GND → ESP32 GND**  

Add:  

- **≥1000 µF capacitor** across LED 5 V and GND  
- **330-470 Ω resistor** in series with WS2812B DIN  

### Data

- **ESP32-C6 GPIO2 → WS2812B DIN**  

### Buttons (planned)

- **Button A → GPIO8 → GND** (with internal pull-up)  
- **Button B → GPIO9 → GND** (with internal pull-up)  

### ASCII Wiring Diagram

```text
       +--------------------------+
       |   YwRobot 545043 PSU     |
       |   5V / GND regulated     |
       +-----------+--------------+
                   |
     +-------------+-----------------+
     |                               |
 +---v---+                     +-----v------+
\| ESP32 |                     | WS2812B    |
\|  C6   |                     | 32x8 Matrix|
\|       |                     |            |
\|    5V +---------------------+ 5V         |
\|   GND +---------------------+ GND        |
\| GPIO2 +---------------------+ DIN        |
\|       |                     |            |
\| GPIO8 <-- Button A --> GND  |            |
\| GPIO9 <-- Button B --> GND  |            |
\|       |                     |            |
 +-------+                     +------------+

(Capacitor ≥1000 µF across 5V/GND near LED matrix)
(Resistor 330-470 Ω in series with DIN line)

```

### Notes

- Keep LED data line short and with resistor close to DIN.  
- Power supply sizing:  
  - Full white, 100% = ~15 A (not practical).  
  - Firmware limits brightness and current (`FastLED.setMaxPowerInVoltsAndMilliamps`).  
  - At reduced brightness (typical 1/8 to 1/4), the YwRobot module is sufficient.  
- Power LED strip **before or at same time** as ESP32 to prevent backfeeding.  

### Pinout Table

| Function        | ESP32-C6 Pin | Connected To           | Notes                                |
|-----------------|--------------|------------------------|--------------------------------------|
| LED Data        | GPIO2        | WS2812B DIN            | With 330-470 Ω series resistor       |
| LED Power       | VIN / 5V     | WS2812B 5V (from PSU)  | Shared 5 V from YwRobot 545043       |
| LED Ground      | GND          | WS2812B GND            | Common ground with PSU               |
| Button A        | GPIO8        | Push button → GND      | Internal pull-up enabled             |
| Button B        | GPIO9        | Push button → GND      | Internal pull-up enabled             |
| ESP32 Power     | VIN / 5V     | YwRobot 5 V output     | USB also used for flashing/debugging |
| ESP32 Ground    | GND          | YwRobot GND            | Must be shared with LED ground       |

---

## Current Features

- Wi-Fi connection and NTP time sync (CET/CEST timezone with DST)  
- 5×7 pixel font rendering for digits and text  
- Demo cycle of multiple **modes**:  
  - Scan line  
  - HELLO text  
  - Time display (test variants)  
  - Animated effects: sinelon, confetti  
  - Transition effects: sweep, confetti reveal  
- Non-blocking animation loop with timing  

---

## Planned Features

### Clock Mode

- Live time display in multiple **styles** (simple, shadow, etc.)  
- User-selectable text color  
- Party mode: fancy transitions (confetti, wipes, sparkles) on minute changes  
- Automatic periodic NTP re-sync  

### Lamp Mode

- Solid color lamp (warm/cool tones)  
- Gradient or ambient color modes  
- Party mode: playful animations (confetti, rainbow waves, color flows)  
- Adjustable brightness  

### Buttons

- Support for short / long / double press  
- Example mappings:  
  - **Button A short:** toggle Clock ↔ Lamp  
  - **Button A long:** toggle Party mode  
  - **Button A double:** cycle brightness levels  
  - **Button B short:** cycle clock style or lamp effect  
  - **Button B long:** enter color picker  
- Color picker overlay:  
  - Hue bar displayed across screen  
  - Navigate with button B  
  - Confirm with button A  

### Settings

- Persistent storage in ESP32 NVS:  
  - Brightness  
  - Clock color  
  - Lamp color  
  - Selected lamp effect  
  - Party mode enabled/disabled  
- Power limiting using FastLED (`setMaxPowerInVoltsAndMilliamps`)  

---

## Roadmap / Milestones

1. **MVP Clock**  
   - Stable Wi-Fi + NTP time sync  
   - Time display with basic font  
   - Brightness limiter  

2. **Input Handling**  
   - Button reading (short/long/double press)  
   - Mode switching (Clock ↔ Lamp)  

3. **Lamp Mode**  
   - Solid and gradient color modes  
   - Brightness adjustment  
   - Color picker overlay  

4. **Party Mode**  
   - Clock: transitions on minute changes  
   - Lamp: confetti and animated color flows  

5. **Persistence**  
   - Save settings in NVS  
   - Restore on boot  

6. **Quality of Life**  
   - OTA firmware updates  
   - Wi-Fi fallback AP config  
   - Optional: Web interface or MQTT integration  

---

## Status

Currently experimenting with:

- Text rendering on the LED matrix  
- Animated transitions (sweep, confetti reveal)  
- Test modes and demo effects  
- Wi-Fi + NTP setup on ESP32-C6  

Next up:  

- Button wiring and input handling  
- First working Clock ↔ Lamp toggle  
- Persistence of brightness and colors  
