# Claude Session Context - ESP32 CrowPanel Compass

**Date:** 2026-02-19 (updated)
**Project:** ESP32-Crowpanel-compass
**Hardware:** Elecrow CrowPanel 2.1" HMI (ESP32-S3, 480x480 IPS, Rotary Knob)

**ESP-NOW sender:** CMPS14-ESP32-SignalK-gateway compass, source code and documentation available in public repository [mkvesala/CMPS14-ESP32-SignalK-gateway](https://github.com/mkvesala/CMPS14-ESP32-SignalK-gateway) branch `feature/crow-panel-integration`.

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
| Knob button toggles T/M heading mode | RotaryEncoder, CompassUI, CrowPanelApplication |
| HeadingData simplified (no valid flags) | HeadingData.h |
| PanelConnected: black=connected, red=disconnected | CompassUI.cpp |
| WiFi removed from CrowPanel (ESP-NOW only) | ESPNowReceiver |
| ESP-NOW channel 6 (matches router) | CrowPanelApplication |
| Attitude Level feature (full end-to-end) | AttitudeUI, ESPNowReceiver, ScreenManager |
| BrightnessScreen (backlight adjustment, NVS persist) | BrightnessUI, ScreenManager |
| Compass rose rotation threshold 0.5° | CompassUI |
| Diagnostics: PPS, UI timing, LVGL timing, heap, stack | CrowPanelApplication |
| Leveling dialog text wrapping fixed | AttitudeUI.cpp |
| ESPNowReceiver simplified (direct static callback, no _instance pointer) | ESPNowReceiver.h/.cpp |
| Serial.print cleanup (only [DIAG] lines remain) | All own files |
| `#define` → `static constexpr` | All own .h files, CrowPanelApplication |
| `COLOR_CONNECTED/DISCONNECTED` → `static constexpr uint32_t` in CompassUI | CompassUI.h/.cpp |
| `showWaiting()` moved to private in CompassUI and AttitudeUI | CompassUI.h, AttitudeUI.h |
| `isShowingTrueHeading()` removed from CompassUI (unused) | CompassUI.h |
| `getLevelState()` removed from AttitudeUI (unused) | AttitudeUI.h |
| AttitudeUI takes ESPNowReceiver& in constructor (was method param) | AttitudeUI.h/.cpp |
| AttitudeUI::showDisconnected() now calls showWaiting() (was empty) | AttitudeUI.cpp |
| LV_COLOR_16_SWAP preprocessor check simplified | CrowPanelApplication |
| ScreenManager takes UI refs in constructor, unified `switchTo(Screen, Direction)` | ScreenManager.h/.cpp |
| ScreenManager `switchNext`/`switchPrevious` refactored to use `nextScreen()`/`previousScreen()` helpers | ScreenManager.cpp |
| BrightnessUI header cleaned up (doxygen → banner comments, `<Preferences.h>` include added) | BrightnessUI.h |
| CrowPanelApplication: app-luokka, omistaa kaikki instanssit | CrowPanelApplication.h/.cpp |
| RotaryEncoder: PCF8574& konstruktorissa (ei enää begin-parametri) | RotaryEncoder.h/.cpp |
| .ino minimoitu: vain `app`, `setup()`, `loop()` | ESP32-Crowpanel-compass.ino |
| README.md luotu | README.md |
| docs/projectlogo.svg luotu | docs/projectlogo.svg |

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

Four diagnostic lines printed every 5 seconds:
```
[DIAG] PPS: 18.3 | UI updates: 52 | UI avg: 0.55 ms | UI max: 0.74 ms
[DIAG] LVGL calls: 151 | avg: 30.17 ms | max: 99.39 ms
[DIAG] Flush calls: 151 | avg: X.XX ms | max: X.XX ms
[DIAG] Heap free: 8133183 | min: 8128023 | Stack loop: 5468 | enc: 1268 | btn: 768
```

- **UI updates:** How many times UI draw code ran in 5s window
- **LVGL calls/avg/max:** `lv_timer_handler()` call count and duration — reveals rendering bottleneck
- **Flush calls/avg/max:** `lvglFlushCb` call count and `draw16bitRGBBitmap` duration — flush portion of LVGL time
- **Heap free/min:** Current and all-time minimum free heap (leak detection)
- **Stack loop/enc/btn:** Stack high water marks for main loop, encoder task, button task

**Key finding:** Compass rose `lv_img_set_angle()` is the main performance bottleneck on CompassScreen. No practical alternative exists on this hardware (no GPU, no hardware rotation support in GC9A01 display controller).

**Optimizations applied (v0.4.0):** avg ~200 ms → ~30 ms, max ~206 ms → ~99 ms:
- Image format `LV_IMG_CF_TRUE_COLOR` (RGB565, 2 bytes/px) — alpha channel removed
- Image 240×240 px with `lv_img_set_zoom(512)` — LVGL rotation operates on ¼ of the pixels vs. 480×480
- `lv_img_set_antialias(ui_ImageCompassRose, false)` in `CompassUI::begin()` — nearest-neighbor, no interpolation
- CompassScreen UI hierarchy flattened (intermediate containers removed)

---

## Architecture

```
┌─────────────────────────────────────────────────────────────┐
│               ESP32-Crowpanel-compass.ino                   │
│         CrowPanelApplication app  →  app.begin/loop()       │
├─────────────────────────────────────────────────────────────┤
│                  CrowPanelApplication                       │
│  ┌──────────────┐  ┌──────────────┐  ┌──────────────┐      │
│  │RotaryEncoder │  │ESPNowReceiver│  │ScreenManager │      │
│  │ (_pcf8574&)  │  │              │  │              │      │
│  └──────┬───────┘  └──────┬───────┘  └──────┬───────┘      │
│         │                 │                  │              │
│         ▼                 ▼                  ▼              │
│  ┌────────────────────────────────────────────────┐        │
│  │                 HeadingData                    │        │
│  └────────────────────────────────────────────────┘        │
│         │                                    │              │
│         ▼                                    ▼              │
│  ┌──────────────┐  ┌──────────────┐  ┌──────────────┐      │
│  │  CompassUI   │  │  AttitudeUI  │  │ BrightnessUI │      │
│  │              │  │ (_receiver&) │  │              │      │
│  └──────────────┘  └──────────────┘  └──────────────┘      │
│                                                             │
│  Hardware: PCF8574, Arduino_ESP32RGBPanel, ST7701_RGBPanel  │
└─────────────────────────────────────────────────────────────┘
```

---

## UI Structure (SquareLine Studio)

### CompassScreen
```
ui_CompassScreen
├── ui_ImageCompassRose (240×240, Scale=512)
├── ui_ImageArrow
├── ui_LabelHeading
├── ui_LabelHeadingMode (T/M toggle)
└── ui_PanelConnected (black=connected, red=disconnected)
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
- CrowPanel configured to channel 6: `static constexpr uint8_t ESP_NOW_CHANNEL = 6`
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
- **Current compass rose:** 240×240 px source, zoom=512 renders at 480×480 — rotation runs on ¼ pixels

### PCF8574 GPIO Expander
- **CRITICAL:** pinMode() must be called BEFORE pcf8574.begin()
- P5 = Rotary encoder button (INPUT_PULLUP)
- PCF8574 alustus tapahtuu `CrowPanelApplication::initPcfAndResetLines()`:ssa

### CrowPanelApplication Design
- Omistaa kaikki instanssit jäsenmuuttujina (ei globaaleja .ino:ssa)
- `_bus` ja `_gfx` stackissa — konstruktori-init listassa, `_gfx(&_bus, ...)`
- LVGL flush-callback saa `_gfx`-osoittimen `disp->user_data`:sta (`disp_drv.user_data = &_gfx` asetetaan `initLvgl()`:ssa, callback ottaa talteen `auto* gfx = static_cast<Arduino_ST7701_RGBPanel*>(disp->user_data)`)
- `_diag_*`-laskurit instanssimuuttujia (alustetaan `= 0` headerissa, C++11), etuliite `_`
- `s_flush_total/max/calls` tiedostotason static-muuttujia `.cpp`:ssä — näkyvät `handleDiagnostics()`:lle saman käännösyksikön kautta, nollataan diagnostiikkaprintissä
- `_buf1` ja `_draw_buf` instanssimuuttujia (ei staattisia), etuliite `_`
- `BUF_PIXELS = SCREEN_WIDTH * 120` (120 riviä)
- Loop jaettu yksityisiin metodeihin: `handleLvglTick`, `handleKnobRotation`, `handleKnobButtonPress`, `handleUIUpdate`, `handleDiagnostics`

### RotaryEncoder Refaktorointi
- `PCF8574&` konstruktoriparametrina (ei enää `begin(PCF8574&)`)
- `_pcf8574` tallennetaan viitteenä (ei osoittimena)
- `processButton()`-null-check poistettu (viite on aina validi)
- `s_instance` staattinen osoitin säilytetty `.cpp`:ssä FreeRTOS-taskeja varten (ei poistettu)

### Code Style Preferences
- Prefer `static constexpr` over `#define` for constants (used in all own .h files and .ino)
- `lv_color_hex()` calls written inline at use site, color values stored as `static constexpr uint32_t` when reused
- LVGL/preprocessor checks (`LV_COLOR_16_SWAP`, `LV_COLOR_DEPTH`) kept as `#if` — these are LVGL config macros
- PCF8574 pin constants (`P0`–`P7`) are `#define 0`–`7` from library, stored as `static constexpr uint8_t`

### Dependency Injection (constructor references)
- **AttitudeUI:** `ESPNowReceiver&` passed via constructor, stored as private `_receiver` reference
  - Used by level state machine: `sendLevelCommand()`, `hasLevelResponse()`, `getLevelResult()`
- **ScreenManager:** `CompassUI&`, `AttitudeUI&`, `BrightnessUI&` passed via constructor
  - Used by `onLeavingCurrentScreen()`: calls `_attitudeUI.cancelLevelOperation()`, `_brightnessUI.cancelAdjustment()`
  - `_compassUI` stored for future use (no cleanup needed currently)
- Instantiation order managed by `CrowPanelApplication` member declaration order in .h: `_bus` → `_gfx` → `_pcf8574` → `_receiver` → `_compassUI` → `_attitudeUI(_receiver)` → `_brightnessUI` → `_encoder(_pcf8574)` → `_screenMgr(_compassUI, _attitudeUI, _brightnessUI)`

### ScreenManager Internal Design
- `switchNext()` / `switchPrevious()` are thin public methods that delegate to private `switchTo(Screen, Direction)`
- Carousel order defined in pure `const` helpers: `nextScreen()`, `previousScreen()`
- `enum class Direction { CW, CCW }` is private — maps to `LV_SCR_LOAD_ANIM_MOVE_LEFT` / `MOVE_RIGHT`
- Single unified `switchTo()` handles: `_initialized` guard → `onLeavingCurrentScreen()` → state update → target selection → animated screen load

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
├── ESP32-Crowpanel-compass.ino  # Main program (minimal: app, setup, loop)
├── CrowPanelApplication.h/.cpp  # App orchestrator, owns all instances
├── HeadingData.h                # Data structures (incl. LevelCommand/Response)
├── ESPNowReceiver.h/.cpp        # ESP-NOW receiver + level command sender
├── CompassUI.h/.cpp             # Compass screen adapter
├── AttitudeUI.h/.cpp            # Attitude screen adapter + level state machine
├── BrightnessUI.h/.cpp          # Brightness screen adapter + adjustment state machine
├── RotaryEncoder.h/.cpp         # Rotary encoder handler (PCF8574& in constructor)
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
├── README.md                    # Project documentation
├── docs/projectlogo.svg         # Project logo (shown in README)
├── UI/                          # SquareLine project
└── RotaryScreen_2_1/            # Elecrow demo (reference)
```

---

## Build Info

- **Flash usage:** ~36% (1,137,857 bytes of 3,145,728)
- **Compass rose:** 240×240 px, zoom=512 (renders at 480×480), `LV_IMG_CF_TRUE_COLOR` (no alpha), antialias off
- **Horizon line:** 680×4 px, 41 KB
- **LVGL:** 8.3.11
- **SquareLine Studio:** 1.6.0
- **ESP32 board package:** 2.0.14 (newer versions incompatible with Elecrow example code)

---

## Performance Characteristics

| Config | UI updates/5s | LVGL avg | LVGL max | Notes |
|--------|--------------|----------|----------|-------|
| v0.3.0 baseline | ~25 | ~200 ms | ~206 ms | 240×240 Scale=512, alpha, antialias ON |
| v0.4.0 current  | ~52 | ~30 ms | ~99 ms | 240×240 zoom=512, no alpha, antialias OFF ✅ |
| Test 1: antialias ON  | ~28 | ~64–77 ms | ~195 ms | 480×480, no alpha, antialias ON |
| Test 2: 240×240 Scale=512, antialias ON | ~31–33 | ~38–55 ms | ~180 ms | 240×240, no alpha, antialias ON |
| Test 3: 240×240 Scale=512, antialias OFF | ~52 | ~30 ms | ~99 ms | 240×240, no alpha, antialias OFF ✅ best |
| CompassScreen (stable heading) | 48–74 | 1–7 ms | — | Threshold prevents unnecessary re-renders |
| AttitudeScreen (data flowing) | ~80 | 4–13 ms | — | Horizon line 680×4 is cheap to render |
| AttitudeScreen (stable) | ~83 | <1 ms | — | Nothing to render |
