# Changelog

All notable changes to this project will be documented in this file.

The format is based on [Keep a Changelog](https://keepachangelog.com/en/1.0.0/), and this project adheres to [Semantic Versioning](https://semver.org/spec/v2.0.0.html).

## [v2.1.0] - 2026-03-07

### Added

#### `BatteryUI` — new screen adapter for battery data
- New `BatteryUI` class realizes `IScreenUI`; constructor: `explicit BatteryUI(ESPNowReceiver &receiver)`
- Receives `BatteryDelta` packets via `ESPNowReceiver::hasNewBatteryData()` / `getBatteryData()` — pull model consistent with other UI adapters
- Four panels: `HOUSE_V` (house bank voltage), `HOUSE_A` (house bank current), `HOUSE_SOC` (state of charge), `START_V` (starter battery voltage) — knob button press cycles in order (modulo)
- Session min/max tracked per measurement (NAN sentinel, resets on reboot, not persisted to NVS)
- EMA-based trend indicators for all four measurements: `↑` / `↓` when EMA deviation exceeds threshold; hidden on first data point and stable readings; EMA alphas: `VOLTAGE_EMA_ALPHA = CURRENT_EMA_ALPHA = SOC_EMA_ALPHA = 0.05`; thresholds: `VOLTAGE_TREND_THRESHOLD = 0.05 V`, `CURRENT_TREND_THRESHOLD = 0.05 A`, `SOC_TREND_THRESHOLD = 0.05 %`
- Connection tracking independent from compass sender: `_last_data_millis` with `CONNECTION_TIMEOUT_MS = 6000 ms`; main value labels show `"---"` on timeout; session min/max preserved during disconnect
- Active panel persisted to NVS (namespace `"battery"`, key `"panel"`) on `onLeave()`; restored on `begin()`
- Registered as screen index 3 in carousel: COMPASS(0) → ATTITUDE(1) → WEATHER(2) → BATTERY(3) → BRIGHTNESS(4)

### Changed

#### `WeatherUI` — trend indicators added for temperature and humidity panels
- Temperature trend (`ui_LabelTrendTemp`) and humidity trend (`ui_LabelTrendHumidity`) added alongside the existing pressure trend (`ui_LabelTrend`) — same EMA pattern
- All three trend labels now hidden in `begin()` and `showWaiting()` (previously only `ui_LabelTrend` was hidden — temperature and humidity trends could remain visible on disconnect)
- EMA alpha and threshold values revised for all three panels: `TEMPERATURE_EMA_ALPHA = PRESSURE_EMA_ALPHA = HUMIDITY_EMA_ALPHA = 0.05`, `TEMPERATURE_TREND_THRESHOLD = PRESSURE_TREND_THRESHOLD = HUMIDITY_TREND_THRESHOLD = 0.05` (previously pressure used `alpha = 0.10`, `threshold = 0.5 hPa`)

#### `ESPNowReceiver` — `BATTERY_DELTA` dispatch added
- `onDataRecv()` switch extended with `BATTERY_DELTA` case — stores `BatteryDelta` payload into `s_latest_battery`, sets `s_has_new_battery = true`; does not update `s_last_rx_millis` / `s_packet_count` (those are compass connection indicators; `BatteryUI` tracks its own `_last_data_millis`)
- Added `hasNewBatteryData() const` — thread-safe read of `s_has_new_battery`
- Added `getBatteryData()` — thread-safe read of `s_latest_battery`, clears `s_has_new_battery`
- Added `s_latest_battery` / `s_has_new_battery` `inline static` members

#### `CrowPanelApplication` — `BatteryUI` added to carousel
- `_batteryUI(ESPNowReceiver&)` member added; declared between `_weatherUI` and `_brightnessUI` to control construction order
- `begin()`: calls `_batteryUI.begin()` and `_screenMgr.addScreen(&_batteryUI)` between weather and brightness
- Screen carousel updated: COMPASS(0) → ATTITUDE(1) → WEATHER(2) → BATTERY(3) → BRIGHTNESS(4)

---

## [v2.0.0] - 2026-03-04

### Added

#### `WeatherUI` — new screen adapter for weather data
- New `WeatherUI` class realizes `IScreenUI`; constructor: `explicit WeatherUI(ESPNowReceiver &receiver)`
- Receives `WeatherDelta` packets (temperature, pressure, humidity) via `ESPNowReceiver::hasNewWeatherData()` / `getWeatherData()` — pull model consistent with `CompassUI` and `AttitudeUI`
- Three panels: `PanelTemperature`, `PanelPressure`, `PanelHumidity` — knob button press cycles TEMPERATURE → PRESSURE → HUMIDITY → TEMPERATURE (modulo)
- Session min/max tracked per measurement (NAN sentinel, resets on reboot, not persisted to NVS)
- Pressure trend indicator `ui_LabelTrend`: shows `↑` / `↓` when EMA of the readings (alpha 0.10) ≥ 0.5 hPa; hidden on first data point and stable readings
- Connection tracking independent from compass: `_last_data_millis` with `CONNECTION_TIMEOUT_MS = 6000` ms (3x 2 s send interval); main value labels show `"---"` on timeout; session min/max preserved during disconnect
- Active panel persisted to NVS (namespace `"weather"`, key `"panel"`) on `onLeave()`; restored on `begin()`

#### `IScreenUI.h` — abstract base class for all screen adapters
- New pure-virtual interface used by `ScreenManager` and `CrowPanelApplication`
- Pure virtuals: `begin()`, `getLvglScreen() const`
- Default-empty virtuals: `onEnter()`, `onLeave()`, `update()`, `onButtonPress()`, `onRotation(int8_t dir)`
- `interceptsRotation() const` — returns `false` by default; override to absorb knob rotation (e.g. `BrightnessUI` in ADJUSTING mode)
- Enables open-ended screen registration without modifying any existing classes

### Changed

#### `espnow_protocol.h` — wrapped in `namespace ESPNow`, framing protocol added
- All contents wrapped in `namespace ESPNow {}` — prevents name collision with application-level structs in other translation units (e.g. `CMPS14Processor::HeadingDelta` in the sender repo)
- `ESPNowReceiver.h`: `using namespace ESPNow;` added — all protocol types remain available unqualified within receiver code
- **Added** `ESPNOW_MAGIC = 0x45534E57` — 4-byte magic (`'E''S''N''W'`) identifies own packets on a shared channel; packets from other ESP-NOW devices are silently discarded
- **Added** `ESPNowMsgType` — `enum class uint8_t`: `HEADING_DELTA=1`, `BATTERY_DELTA=2`, `WEATHER_DELTA=3`, `LEVEL_COMMAND=10`, `LEVEL_RESPONSE=11`
- **Added** `ESPNowHeader` — fixed 8-byte `__attribute__((packed))` struct: `magic` (uint32_t), `msg_type` (uint8_t), `payload_len` (uint8_t), `reserved[2]`; 4-byte aligned so float payloads remain naturally aligned
- **Added** `ESPNowPacket<TPayload>` — `__attribute__((packed))` template wrapping `ESPNowHeader` + `TPayload`
- **Added** `initHeader()` — inline helper to fill all `ESPNowHeader` fields in one call
- **Added** `BatteryDelta` — payload stub for future battery sender: `house_voltage`, `house_current`, `house_power`, `house_soc`, `start_voltage` (5 × float)
- **Added** `WeatherDelta` — payload stub for future weather sender: `temperature_c`, `humidity_p`, `pressure_hpa` (3 × float)
- Existing types (`HeadingDelta`, `HeadingData`, `LevelCommand`, `LevelResponse`, `convertDeltaToData`) unchanged; `LevelCommand`/`LevelResponse` magic fields now redundant (`msg_type` is the authoritative discriminator) but retained for now

#### `ESPNowReceiver` — header-based dispatch, framed send
- `onDataRecv()` rewritten: size-based type discrimination → `ESPNowHeader`-based dispatch
  - Minimum frame size guard: `data_len < sizeof(ESPNowHeader)` → discard
  - Magic validation: `hdr.magic != ESPNOW_MAGIC` → discard
  - Frame integrity check: `data_len < sizeof(ESPNowHeader) + hdr.payload_len` → discard
  - `switch(static_cast<ESPNowMsgType>(hdr.msg_type))` with `HEADING_DELTA`, `LEVEL_RESPONSE`, and `WEATHER_DELTA` cases; `default` silently ignores unknown types
  - `LEVEL_RESPONSE`: `memcmp(resp.magic, "LVLR")` check removed — `msg_type` is now the authoritative discriminator
- **Added** `WEATHER_DELTA` case — stores `WeatherDelta` payload into `s_latest_weather`, sets `s_has_new_weather = true`; does not update `s_last_rx_millis` / `s_packet_count` (those are compass connection indicators; `WeatherUI` tracks own `_last_data_millis`)
- **Added** `hasNewWeatherData() const` — thread-safe read of `s_has_new_weather`
- **Added** `getWeatherData()` — thread-safe read of `s_latest_weather`, clears `s_has_new_weather`
- **Added** `s_latest_weather` / `s_has_new_weather` `inline static` members
- `sendLevelCommand()`: sends `ESPNowPacket<LevelCommand>` (16 bytes) via `initHeader()` instead of bare `LevelCommand` (8 bytes)
- **⚠ Breaking wire protocol change** — requires coordinated update of CMPS14-ESP32-SignalK-gateway to v1.3.0

#### `ScreenManager` — fully rewritten to index-based, type-erased carousel
- **Removed:** typed constructor parameters `CompassUI&`, `AttitudeUI&`, `BrightnessUI&`; `enum class Screen`; `isCompassActive()`, `isAttitudeActive()`, `isBrightnessActive()`; all `switch`-based screen dispatching
- **Added:** `addScreen(IScreenUI*)` — registers a screen before `begin()`; up to `MAX_SCREENS = 8` screens
- **Added:** `getCurrentScreen() const` — returns `IScreenUI*` to the currently active screen
- Carousel arithmetic: `nextIdx()` / `previousIdx()` use modulo (`% _screen_count`); no switch statements
- `switchTo()` calls `onLeavingCurrentScreen()` (→ `onLeave()` on departing screen) then `onEnter()` on arriving screen
- Adding a new screen requires only: create class `: public IScreenUI`, call `addScreen(&instance)` in `begin()` — carousel requires no further changes

#### `CompassUI` — realizes `IScreenUI`
- Now realizes `IScreenUI`; `ESPNowReceiver&` moved from `CrowPanelApplication` to constructor parameter `explicit CompassUI(ESPNowReceiver& receiver)`
- `getLvglScreen() const override` → returns `ui_CompassScreen` (non-inline, keeps `ui.h` out of header)
- `update() override` — calls `_receiver.isConnected()`, `hasNewData()`, `getData()` internally; no parameters
- `onButtonPress() override` — calls private `toggleHeadingMode()`
- `CONNECTION_TIMEOUT_MS = 3000` moved from `CrowPanelApplication` to `CompassUI` as `static constexpr`
- **Removed from public API:** `update(const HeadingData&, bool)`, `showDisconnected()`, `toggleHeadingMode()` (all → private)
- **Removed from header:** `#include "espnow_protocol.h"` (now included transitively via `ESPNowReceiver.h`)

#### `AttitudeUI` — realizes `IScreenUI`
- Now realizes `IScreenUI`
- `getLvglScreen() const override` → returns `ui_AttitudeScreen`
- `update() override` — calls `_receiver.isConnected()`, `hasNewData()`, `getData()` internally; tracks `_last_connected` to call `showWaiting()` once on disconnect transition; always ticks `updateLevelState()` regardless of connection state
- `onButtonPress() override` — replaces `bool handleButtonPress()` (same state machine logic, `void` return)
- `onLeave() override` — calls `cancelLevelOperation()`; ensures dialog is hidden when switching away mid-operation
- **Moved to private:** `updateLevelState()`, `cancelLevelOperation()`
- **Removed from public API:** `update(const HeadingData&, bool)`, `showDisconnected()`, `handleButtonPress()`, `updateLevelState()`, `cancelLevelOperation()`
- `_last_connected` member added; `CONNECTION_TIMEOUT_MS = 3000` added as `static constexpr`
- **Removed from header:** `#include "ui.h"` (kept in `.cpp` only), `#include "espnow_protocol.h"`

#### `BrightnessUI` — realizes `IScreenUI`, PWM channel to constructor
- Now realizes `IScreenUI`
- `pwm_channel` parameter moved from `begin(int)` to constructor: `explicit BrightnessUI(int pwm_channel)`; `begin()` now takes no parameters and overrides `IScreenUI::begin()`
- `getLvglScreen() const override` → returns `ui_BrightnessScreen`
- `update() override` — delegates to private `updateState()`
- `onButtonPress() override` — delegates to private `handleButtonPress()`
- `onRotation(int8_t dir) override` — delegates to private `handleRotation(dir)`
- `interceptsRotation() const override` — returns `isAdjusting()` (true when ADJUSTING)
- `onLeave() override` — delegates to private `cancelAdjustment()` (saves brightness to NVS on screen leave)
- **Moved to private:** `handleButtonPress()`, `handleRotation()`, `updateState()`, `cancelAdjustment()`, `isAdjusting()`
- **Removed from header:** `#include "ui.h"` (kept in `.cpp` only)

#### `CrowPanelApplication` — simplified orchestration
- Constructor init list: `_compassUI(_receiver)`, `_attitudeUI(_receiver)`, `_weatherUI(_receiver)`, `_brightnessUI(PWM_CHANNEL)`, `_screenMgr()` (no args)
- `begin()`: calls `begin()` for each UI adapter in order, then `addScreen()` for each; screen carousel: **COMPASS(0) → ATTITUDE(1) → WEATHER(2) → BRIGHTNESS(3)**; then `_screenMgr.begin()`
- `handleKnobRotation()` — 10 lines → 6: `getCurrentScreen()->interceptsRotation()` / `onRotation()` / `switchNext()` / `switchPrevious()`
- `handleKnobButtonPress()` — 6 lines → 2: `getCurrentScreen()->onButtonPress()`
- `handleUIUpdate()` — 35 lines → 8: single `getCurrentScreen()->update()` call with timing measurement; all data fetch, routing, and per-screen state machine ticks removed
- **Removed:** `CONNECTION_TIMEOUT_MS`, `#include "espnow_protocol.h"`, `isCompassActive()` / `isAttitudeActive()` / `isBrightnessActive()` usage, explicit `updateLevelState()` / `updateState()` calls, `was_connected` static, `showDisconnected()` calls

### Performance
- UI update time now correctly measures only pull-model overhead (data fetch + threshold check): avg 0.54 ms, max 0.80 ms
- LVGL max (~164 ms) now reflects the full compass rose rendering cost; previously this was split between UI time (`lv_obj_update_layout()` ~91 ms) and LVGL time; total rendering budget is unchanged

### UI
- Sun icon on the brightness screen changed - [Sun icons created by Freepik - Flaticon](https://www.flaticon.com/free-icons/sun)
- Humidity icon on the weather screen - [Humidity icons created by Freepik - Flaticon](https://www.flaticon.com/free-icons/humidity)
- Temperature icon on the weather screen - [Temperature icons created by Freepik - Flaticon](https://www.flaticon.com/free-icons/temperature)
- Pressure icon on the weather screen - [Pressure icons created by Muhammad Ali - Flaticon](https://www.flaticon.com/free-icons/pressure)
- Level dialog Success! message timeout increased from 1500 ms to 2000 ms

### Developer Notes

#### ESP-NOW wire protocol (v2.0.0)

All packets now carry an 8-byte `ESPNowHeader` prefix. Requires coordinated update of CMPS14-ESP32-SignalK-gateway to v1.3.0.

| Packet | v1.0.0 | v2.0.0 |
|--------|--------|--------|
| `HeadingDelta` | 16 B bare | 24 B (`ESPNowHeader` + 16 B payload) |
| `LevelCommand` | 8 B bare | 16 B (`ESPNowHeader` + 8 B payload) |
| `LevelResponse` | 8 B bare | 16 B (`ESPNowHeader` + 8 B payload) |
| `WeatherDelta` | - | 20 B (`ESPNowHeader` + 12 B payload) |

```cpp
// Every packet on the wire
struct ESPNowHeader {       // 8 bytes, packed
    uint32_t magic;         // 0x45534E57 ('E''S''N''W')
    uint8_t  msg_type;      // ESPNowMsgType enum
    uint8_t  payload_len;   // sizeof(payload struct)
    uint8_t  reserved[2];
};

template <typename TPayload>
struct ESPNowPacket { ESPNowHeader hdr; TPayload payload; }; // packed
```

---

## [v1.0.0] - 2026-02-25

### Changed

#### Naming conventions unified across all classes
- All private member variables renamed to `_snake_case` (previously mixed camelCase and underscore styles)
- All static variables renamed to `s_snake_case`
- All local variables renamed to `snake_case`

#### Member initialization style unified (C++11 in-class initializers)
- Trivial defaults (`false`, `0`, `nullptr`, enum defaults) moved from constructor member init lists to in-class initializers in header files
- Constructor member init lists now contain only: reference members (mandatory), struct-initialized members (`portMUX_INITIALIZER_UNLOCKED`), and semantically non-trivial defaults (sentinel values such as `0xFFFF`, `0x7FFF`; `_last_button_state(true)` for INPUT_PULLUP idle state; `_use_true_heading(true)`)
- Affected classes: `CompassUI`, `AttitudeUI`, `BrightnessUI`, `RotaryEncoder`, `ScreenManager`, `ESPNowReceiver`

#### Static member declarations moved to headers (C++17 inline static)
- `ESPNowReceiver`: `s_spinlock` (renamed, earlier `_mux`), `s_latest_data`, `s_has_new_data`, `s_last_rx_millis`, `s_packet_count`, `s_level_response_received`, `s_level_response_success`, `BROADCAST_ADDR` declared and initialized as `inline static` in header
- `RotaryEncoder`: `s_instance` declared as `inline static` in header
- Eliminates separate `.cpp` definitions previously required for static members

#### `BROADCAST_ADDR` changed to `inline static constexpr`
- Was a static array defined in `.cpp`; now `inline static constexpr uint8_t BROADCAST_ADDR[6]` in `ESPNowReceiver.h`

#### `update()` signature simplified
- `pps` parameter removed from `CompassUI::update()` and `AttitudeUI::update()` — was unused in both implementations

#### `AttitudeUI::update()` handles disconnected state internally
- `if (!connected) { this->showDisconnected(); return; }` added at top of `update()`
- Disconnected handling no longer requires separate call path from `CrowPanelApplication`

#### Flush diagnostics added to `handleDiagnostics()`
- `s_flush_total`, `s_flush_max`, `s_flush_calls` file-scope static variables measure `draw16bitRGBBitmap` duration in `lvglFlushCb()`
- Fourth `[DIAG]` line added: `Flush calls | avg | max`
- Flush counters reset alongside other diagnostic counters each print cycle
- Redundant `flush_avg` running-average variable removed

#### `nullptr` replacing `NULL` for pointer initialization
- `_buf1 = nullptr` and `s_instance = nullptr` — type-safe null pointer constant

#### Adaptive LVGL tick scheduling
- `lv_conf.h` has `LV_TICK_CUSTOM = 1` — LVGL reads time directly from `millis()` via `LV_TICK_CUSTOM_SYS_TIME_EXPR = (millis())`, thus, `lv_tick_inc()` not required here
- `lv_timer_handler()` return value (`uint32_t next_ms`) now drives the next call interval instead of a fixed 5 ms constant
- Return value represents milliseconds until the next LVGL internal timer is due to fire
- Clamped to `[LVGL_TICK_MIN_MS = 1, LVGL_TICK_MAX_MS = 20]`: `LV_NO_TIMER_READY` (0xFFFFFFFF) and other large values clamp to 20 ms; zero or sub-1 ms values (e.g. render bottleneck, concurrent-call guard returning 1) clamp to 1 ms
- `static constexpr uint32_t LVGL_TICK_INTERVAL_MS = 5` removed; replaced by `uint32_t _next_lvgl_interval_ms = 5` instance member and two constexprs `LVGL_TICK_MIN_MS` / `LVGL_TICK_MAX_MS`
- During screen transitions (~300 ms animation): LVGL requests short intervals → stays at or below 20 ms ceiling, animations unaffected
- During idle / stable heading: LVGL reports ~30 ms (default `LV_DISP_DEF_REFR_PERIOD`) → clamped to 20 ms, reducing redundant `lv_timer_handler()` calls

#### LVGL draw buffer enlarged
- `BUF_PIXELS` increased from `SCREEN_WIDTH * 40` to `SCREEN_WIDTH * 120` (40 → 120 lines)
- Fewer `lvglFlushCb` calls per frame — more pixels transferred per DMA burst
- LVGL max render time reduced: ~99 ms → ~91 ms (measured on CompassScreen with active heading)

### Performance
- Measured on CompassScreen with heading active (~19 Hz): LVGL calls ~102–112/5s (was 151), avg 37–42 ms, max 91 ms, flush avg 4.6 ms

---

## [v0.4.0] - 2026-02-22

### Changed

#### CrowPanelApplication refactoring

- `s_gfx` static file-scope pointer removed — LVGL flush callback now receives `_gfx` via `disp_drv.user_data`:
  - `initLvgl()`: `disp_drv.user_data = &_gfx;`
  - `lvglFlushCb()`: `auto* gfx = static_cast<Arduino_ST7701_RGBPanel*>(disp->user_data);`
- `handleLvglTick()` now uses a `_last_lvgl_tick` timer guard (`LVGL_TICK_INTERVAL_MS = 5 ms`), consistent with the pattern used in `handleUIUpdate()` and `handleDiagnostics()`
- All loop sub-methods (`handleLvglTick`, `handleKnobRotation`, `handleKnobButtonPress`, `handleUIUpdate`, `handleDiagnostics`) now use a uniform timer-check style

#### CompassScreen UI hierarchy flattened
- Intermediate container/panel elements removed: `ui_PanelTop`, `ui_PanelCompassRose`, `ui_PanelArrow`, `ui_PanelHeading`, `ui_PanelHeadingMode`
- `ui_ImageCompassRose`, `ui_ImageArrow`, `ui_LabelHeading`, `ui_LabelHeadingMode`, `ui_PanelConnected` now direct children of `ui_CompassScreen`
- AttitudeScreen and BrightnessScreen hierarchy unchanged

#### Compass rose image optimizations
- Image format changed from `LV_IMG_CF_TRUE_COLOR_ALPHA` (3 bytes/px) to `LV_IMG_CF_TRUE_COLOR` (RGB565, 2 bytes/px) — alpha channel removed
- Image resolution 240×240 px with `lv_img_set_zoom(512)` — renders at 480×480, LVGL rotation operates on ¼ of the pixels vs. a native 480×480 source image
- `lv_img_set_antialias(ui_ImageCompassRose, false)` added to `CompassUI::begin()` — nearest-neighbor scaling, no per-pixel interpolation during rotation
- Combined result: LVGL avg ~200 ms → ~30 ms, max ~206 ms → ~99 ms (measured on CompassScreen with heading changing at ~19 Hz)

### Performance
- Compass rose rotation render time max reduced by 2× vs. v0.3.0 baseline

---

## [v0.3.0] - 2026-02-21

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
- `docs/CrowPanel_2_1_HMI_mounting.stl` - Mounting frame model for 3D printing
- `docs/uml_diagram.png`- Class diagram
- `docs/full_uml_diagram.jpeg` - Class diagram including CMPS14-ESP32-SignalK-gateway
- `docs/*screen.png`- Screenshots of compass, attitude and brightness screens
- `.github` - Directory containing pull request, bug report and feature request templates.
- `CONTRIBUTING.md` - Guidelines to contribute to the project

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

#### Enum classes refactored
- `LevelState` - Now a private member of `AttitudeUI` class
- `Direction` and `Screen` - Now privagte members of `ScreenManager` class
- `BrightnessState` - Now a private member of `BrightnessUI` class

#### HeadingData.h renamed
- `HeadingData.h` renamed to `espnow_protocol.h` - Documented as `espnow.protocol` package on the UML class diagram
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

[v2.1.0]: https://github.com/mkvesala/ESP32-Crowpanel-compass/releases/tag/v2.1.0
[v2.0.0]: https://github.com/mkvesala/ESP32-Crowpanel-compass/releases/tag/v2.0.0
[v1.0.0]: https://github.com/mkvesala/ESP32-Crowpanel-compass/releases/tag/v1.0.0
[v0.4.0]: https://github.com/mkvesala/ESP32-Crowpanel-compass/releases/tag/v0.4.0
[v0.3.0]: https://github.com/mkvesala/ESP32-Crowpanel-compass/releases/tag/v0.3.0
[v0.2.0]: https://github.com/mkvesala/ESP32-Crowpanel-compass/releases/tag/v0.2.0
[v0.1.0]: https://github.com/mkvesala/ESP32-Crowpanel-compass/releases/tag/v0.1.0
[v0.0.2]: https://github.com/mkvesala/ESP32-Crowpanel-compass/releases/tag/v0.0.2
[v0.0.1]: https://github.com/mkvesala/ESP32-Crowpanel-compass/releases/tag/v0.0.1 
