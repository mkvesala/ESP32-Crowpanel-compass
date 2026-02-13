# Claude Session Context - ESP32 CrowPanel Compass

**Date:** 2026-02-11 (updated)
**Project:** ESP32-Crowpanel-compass
**Hardware:** Elecrow CrowPanel 2.1" HMI (ESP32-S3, 480x480 IPS, Rotary Knob)

**ESP-NOW sender:** CMPS14-ESP32-SignalK-gateway compass, source code and documentation available in public repository [mkvesala/CMPS14-ESP32-SignalK-gateway](https://github.com/mkvesala/CMPS14-ESP32-SignalK-gateway/tree/feature/crow-panel-integration) branch `feature/crow-panel-integration`.

---

## Project Overview

MVP implementation of a marine instrument that receives ESP-NOW broadcast messages from CMPS14 compass and displays heading/pitch/roll values with LVGL UI on a round touchscreen.

### Screens

1. **CompassScreen** - Compass rose and heading value
2. **AttitudeScreen** - Artificial horizon (white line) and pitch/roll values
3. **BrightnessScreen** - Display backlight brightness adjustment (NVS persistence)

---

## All Features Implemented

| Feature | Files |
|---------|-------|
| Knob button toggles T/M heading mode | RotaryEncoder, CompassUI, .ino |
| HeadingData simplified (no valid flags) | HeadingData.h |
| PanelConnected: black=connected, red=disconnected | CompassUI.cpp |
| WiFi removed from CrowPanel (ESP-NOW only) | ESPNowReceiver |
| ESP-NOW channel 6 (matches router) | .ino |
| Attitude Level feature (full end-to-end) | AttitudeUI, ESPNowReceiver, ScreenManager, .ino |
| BrightnessScreen (backlight adjustment, NVS persist) | BrightnessUI, ScreenManager, .ino |
| Compass rose rotation threshold 0.5° | CompassUI |
| Diagnostics: PPS, UI timing, LVGL timing, heap, stack | .ino |
| Leveling dialog text wrapping fixed | AttitudeUI.cpp |
| ESPNowReceiver simplified (direct static callback, no _instance pointer) | ESPNowReceiver.h/.cpp |
| Serial.print cleanup (only [DIAG] lines remain) | All own files |

### Attitude Level Feature

**State machine flow:**
1. Knob press → CONFIRM_WAIT (show "Level attitude?\n\nPress knob again\nto confirm." in yellow)
2. Second press → SENDING (show "Leveling..." in white, send ESP-NOW broadcast)
3. Response received → SUCCESS ("Success!" in green) or FAILED ("Failed!" in red)
4. Timeout or screen switch → return to IDLE (dialog hidden)

**Timeouts:** CONFIRM_WAIT 3s, SENDING 3s, SUCCESS 1.5s, FAILED 2s

### BrightnessScreen Feature

**State machine flow:**
1. Knob press → ADJUSTING (show arc overlay, ContainerAdjustment visible)
2. Knob rotation → ±5% brightness (arc, label, and backlight update in real-time)
3. 3 sec timeout after last rotation → save to NVS, hide arc, return to IDLE
4. Screen switch while ADJUSTING → save + cancel (return to IDLE)

**Normal mode (IDLE):** Shows sun icon, "Brightness" label, and current percentage. Knob rotation switches screens normally.

**Brightness range:** 5%–100% (5% minimum prevents screen going completely dark). Default: 78% (~200/255).

**Persistence:** ESP32 Preferences library (NVS), namespace `"display"`, key `"brightness"`. Loaded on boot in `begin()`, saved on auto-save timeout or screen switch.

**PWM mapping:** Linear `percent * 255 / 100`. GPIO 6, LEDC channel 0, 5 kHz, 8-bit.

### Compass Rose Rotation Threshold

CompassUI skips `lv_img_set_angle()` when heading changes < 0.5° (5 x10-units).
Handles 360°→0° wraparound. First render always draws (sentinel 0xFFFF).
Reduces heavy LVGL re-renders: compass rose software rotation takes ~194 ms on ESP32-S3.

### Diagnostics

Three diagnostic lines printed every 5 seconds:
```
[DIAG] PPS: 18.4 | UI updates: 25 | UI avg: 0.57 ms | UI max: 0.72 ms
[DIAG] LVGL calls: 25 | avg: 201.57 ms | max: 206.75 ms
[DIAG] Heap free: 8133839 | min: 8128811 | Stack loop: 4844 | enc: 1312 | btn: 728
```

- **UI updates:** How many times UI draw code ran in 5s window
- **LVGL calls/avg/max:** `lv_timer_handler()` call count and duration — reveals rendering bottleneck
- **Heap free/min:** Current and all-time minimum free heap (leak detection)
- **Stack loop/enc/btn:** Stack high water marks for main loop, encoder task, button task

**Key finding:** Compass rose `lv_img_set_angle()` causes ~194 ms LVGL software re-render per frame (480×480 px). This is the main performance bottleneck on CompassScreen. No practical alternative exists on this hardware (no GPU, no hardware rotation support in GC9A01 display controller).

---

## Architecture

```
┌─────────────────────────────────────────────────────────────┐
│                    Main Loop                                │
├─────────────────────────────────────────────────────────────┤
│  ┌──────────────┐  ┌──────────────┐  ┌──────────────┐      │
│  │RotaryEncoder │  │ESPNowReceiver│  │ScreenManager │      │
│  └──────┬───────┘  └──────┬───────┘  └──────┬───────┘      │
│         │                 │                  │              │
│         ▼                 ▼                  ▼              │
│  ┌────────────────────────────────────────────────┐        │
│  │                 HeadingData                    │        │
│  └────────────────────────────────────────────────┘        │
│         │                                    │              │
│         ▼                                    ▼              │
│  ┌──────────────┐                    ┌──────────────┐      │
│  │  CompassUI   │                    │  AttitudeUI  │      │
│  └──────────────┘                    └──────────────┘      │
└─────────────────────────────────────────────────────────────┘
```

---

## UI Structure (SquareLine Studio)

### CompassScreen
```
ui_CompassScreen
├── ui_PanelTop (white background)
│   ├── ui_PanelCompassRose
│   │   └── ui_ImageCompassRose (240×240, Scale=512)
│   ├── ui_PanelArrow + ui_ImageArrow
│   ├── ui_PanelHeading + ui_LabelHeading
│   ├── ui_PanelHeadingMode + ui_LabelHeadingMode (T/M toggle)
│   └── ui_PanelConnected (black=connected, red=disconnected)
```

### AttitudeScreen
```
ui_AttitudeScreen (black background)
├── ui_ContainerHorizonGroup (680×680)
│   └── ui_ImageHorizon (680×4 white line)
│
├── ui_ContainerAttitudeGroup (484×484, stationary)
│   ├── ui_PanelPitch + ui_LabelPitchTitle + ui_LabelPitch
│   ├── ui_PanelRoll + ui_LabelRollTitle + ui_LabelRoll
│   ├── ui_PanelHull (ship hull)
│   ├── ui_PanelDeck
│   ├── ui_PanelBridge
│   ├── ui_PanelMast
│   ├── ui_PanelPortside (red navigation light, left)
│   └── ui_PanelStarboard (green navigation light, right)
│
└── ui_ContainerLevelingDialog (hidden by default)
    └── ui_PanelLevelingDialog
        └── ui_LabelLevelingDialog
```

### BrightnessScreen
```
ui_BrightnessScreen (black background)
├── ui_ContainerBrightness (484×484, stationary)
│   └── ui_PanelBrightness (280×280)
│       ├── ui_ImageBrightness (sun icon, Scale=512)
│       ├── ui_LabelBrightness ("Brightness", top)
│       └── ui_LabelBrightnessValue ("XX%", bottom)
│
└── ui_ContainerAdjustment (476×476, hidden by default)
    └── ui_ArcAdjustment (460×460, blue bg, white indicator, 20px)
```

---

## Knob Button Behavior

| Screen | Button Action | Rotation (normal) | Rotation (special) |
|--------|---------------|-------------------|-------------------|
| CompassScreen | Toggle True/Magnetic heading mode | Switch screen | — |
| AttitudeScreen | Trigger level confirmation dialog | Switch screen | — |
| BrightnessScreen | Enter ADJUSTING mode (show arc) | Switch screen | ±5% brightness (ADJUSTING mode only) |

---

## Important Notes

### ESP-NOW Channel Configuration
- Compass uses `peer.channel = 0` = uses WiFi AP's channel
- CrowPanel configured to channel 6: `#define ESP_NOW_CHANNEL 6`
- Router set to fixed channel 6

### ESP-NOW Callback API Versions
- **CrowPanel (Core 2.x):** `void (*)(const uint8_t* mac, const uint8_t* data, int len)`
- **Compass (Core 3.x):** `void (*)(const esp_now_recv_info_t* recv_info, const uint8_t* data, int len)` — MAC from `recv_info->src_addr`

### Level Command Protocol
```cpp
// CrowPanel → Compass (broadcast, 8 bytes)
struct LevelCommand {
    uint8_t magic[4];     // "LVLC"
    uint8_t reserved[4];
};

// Compass → CrowPanel (unicast, 8 bytes)
struct LevelResponse {
    uint8_t magic[4];     // "LVLR"
    uint8_t success;      // 1 = OK, 0 = failed
    uint8_t reserved[3];
};
```

### Screen Carousel Order
- **CW (clockwise):** COMPASS → ATTITUDE → BRIGHTNESS → COMPASS → ...
- **CCW (counter-clockwise):** COMPASS → BRIGHTNESS → ATTITUDE → COMPASS → ...

### NVS Persistence (Preferences)
- Namespace: `"display"`, Key: `"brightness"` (int8_t, 5–100%)
- Loaded once in `brightnessUI.begin()`, saved on auto-save timeout or screen switch
- Default value (first boot): 78% (~200/255)

### Compass Deadband
- Compass sender has 0.25° deadband — no packet sent if heading AND attitude change less than 0.25°
- CrowPanel has additional 0.5° threshold for compass rose rotation rendering only
- Heading label updates at 1° resolution (cheap text operation)

### SquareLine Studio Export
**WARNING:** SquareLine Studio clears export directory completely! Always git commit or backup before export.

### SquareLine Studio Generated Files
`CMakeLists.txt` and `filelist.txt` (in root and `ui_files/`) are auto-generated by SquareLine Studio for ESP-IDF CMake builds. **Arduino IDE ignores them completely.**

### LVGL Scale Value
- 256 = 100% (original size)
- Formula: (target_size / original_size) × 256
- Example: 480/240 × 256 = 512

### PCF8574 GPIO Expander
- **CRITICAL:** pinMode() must be called BEFORE pcf8574.begin()
- P5 = Rotary encoder button (INPUT_PULLUP)

### Code Style Preferences
- Prefer `static constexpr` over `#define` for constants (already used in AttitudeUI.h, CompassUI.h, RotaryEncoder.h)
- `#define` still used in .ino for pin assignments and configuration — can be converted
- Exception: `lv_color_hex()` macros must stay as `#define` (not constexpr-compatible)

---

## Fonts

| Font | Usage |
|------|-------|
| `ui_font_FontHeading96b` | CompassScreen: heading |
| `ui_font_FontHeading64b` | CompassScreen: T/M mode |
| `ui_font_FontAttitude84c` | AttitudeScreen: pitch/roll (includes +) |
| `ui_font_FontAttitudeTitle24` | AttitudeScreen: "Pitch", "Roll" |
| `ui_font_FontDialog36` | AttitudeScreen: leveling dialog |

---

## File Structure

```
ESP32-Crowpanel-compass/
├── ESP32-Crowpanel-compass.ino  # Main program
├── HeadingData.h                # Data structures (incl. LevelCommand/Response)
├── ESPNowReceiver.h/.cpp        # ESP-NOW receiver + level command sender
├── CompassUI.h/.cpp             # Compass screen adapter
├── AttitudeUI.h/.cpp            # Attitude screen adapter + level state machine
├── BrightnessUI.h/.cpp          # Brightness screen adapter + adjustment state machine
├── RotaryEncoder.h/.cpp         # Rotary encoder handler
├── ScreenManager.h/.cpp         # Screen management (3-screen carousel) + cleanup
├── CLAUDE_SESSION_CONTEXT.md    # This file
├── .gitignore
├── ui.h/.c                      # SquareLine generated
├── ui_CompassScreen.h/.c        # SquareLine generated
├── ui_AttitudeScreen.h/.c       # SquareLine generated
├── ui_BrightnessScreen.h/.c     # SquareLine generated
├── ui_helpers.h/.c              # SquareLine generated
├── ui_font_*.c                  # Fonts
├── ui_img_*.c                   # Images
├── CMakeLists.txt               # SquareLine generated (not used by Arduino IDE)
├── filelist.txt                 # SquareLine generated (not used by Arduino IDE)
├── UI/                          # SquareLine project
└── RotaryScreen_2_1/            # Elecrow demo (reference)
```

---

## Build Info

- **Flash usage:** ~36% (1,137,857 bytes of 3,145,728)
- **Compass rose:** 240×240 px, Scale=512, ~850 KB C-code
- **Horizon line:** 680×4 px, 41 KB

---

## Performance Characteristics

| Screen | UI updates/5s | LVGL avg | Notes |
|--------|--------------|----------|-------|
| CompassScreen (heading changing) | ~25 | ~200 ms | Bottleneck: compass rose software rotation |
| CompassScreen (stable heading) | 48–74 | 1–7 ms | Threshold prevents unnecessary re-renders |
| AttitudeScreen (data flowing) | ~80 | 4–13 ms | Horizon line 680×4 is cheap to render |
| AttitudeScreen (stable) | ~83 | <1 ms | Nothing to render |
