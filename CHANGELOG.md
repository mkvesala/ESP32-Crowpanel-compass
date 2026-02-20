# Changelog

All notable changes to this project will be documented in this file.

The format is based on [Keep a Changelog](https://keepachangelog.com/en/1.0.0/), and this project adheres to [Semantic Versioning](https://semver.org/spec/v2.0.0.html).

## [v0.3.0] - 2026-02-19

### Added

#### CrowPanelApplication class
- New `CrowPanelApplication` class that owns all application instances and orchestrates the main program
  - Owns: `Arduino_ESP32RGBPanel`, `Arduino_ST7701_RGBPanel`, `PCF8574`, `ESPNowReceiver`, `CompassUI`, `AttitudeUI`, `BrightnessUI`, `RotaryEncoder`, `ScreenManager`
  - Public API: `begin()`, `loop()`
  - Hardware init methods: `initPcfAndResetLines()`, `initDisplay()`, `initBacklight()`, `initLvgl()`
  - Loop split into private methods: `handleLvglTick()`, `handleKnobRotation()`, `handleKnobButtonPress()`, `handleUIUpdate()`, `handleDiagnostics()`
  - `_bus` (`Arduino_ESP32RGBPanel`) and `_gfx` (`Arduino_ST7701_RGBPanel`) owned as stack members, initialized in constructor member init list
  - `s_gfx` static pointer used only for LVGL flush callback (set once in `initLvgl()`)
  - All constants moved from `.ino` to `static constexpr` members of `CrowPanelApplication`
  - Diagnostic counters as non-static instance members (initialized inline, C++11)

#### New files
- `CrowPanelApplication.h` - Application class declaration
- `CrowPanelApplication.cpp` - Application class implementation
- `README.md` - Project documentation
- `CHANGELOG.md` - This file
- `docs/projectlogo.svg` - Project logo for README

### Changed

#### Main .ino minimized
- `ESP32-Crowpanel-compass.ino` reduced to three elements: `CrowPanelApplication app`, `setup()` calling `app.begin()`, `loop()` calling `app.loop()`
- All hardware init, loop logic, constants and diagnostics moved to `CrowPanelApplication`

#### RotaryEncoder refactored
- Constructor now takes `PCF8574&` as parameter: `RotaryEncoder(PCF8574& pcf)`
- `_pcf8574` stored as a reference member (`PCF8574&`) instead of a raw pointer (`PCF8574*`)
- `begin()` no longer takes a `PCF8574&` parameter
- Null-check `if (!_pcf8574) return;` removed from `processButton()` — reference is always valid
- `s_instance` static pointer retained in `.cpp` for FreeRTOS task access

#### Member instantiation order
- `CrowPanelApplication` member declaration order in `.h` controls C++ construction order:
  `_bus` → `_gfx` → `_pcf8574` → `_receiver` → `_compassUI` → `_attitudeUI` → `_brightnessUI` → `_encoder` → `_screenMgr`

---

## [v0.2.0] - 2026-02-17

### Added

#### BrightnessScreen
- New `BrightnessUI` class for display backlight brightness adjustment
  - Arc overlay UI (ADJUSTING mode): knob rotation adjusts brightness ±5%, updates arc, label and backlight in real-time
  - 3-second auto-save timeout after last rotation → saves to NVS and returns to idle
  - NVS persistence via ESP32 `Preferences` library, namespace `"display"`, key `"brightness"`
  - Brightness range: 5%–100% (minimum 5% prevents screen going completely dark)
  - Default on first boot: 78% (~200/255 PWM duty)
  - PWM: GPIO 6, LEDC channel 0, 5 kHz, 8-bit resolution
- `ScreenManager` extended to support 3-screen carousel (COMPASS → ATTITUDE → BRIGHTNESS)

#### Attitude Level feature
- Full end-to-end attitude leveling via knob button on AttitudeScreen
- Two-press confirmation state machine:
  1. Knob press → CONFIRM_WAIT: dialog "Level attitude?\n\nPress knob again\nto confirm." (yellow)
  2. Second press → SENDING: "Leveling..." (white), sends `LevelCommand` broadcast via ESP-NOW
  3. Response received → SUCCESS: "Success!" (green) or FAILED: "Failed!" (red)
  4. Timeout or screen switch → IDLE (dialog hidden)
- Timeouts: CONFIRM_WAIT 3s, SENDING 3s, SUCCESS 1.5s, FAILED 2s
- `ESPNowReceiver` extended: `sendLevelCommand()`, `hasLevelResponse()`, `getLevelResult()`

#### Diagnostics
- Three `[DIAG]` lines printed to Serial every 5 seconds:
  - PPS, UI update count, UI avg/max runtime
  - LVGL `lv_timer_handler()` call count, avg/max duration
  - Heap free/min, stack high water marks for loop task, encoder task, button task

### Changed

#### Code quality refactoring
- `#define` constants replaced with `static constexpr` throughout all own `.h` files and `.ino`
- `COLOR_CONNECTED` / `COLOR_DISCONNECTED` → `static constexpr uint32_t` in `CompassUI`
- `showWaiting()` moved to private in `CompassUI` and `AttitudeUI`
- `isShowingTrueHeading()` removed from `CompassUI` (unused)
- `getLevelState()` removed from `AttitudeUI` (unused)
- `AttitudeUI` constructor now takes `ESPNowReceiver&` (was a method parameter)
- `AttitudeUI::showDisconnected()` now calls `showWaiting()` (was empty)
- `LV_COLOR_16_SWAP` preprocessor check simplified
- `Serial.print` cleanup — only `[DIAG]` lines remain in own files
- `ESPNowReceiver` simplified: direct static callback, no `_instance` pointer

#### ScreenManager redesign
- Takes `CompassUI&`, `AttitudeUI&`, `BrightnessUI&` via constructor (dependency injection)
- Unified `switchTo(Screen, Direction)` private method replaces duplicated logic
- `switchNext()` / `switchPrevious()` delegated to `nextScreen()` / `previousScreen()` helpers
- `enum class Direction { CW, CCW }` private, maps to LVGL slide animations
- `onLeavingCurrentScreen()` handles cleanup: `cancelLevelOperation()`, `cancelAdjustment()`

#### BrightnessUI header
- Banner comments replacing Doxygen style
- `<Preferences.h>` include added

### Performance

- Compass rose `lv_img_set_angle()` identified as main bottleneck: ~194 ms LVGL software re-render per frame on ESP32-S3
- Rotation threshold 0.5° added to `CompassUI`: skips re-render when heading change is below threshold
  - Handles 360°→0° wraparound, sentinel `0xFFFF` forces first render

### Developer Notes

#### Level command protocol (ESP-NOW)
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

#### ESP-NOW channel
- CrowPanel and compass must share the same WiFi channel
- CrowPanel: `static constexpr uint8_t ESP_NOW_CHANNEL = 6`
- Router must be set to fixed channel 6 to prevent channel jumping

---

## [v0.1.0] - 2026-02-10

### Including
- [v0.0.2]
- [v0.0.1]

### Added

#### Initial MVP implementation
- ESP-NOW receiver for `HeadingData` struct (heading, pitch, roll in radians) from CMPS14-ESP32-SignalK-gateway
- WiFi removed from CrowPanel — ESP-NOW only operation
- ESP-NOW channel 6 to match compass sender

#### CompassScreen
- Rotating compass rose (240x240 px image, rendered at 480x480 with LVGL scale 512)
- Heading value label (96pt bold font)
- True/Magnetic heading mode toggle via knob button press
- T/M mode indicator label (64pt bold font)
- Connected indicator panel (black = connected, red = disconnected)

#### AttitudeScreen
- Artificial horizon: white 680x4 px image rotated and translated by pitch and roll
- Pitch and roll value labels (84pt font)
- Ship silhouette overlay (hull, deck, bridge, mast, port/starboard navigation lights)

#### RotaryEncoder
- FreeRTOS tasks on core 0: `encoderTask` (2ms polling), `buttonTask` (5ms polling)
- Rising edge detection for rotation, falling edge + 50ms debounce for button press
- Thread-safe access via `portENTER_CRITICAL` spinlock

#### ScreenManager
- 2-screen carousel: COMPASS ↔ ATTITUDE with animated slide transitions
- Knob rotation CW/CCW switches screens

#### HeadingData
- Simplified struct without validity flags: `heading_rad`, `heading_true_rad`, `pitch_rad`, `roll_rad`

[v0.3.0]: https://github.com/mkvesala/ESP32-CrowPanel-compass/releases/tag/v0.3.0
[v0.2.0]: https://github.com/mkvesala/ESP32-CrowPanel-compass/releases/tag/v0.2.0
[v0.1.0]: https://github.com/mkvesala/ESP32-CrowPanel-compass/releases/tag/v0.1.0
[v0.0.2]: https://github.com/mkvesala/ESP32-CrowPanel-compass/releases/tag/v0.0.2
[v0.0.1]: https://github.com/mkvesala/ESP32-CrowPanel-compass/releases/tag/v0.0.1 