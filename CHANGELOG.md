# Changelog

All notable changes to this project will be documented in this file.

The format is based on [Keep a Changelog](https://keepachangelog.com/en/1.0.0/), and this project adheres to [Semantic Versioning](https://semver.org/spec/v2.0.0.html).

## [v4.1.0] - 2026-05-12

### Fixed

#### `AttitudeUI` — MINMAX view shows only values from when AttitudeUI was the active screen

`updateMinMax()` was called inside `AttitudeUI::update()`, which only runs for the active screen. When the user navigated away, min/max tracking paused. Values accumulated only while AttitudeUI was visible, not across the full runtime.

Additionally, `getData()` clears the `s_has_new_data` flag — so even a hypothetical background call from AttitudeUI would silently lose packets already consumed by CompassUI (which shares the same `HeadingDelta` message type).

**Fix:** Min/max tracking moved into `ESPNowReceiver::onDataRecv()` inside the `HEADING_DELTA` case, under the same spinlock as the existing data store. Every received packet updates the lifetime min/max regardless of which screen is active.

`AttitudeUI`:
- Removed private members `_min_pitch_x10`, `_max_pitch_x10`, `_min_roll_x10`, `_max_roll_x10` and private method `updateMinMax()`
- `updateMinMaxPanels()` and `updateMinMaxLabels()` now read from `ESPNowReceiver::getMinPitch_x10()`, `getMaxPitch_x10()`, `getMinRoll_x10()`, `getMaxRoll_x10()`

`ESPNowReceiver` — new public static API:
- `getMinPitch_x10()` / `getMaxPitch_x10()` / `getMinRoll_x10()` / `getMaxRoll_x10()` — thread-safe reads of lifetime extremes (×10 integer format, same scale as `HeadingData`)
- `hasMinMaxData()` — returns `true` once at least one packet has been received
- `resetMinMax()` — resets all four values to sentinel (`0x7FFF`); runtime-only, not persisted to NVS

New private static members: `s_min_pitch_x10`, `s_max_pitch_x10`, `s_min_roll_x10`, `s_max_roll_x10` (all initialized to `MINMAX_SENTINEL = 0x7FFF`).

---

### Added

#### EngineScreen — new screen for HALMET engine and tank data

New `EngineUI` class realizes `IScreenUI`. Receives `HALMETEngineDelta` (msg type 5) and `HALMETTankDelta` (msg type 6) packets from HALMET-ESP32-SignalK-gateway.

**View cycle:** `EXHAUST` → `FUEL0` → `EXHAUST` (knob button press cycles, NVS persisted, namespace `"engine"`, key `"view"`)

---

##### EXHAUST view

Shows exhaust temperature in °C converted from Kelvin (`exhaust_temp_k − 273.15`). Session min/max tracked runtime-only (NAN sentinel, resets on reboot, not persisted to NVS).

EMA trend indicator (↑/↓): `EXHAUST_EMA_ALPHA = 0.05`, neutral zone threshold `EXHAUST_TREND_THRESHOLD = 0.001`. Hidden until the second data point arrives and in the neutral zone. EMA reference drifts toward the current EMA when in the neutral zone.

On disconnect: main value label shows `"---"`, trend indicator hidden. Min/max labels preserved if a reading has previously arrived.

---

##### FUEL0 view

Arc gauge (`ui_ArcFuel`, 180°–360° sweep, arc value 0–100) maps directly to fuel ratio 0.0–1.0. Tank capacity `TANK_CAPACITY_L = 400.0 L`. Litres label: `(int)round(ratio × 400)`.

Dynamic arc color based on fuel ratio:

| Ratio | Color | Hex |
|-------|-------|-----|
| ≥ 25% | Green | `0x28C850` |
| 10–25% | Yellow | `0xE6B400` |
| < 10% | Red | `0xDC2828` |

Arc value and color render-cached (`_last_arc_value`, `_last_arc_color`) — no redundant `lv_arc_set_value()` or `lv_obj_set_style_arc_color()` calls. Color cache sentinel `0xFFFFFFFF` forces color set on first update.

On disconnect: litres label shows `"---"`.

---

**Connection tracking:** Engine and tank connections tracked independently via `_last_engine_millis` and `_last_tank_millis`. `CONNECTION_TIMEOUT_MS = 6000`. Engine data and tank data can disconnect and reconnect independently without affecting each other's display state.

Registered as screen index 4: COMPASS(0) → ATTITUDE(1) → WEATHER(2) → BATTERY(3) → **ENGINE(4)** → BRIGHTNESS(5)

---

#### `espnow_protocol.h` — HALMET message types and payload structs

`ESPNowMsgType` enum extended:

```cpp
HALMET_ENGINE_DELTA = 5,
HALMET_TANK_DELTA   = 6,
```

New payload structs (sent by HALMET-ESP32-SignalK-gateway):

```cpp
struct HALMETEngineDelta {
    float exhaust_temp_k;    // propulsion.0.exhaustTemperature [K]
};

struct HALMETTankDelta {
    float fuel_level_ratio;  // tanks.fuel.0.currentLevel [0.0..1.0]
};
```

Wire packets: `ESPNowHeader` (8 B) + payload (4 B each) = 12 B total per packet.

---

#### `ESPNowReceiver` — HALMET dispatch added

`onDataRecv()` switch extended with `HALMET_ENGINE_DELTA` and `HALMET_TANK_DELTA` cases — store payloads directly into static members (no conversion needed, raw floats stored as-is), set flags.

Added:
- `hasNewEngineData() const` — thread-safe read of `s_has_new_engine`
- `getEngineData()` — thread-safe read of `s_latest_engine`, clears `s_has_new_engine`
- `hasNewTankData() const` — thread-safe read of `s_has_new_tank`
- `getTankData()` — thread-safe read of `s_latest_tank`, clears `s_has_new_tank`
- `s_latest_engine` / `s_has_new_engine` / `s_latest_tank` / `s_has_new_tank` `inline static` members

NaN guard for both payloads is in `EngineUI::update()` (`if (!isnan(eng.exhaust_temp_k))`, `if (!isnan(tank.fuel_level_ratio))`), consistent with the pattern used in `BatteryUI` and `WeatherUI`.

---

#### `CrowPanelApplication` — `EngineUI` added to carousel

- `_engineUI(ESPNowReceiver&)` member added between `_batteryUI` and `_brightnessUI` (controls construction order)
- `begin()`: calls `_engineUI.begin()` and `_screenMgr.addScreen(&_engineUI)` between battery and brightness
- Screen carousel updated: COMPASS(0) → ATTITUDE(1) → WEATHER(2) → BATTERY(3) → ENGINE(4) → BRIGHTNESS(5)

---

#### EngineScreen SquareLine Studio design

New screen exported from SquareLine Studio 1.6.0:

- `ui_PanelExhaustTemp` — 480×480 container for EXHAUST view: `ui_ImageExhaustTem` (icon), `ui_LabelExhaustTemp` (96pt bold, centered), `ui_LabelTrendExhaustTemp` (36pt, y+80), `ui_LabelMaxExhaustTemp` (36pt, y+120), `ui_LabelMinExhaustTemp` (36pt, y+160)
- `ui_ContainerFuelGauge` — 484×484 container for FUEL0 view: `ui_ArcFuel` (480×480, 180°–360° sweep, 25px arc width), `ui_LabelLitres` (96pt bold, y+5), `ui_LabelLitresTitle` ("L", 36pt, y+70), fraction labels (0 / ¼ / ½ / ¾ / F), five static tick-mark panels (Panel12, Panel34, Panel14, PanelEmpty, PanelFull), `ui_ImageFuel` (fuel icon, scale 192)

A PNG fuel gauge background image that was present in the initial SquareLine design was removed and replaced by programmatic panels, reducing flash usage.

Generated files: `ui_EngineScreen.h`, `ui_EngineScreen.c`

---

## [v4.0.0] - 2026-05-02

### Removed

#### Leveling functionality — CrowPanel is now receive-only

CrowPanel no longer sends any ESP-NOW packets. The attitude leveling command (sent to CMPS14-ESP32-SignalK-gateway) and the corresponding response handling have been removed entirely.

**`ESPNowReceiver` — public API removed:**
- `sendLevelCommand()` — built and broadcast `ESPNowPacket<LevelCommand>`; managed the broadcast peer registration
- `hasLevelResponse()` — polled `s_level_response_received` flag
- `getLevelResult()` — retrieved and cleared the level response success flag

Static members removed: `s_level_response_received`, `s_level_response_success`, `BROADCAST_ADDR`.

The `LEVEL_RESPONSE` case removed from `onDataRecv()` dispatch switch. Pre-existing cases (`HEADING_DELTA`, `BATTERY_DELTA`, `WEATHER_DELTA`) unchanged; `GNSS_DELTA` added as part of this release (see below).

---

**`espnow_protocol.h` — protocol definitions removed:**

`ESPNowMsgType` enum values removed:
```cpp
LEVEL_COMMAND   = 10,   // removed
LEVEL_RESPONSE  = 11,   // removed
```

Payload structs removed:
```cpp
struct LevelCommand { uint8_t magic[4]; uint8_t reserved[4]; };    // removed
struct LevelResponse { uint8_t magic[4]; uint8_t success; uint8_t reserved[3]; }; // removed
```

`ESPNowPacket<TPayload>` template and `initHeader()` helper are **retained** — they belong to the shared protocol definition and will be used by companion projects.

---

**`AttitudeUI` — LEVELING view removed:**

`AttitudeView::LEVELING` removed from the enum; `AttitudeUI` now cycles between two views only. `LevelState` state machine (`IDLE`, `COUNTDOWN`, `SENDING`, `SUCCESS`, `FAILED`) removed entirely along with all associated members, constants, and private methods (`updateLevelState()`, `updateLevelDialog()`, `setLevelState()`).

The min/max reset-on-leveling-success behavior is also removed — session min/max values now persist until reboot only.

---

### Changed

#### `AttitudeUI` — 2-view cycling (ATTITUDE ↔ MINMAX)

Knob button press now toggles between ATTITUDE and MINMAX views. Previously cycled through three views (ATTITUDE → MINMAX → LEVELING → ATTITUDE).

`CONNECTION_TIMEOUT_MS` increased from 3000 → 5000 ms.

---

### Added

#### CompassScreen — GNSS data integration (UBLOX-ESP32-SignalK-gateway)

`GnssDelta` packets (`GNSS_DELTA = 4`) are now received and processed. `ESPNowReceiver` dispatches the new message type, converts the wire payload to an internal `GnssData` struct, and exposes it via `hasNewGnssData()` / `getGnssData()` — identical pattern to existing `WeatherDelta` and `BatteryDelta` handling.

---

#### CompassScreen — 3-view cycling (HEADING → COG → SOG)

Knob button press cycles through three views. `CompassView` enum drives a `showView()` method that controls container visibility, mode label text, and render cache resets. Active view is persisted to NVS on `onLeave()` (namespace `"compass"`, key `"view"`).

| View | Visible container | LabelHeadingMode | Data source |
|------|-------------------|------------------|-------------|
| HEADING | ContainerCompass | `HDG(T)` | CMPS14 (`HeadingDelta`) |
| COG | ContainerCompass | `COG(T)` | GNSS (`GnssDelta`) |
| SOG | ContainerSog | — (hidden) | GNSS (`GnssDelta`) |

---

#### CompassScreen — HEADING view

Shows true heading from CMPS14. Compass rose rotates to HDG(T). Label format: 3-digit with leading zero, e.g. `090°`. On disconnect, last known heading and rose position are preserved (no reset to dashes) — consistent with v3.1.x behavior.

Connection indicator (`PanelConnected`) tracks CMPS14 packet timing (`HEADING_TIMEOUT_MS = 5000 ms`).

---

#### CompassScreen — COG view

Shows course over ground (true) from GNSS. Compass rose rotates to COG(T) using the same deadband logic as HEADING view (`ROTATION_THRESHOLD_X10 = 5`). Label format: 3-digit with leading zero, e.g. `090°`.

Shows `---°` when `fix_ok = 0` (no valid GNSS fix) or when GNSS sender is disconnected.

Connection indicator tracks GNSS packet timing (`GNSS_TIMEOUT_MS = 5000 ms`).

---

#### CompassScreen — SOG view

Shows speed over ground as a speedometer. Arc (`ui_ArcSog`) range `0–100` maps to `0.0–10.0 kn` (value = knots × 10). Arc is clamped to 100 for speeds above 10 kn; label always shows true value. Label format: one decimal place, e.g. `7.1`.

Shows `--.-` when `fix_ok = 0` or when GNSS sender is disconnected.

Connection indicator tracks GNSS packet timing.

---

### Fixed

#### `BatteryUI` / `WeatherUI` — NaN values rendered as `"nan"` in UI labels

If a sender transmits a `NaN` float (e.g. before a measurement is available), `snprintf("%.1fV", NAN)` produces the string `"nan"` which LVGL renders directly.

Fix: `isnan()` guard added at the top of every `update*` method in both classes. A `NaN` input causes an early return — the label retains its previous value (or `"---"` if no valid reading has arrived yet), min/max tracking is not updated, and the EMA state is not corrupted.

Affected methods: `BatteryUI::updateHouseVoltage()`, `updateHouseCurrent()`, `updateHouseSoc()`, `updateStartVoltage()`; `WeatherUI::updateTemperature()`, `updatePressure()`, `updateHumidity()`.

---

### Changed

#### `espnow_protocol.h` — `GnssData` internal struct and `GNSS_DELTA` message type added

`ESPNowMsgType::GNSS_DELTA = 4` added to the enum.

`GnssData` internal struct (scaled integers, analogous to `HeadingData`):

```cpp
struct GnssData {
    uint16_t cog_true_x10;   // COG true 0–3599 (0.0°–359.9°)
    uint16_t sog_knots_x10;  // SOG in knots × 10 (e.g. 72 = 7.2 kn)
    uint8_t  fix_ok;         // 1 = valid fix (getGnssFixOk())
};
```

`convertGnssDeltaToData()` converts the wire `GnssDelta` (float radians / m·s⁻¹) to `GnssData`:
- `cog_true_rad` → `cog_true_x10` via `RAD_TO_DEG_X10`
- `sog_ms` → `sog_knots_x10` via `MS_TO_KNOTS_X10 = 1.94384 × 10`
- `fix_ok` copied verbatim

Sent by UBLOX-ESP32-SignalK-gateway. Wire packet: 32 B (`ESPNowHeader` 8 B + `GnssDelta` 24 B).

---

#### `ESPNowReceiver` — `GNSS_DELTA` dispatch added

`onDataRecv()` switch extended with `GNSS_DELTA` case — converts `GnssDelta` to `GnssData` via `convertGnssDeltaToData()`, stores in `s_latest_gnss`, sets `s_has_new_gnss = true`. Does not update `s_last_rx_millis` / `s_packet_count` (those are CMPS14 connection indicators; `CompassUI` tracks GNSS timing independently via `_last_gnss_millis`).

Added `hasNewGnssData() const` — thread-safe read of `s_has_new_gnss`.

Added `getGnssData()` — thread-safe read of `s_latest_gnss`, clears `s_has_new_gnss`.

Added `s_latest_gnss` / `s_has_new_gnss` `inline static` members.

Constructor made `explicit`. Default channel parameter of `begin()` changed from `1` → `6` (matches the fixed router channel used by all companion gateways).

---

#### `CompassUI` — refactored from toggle to 3-view cycle

Previous behavior: knob button toggled between HDG(T) and HDG(M). Magnetic heading mode removed.

New behavior: knob button cycles HEADING → COG → SOG → HEADING (modulo). `CompassView` enum replaces the `_use_true_heading` bool.

`_last_heading_x10` / `_last_heading_deg` / `_last_is_true` replaced by:
- `_last_rose_x10` — single cache for currently rendered compass rose rotation (reset on view switch, shared by HEADING and COG views)
- `_last_label_deg` — heading/COG label cache (reset on view switch)
- `_last_sog_x10` — SOG arc/label cache
- `_last_gnss_millis` / `_last_gnss_fix` — GNSS connection tracking

`showView()` is the single point of control for container visibility, mode label, cache resets, and forcing the connection dot to red on view switch.

`onLeave()` added — saves active view to NVS (namespace `"compass"`, key `"view"`). Default on first boot: `HEADING`.

`<Preferences.h>` include added to `CompassUI.h`.

---

## [v3.1.1] - 2026-04-06

### Changed

Updated README and UML class diagrams.

## [v3.1.0] - 2026-03-30

### Added

#### AttitudeScreen — 3-view cycling (ATTITUDE → MINMAX → LEVELING)

Knob button press cycles through three internal views. `AttitudeView` enum drives a `showView()` method that is the single point of control for all container visibility. `onLeave()` always resets to ATTITUDE view.

| View | Visible containers |
|---|---|
| ATTITUDE | ContainerHorizonGroup, ContainerAttitudeGroup, ContainerVessel |
| MINMAX | ContainerMinMax, ContainerVessel, roll-line container |
| LEVELING | ContainerLevelingDialog |

---

#### AttitudeScreen — session min/max tracking for pitch and roll

Runtime-only session min/max for pitch and roll (resets on reboot, not persisted to NVS). Tracked as `int16_t` in tenths-of-degrees. Sentinel value `0x7FFF` indicates no data yet (impossible for pitch ±900 or roll ±1800).

---

#### AttitudeScreen — MINMAX view

Four static colored horizon lines show session extremes:

| Line | Color | Meaning |
|---|---|---|
| PanelMaxPitch | Yellow | Highest pitch (bow up) |
| PanelMinPitch | Blue | Lowest pitch (bow down) |
| _img_max_roll | Green | Maximum roll (starboard) |
| _img_min_roll | Red | Minimum roll (port) |

Pitch lines (`PanelMaxPitch`, `PanelMinPitch`) are SquareLine-generated `lv_obj` panels displaced vertically with `lv_obj_set_y()`. Roll lines are programmatic `lv_image` objects rotated with `lv_image_set_rotation()` (see bug fix below).

Visual positions are clamped to ±30° for readability (`MINMAX_LINE_CLAMP_X10 = 300`). Numeric labels always show the true session values without clamping.

Live horizon (`ContainerHorizonGroup`) is hidden in MINMAX view.

---

#### AttitudeScreen — LEVELING view with auto-countdown

Entering LEVELING view starts a 5-second countdown. The countdown label (`LabelLevelingDialog`) updates every second. At zero the level command is sent automatically via `ESPNowReceiver::sendLevelCommand()`. Knob press or carousel rotation cancels immediately and returns to ATTITUDE view. States: `IDLE → COUNTDOWN → SENDING → SUCCESS/FAILED → ATTITUDE`.

On `SUCCESS`, all four session min/max values are reset to `SENTINEL` — the sensor's reference point has changed so the old extremes are no longer meaningful. Cancellation and `FAILED` leave min/max values intact.

[Bubble level icons created by vectorsmarket15 - Flaticon](https://www.flaticon.com/free-icons/bubble-level)

---

### Changed

#### AttitudeScreen — disconnect behavior

On disconnect only `PanelStarboard` and `PanelPortside` (navigation lights) are hidden. Last known pitch/roll values and horizon line position are preserved — no "---" reset and no horizon snap to neutral.

---

#### AttitudeScreen — all three horizon lines are now programmatic `lv_image` objects

`ImageHorizon` was removed from SquareLine and is now created in `AttitudeUI::begin()` alongside the two roll min/max lines. All three share a single `lv_image_dsc_t` (`s_horizonline_dsc`) backed by a 5 440-byte static SRAM buffer (680×4 px, `LV_COLOR_FORMAT_RGB565`, all `0xFF` = white). No PNG asset file is needed. Roll and pitch min/max line colors are applied via `lv_obj_set_style_image_recolor_opa(255)`.

Moving `ImageHorizon` to code ensures `LV_IMAGE_ALIGN_DEFAULT` and the correct pivot `(340, 2)` are set from the start, replacing the two fixup calls that were previously needed in `begin()`.

---

### Fixed

#### AttitudeScreen — LVGL 9 PARTIAL mode freeze caused by `lv_obj_set_style_transform_rotation()`

`lv_obj_set_style_transform_rotation()` on a plain `lv_obj` (the SquareLine-generated `PanelMaxRoll`/`PanelMinRoll`, 484×4 px) causes `lv_timer_handler()` to never return in LVGL 9 PARTIAL rendering mode, freezing the device. Root cause: LVGL 9 PARTIAL mode must iterate render bands to find dirty regions for a transformed object; a 484-px-wide object in a 480-px display triggers a degenerate band-search loop.

Fix: `PanelMaxRoll` and `PanelMinRoll` were removed from SquareLine. Roll min/max lines are replaced by programmatic `lv_image` objects using `lv_image_set_rotation()`, which is the same API used for the live horizon line and is confirmed safe in LVGL 9 PARTIAL mode.

## [v3.0.0] - 2026-03-15

### Changed

#### Library upgrades — ESP32 Core 2.0.14 → 3.3.7, LVGL 8.3.6 → 9.5.0, Arduino GFX 1.3.1 → 1.6.5

Major library upgrade across all three core dependencies. All changes are backwards-incompatible at the API level. No functional behavior visible to the end user is changed.

---

#### Arduino GFX Library 1.3.1 → 1.6.5 — display driver restructured

`Arduino_ST7701_RGBPanel` is not available in the Arduino Library Manager distribution of version 1.6.5. Replaced with the equivalent three-object structure:

```cpp
// v2.x (Arduino_GFX 1.3.1)
Arduino_ESP32RGBPanel _bus;
Arduino_ST7701_RGBPanel _gfx;

// v3.0.0 (Arduino_GFX 1.6.5)
Arduino_SWSPI _init_bus;       // SPI init-command bus for ST7701S
Arduino_ESP32RGBPanel _bus;    // RGB parallel data bus
Arduino_RGB_Display _gfx;      // display object
```

`Arduino_SWSPI` handles the SPI command/data sequence sent to ST7701S on startup (CS=16, SCK=2, MOSI=1). `Arduino_RGB_Display::begin()` now returns `bool` — halts in `while(1)` if initialization fails.

---

#### `Crowpanel_ST7701_Init.h/.cpp` — new project-specific ST7701S init table

The GFX library's built-in `st7701_type5` init sequence sets `0x36 = 0x08` (BGR byte order), causing channels to render in wrong order. A project-specific init table is added to the repository with two corrected values:

```cpp
WRITE_C8_D8, 0x36, 0x00,  // was 0x08 (BGR) → 0x00 (RGB)
WRITE_C8_D8, 0x3A, 0x60,  // RGB666 mode (for unknown reason 0x50 = RGB565 caused wrong color order)
```

All other entries are identical to `st7701_type5`. The constructor references this table directly:

```cpp
_gfx(480, 480, &_bus, 0, true,
     &_init_bus, GFX_NOT_DEFINED,
     crowpanel_st7701_type5_init_operations,
     crowpanel_st7701_type5_init_operations_len)
```

---

#### `_bus` constructor — timing parameters and pixel clock

`Arduino_ESP32RGBPanel` constructor extended with explicit trailing parameters. All values are significant for display stability.

```cpp
_bus(40 /* DE */, 7 /* VSYNC */, 15 /* HSYNC */, 41 /* PCLK */,
     /* R4–R0, G5–G0, B4–B0 pins ... */
     1, 10, 4, 24,   /* hsync: polarity, front_porch, pulse_width, back_porch */
     1, 10, 4, 24,   /* vsync: polarity, front_porch, pulse_width, back_porch */
     0  /* pclk_active_neg */,
     10000000 /* prefer_speed: 10 MHz pixel clock */,
     false /* useBigEndian */,
     0 /* de_idle_high */, 0 /* pclk_idle_high */,
     4800 /* bounce_buffer_size_px */)
```

**`hsync_polarity = 1` / `vsync_polarity = 1`:** Must be 1 (active-high). Polarity 0 caused a random vertical stripe pattern across the entire display — the RGB panel DMA read the framebuffer offset from the correct position relative to the sync edges.

**`hsync_back_porch = 24` / `vsync_back_porch = 24`:** Empirically tuned; values in the range 20–24 affect display stability on this panel. Back porch contributes to H/V total pixel count and thus the effective frame rate at a given pixel clock.

**`prefer_speed = 10000000` (10 MHz):** Overrides the library default (12 MHz for OPI PSRAM targets). The lower clock rate was found empirically to reduce display glitching. The Elecrow CrowPanel 2.1" uses ESP32-S3 with OPI PSRAM; without `prefer_speed`, the library defaults to 12 MHz (`#ifndef CONFIG_SPIRAM_MODE_QUAD`).

**`bounce_buffer_size_px = 4800` (10 lines × 480 px, 9 600 bytes SRAM):** LCD_CAM DMA seems to read the RGB565 framebuffer directly from PSRAM (`fb_in_psram = true` in `esp_lcd_rgb_panel_config_t`). Under PSRAM bus load (CPU writes, heap allocations, WiFi stack), this probably caused wait-cycles in the DMA stream that manifested as a brief periodic display shift. With a non-zero `bounce_buffer_size_px`, the ESP-IDF RGB panel driver maintains a small SRAM staging buffer filled continuously by a background GDMA channel; the LCD DMA reads from SRAM only. Confirmed to eliminate the periodic visual glitch that occurred even with LVGL fully disabled.

---

#### ESP32 Core 3.x — LEDC PWM API (`CrowPanelApplication.cpp`, `BrightnessUI.cpp`)

```cpp
// v2.x (Core 2.x)
ledcSetup(PWM_CHANNEL, PWM_FREQ, PWM_RESOLUTION);
ledcAttachPin(SCREEN_BACKLIGHT_PIN, PWM_CHANNEL);
ledcWrite(PWM_CHANNEL, duty);

// v3.0.0 (Core 3.x)
ledcAttach(SCREEN_BACKLIGHT_PIN, PWM_FREQ, PWM_RESOLUTION);
ledcWrite(SCREEN_BACKLIGHT_PIN, duty);  // pin, not channel
```

`_pwm_channel` member removed from `BrightnessUI`; constructor parameter changed from `int pwm_channel` to `uint8_t backlight_pin`.

---

#### ESP32 Core 3.x — ESP-NOW receive callback (`ESPNowReceiver.h/.cpp`)

```cpp
// v2.x (Core 2.x)
static void onDataRecv(const uint8_t* mac_addr, const uint8_t* data, int data_len);

// v3.0.0 (Core 3.x)
static void onDataRecv(const esp_now_recv_info_t* recv_info, const uint8_t* data, int data_len);
```

Callback body unchanged — `mac_addr` was not used. Coordinated with CMPS14-ESP32-SignalK-gateway which already uses the Core 3.x signature.

---

#### LVGL 8 → 9 — display driver API rewritten (`CrowPanelApplication.h/.cpp`)

Buffer type corrected: `lv_color_t` is 3 bytes in LVGL 9 (RGB888 internal format); draw buffer must be `uint16_t` for RGB565.

```cpp
// v2.x (LVGL 8)
lv_color_t* _buf1;
_buf1 = (lv_color_t*)heap_caps_malloc(sizeof(lv_color_t) * BUF_PIXELS, MALLOC_CAP_INTERNAL);
lv_disp_draw_buf_init(&_draw_buf, _buf1, NULL, BUF_PIXELS);
lv_disp_drv_t disp_drv;
lv_disp_drv_init(&disp_drv);
disp_drv.flush_cb = lvglFlushCb;
disp_drv.draw_buf = &_draw_buf;
disp_drv.user_data = &_gfx;
lv_disp_drv_register(&disp_drv);

// v3.0.0 (LVGL 9)
uint16_t* _buf1;
_buf1 = (uint16_t*)heap_caps_malloc(sizeof(uint16_t) * BUF_PIXELS, MALLOC_CAP_INTERNAL);
_lvgl_disp = lv_display_create(SCREEN_WIDTH, SCREEN_HEIGHT);
lv_display_set_flush_cb(_lvgl_disp, lvglFlushCb);
lv_display_set_buffers(_lvgl_disp, _buf1, nullptr,
                       sizeof(uint16_t) * BUF_PIXELS,
                       LV_DISPLAY_RENDER_MODE_PARTIAL);
lv_display_set_user_data(_lvgl_disp, &_gfx);
```

DIRECT rendering mode (`LV_DISPLAY_RENDER_MODE_DIRECT`) was evaluated but is unusable on this hardware, screen flickering was severe.

---

#### LVGL 8 → 9 — flush callback (`CrowPanelApplication.cpp`)

```cpp
// v2.x (LVGL 8)
static void lvglFlushCb(lv_disp_drv_t* disp, const lv_area_t* area, lv_color_t* color_p) {
    auto* gfx = static_cast<Arduino_ST7701_RGBPanel*>(disp->user_data);
    gfx->draw16bitRGBBitmap(area->x1, area->y1, (uint16_t*)&color_p->full, w, h);
    lv_disp_flush_ready(disp);
}

// v3.0.0 (LVGL 9)
static void lvglFlushCb(lv_display_t* disp, const lv_area_t* area, uint8_t* px_map) {
    auto* gfx = static_cast<Arduino_RGB_Display*>(lv_display_get_user_data(disp));
    gfx->draw16bitRGBBitmap(area->x1, area->y1, (uint16_t*)px_map, w, h);
    lv_display_flush_ready(disp);
}
```

---

#### LVGL 8 → 9 — tick source (`CrowPanelApplication.cpp`)

`LV_TICK_CUSTOM = 1` macro and `LV_TICK_CUSTOM_SYS_TIME_EXPR = (millis())` no longer exist in LVGL 9's `lv_conf.h`. Replaced with a runtime call:

```cpp
lv_tick_set_cb(millis);  // called once in initLvgl()
```

---

#### LVGL 8 → 9 — image widget API (`CompassUI.cpp`, `AttitudeUI.cpp`)

| LVGL 8 | LVGL 9 |
|--------|--------|
| `lv_img_set_zoom(img, 512)` | `lv_image_set_scale(img, 512)` |
| `lv_img_set_angle(img, angle)` | `lv_image_set_rotation(img, angle)` |
| `lv_img_set_antialias(img, false)` | `lv_image_set_antialias(img, false)` |
| `lv_img_set_pivot(img, x, y)` | `lv_image_set_pivot(img, x, y)` |

---

#### LVGL 9 — `lv_conf.h` relocated to project root

`lv_conf.h` moved to the project root directory. The Arduino-LVGL integration finds it via `__has_include` before falling back to the version bundled in `libraries/`. All values in the project-level file match LVGL 9 defaults; the file is committed to the repository.

---

#### LVGL 9 — SquareLine Studio re-export (LVGL 9.3 target)

All SquareLine-generated files (`ui.h/.c`, `ui_*Screen.h/.c`, `ui_helpers.h/.c`) re-exported targeting LVGL 9.3. Generated code uses `lv_image_*` API throughout.

---

#### AttitudeScreen

Ship silhouette scaled bigger for better readability. The red and green small panels (navigation lights) on starboard and portside of the ship silhouette are hidden when disconnected and show again when new data received from the compass.

---

### Fixed

#### `AttitudeUI` — horizon line rotation (roll axis) not working in LVGL 9

**Root cause:** SquareLine Studio 1.6.0 with LVGL 9 target emits `lv_image_set_inner_align(ui_ImageHorizon, LV_IMAGE_ALIGN_TILE)` for the horizon line image. In LVGL 9, the TILE draw path does not apply `img->rotation` to the draw descriptor — `lv_image_set_rotation()` calls have no effect in TILE mode. This was not an issue in LVGL 8 where `lv_image_set_inner_align()` did not exist.

**Fix** in `AttitudeUI::begin()`:
```cpp
lv_image_set_inner_align(ui_ImageHorizon, LV_IMAGE_ALIGN_DEFAULT); // override TILE
lv_image_set_pivot(ui_ImageHorizon, 340, 2);
```

---

#### `AttitudeUI` — horizon line rotation pivot at wrong position after LVGL 9 migration

**Root cause:** In LVGL 9, `lv_image_set_inner_align()` silently resets the image pivot to (0, 0) when called. If `lv_image_set_pivot()` is called first, the pivot is subsequently discarded.

**Fix:** Call order enforced in `AttitudeUI::begin()` — `lv_image_set_inner_align()` must precede `lv_image_set_pivot()`:

```cpp
lv_image_set_inner_align(ui_ImageHorizon, LV_IMAGE_ALIGN_DEFAULT); // FIRST — resets pivot to (0,0)
lv_image_set_pivot(ui_ImageHorizon, 340, 2);                        // SECOND — sets final pivot
```

Pivot geometry: `ui_ImageHorizon` is 680×4 px positioned at (−100, 238) on the screen. Pivot (340, 2) in widget-local coordinates maps to screen coordinate (240, 240) — exact screen center.

---

### Performance

LVGL 9 PARTIAL rendering performance is somewhat identical to LVGL 8.

### Developer Notes

#### `_bus` constructor parameter reference

```
Arduino_ESP32RGBPanel(
    de, vsync, hsync, pclk,
    r0–r4, g0–g5, b0–b4,        // 16 data pins
    hsync_polarity, hsync_front_porch, hsync_pulse_width, hsync_back_porch,
    vsync_polarity, vsync_front_porch, vsync_pulse_width, vsync_back_porch,
    pclk_active_neg,             // 0 = sample on rising edge
    prefer_speed,                // pixel clock Hz; GFX_NOT_DEFINED = 12 MHz (OPI PSRAM)
    useBigEndian,                // false = normal RGB565 GPIO bit order
    de_idle_high,                // 0 = DE idles low
    pclk_idle_high,              // 0 = PCLK idles low
    bounce_buffer_size_px)       // >0 = SRAM bounce buffer for LCD DMA
```

---

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

[v4.1.0]: https://github.com/mkvesala/ESP32-Crowpanel-compass/releases/tag/v4.1.0
[v4.0.0]: https://github.com/mkvesala/ESP32-Crowpanel-compass/releases/tag/v4.0.0
[v3.1.1]: https://github.com/mkvesala/ESP32-Crowpanel-compass/releases/tag/v3.1.1
[v3.1.0]: https://github.com/mkvesala/ESP32-Crowpanel-compass/releases/tag/v3.1.0
[v3.0.0]: https://github.com/mkvesala/ESP32-Crowpanel-compass/releases/tag/v3.0.0
[v2.1.0]: https://github.com/mkvesala/ESP32-Crowpanel-compass/releases/tag/v2.1.0
[v2.0.0]: https://github.com/mkvesala/ESP32-Crowpanel-compass/releases/tag/v2.0.0
[v1.0.0]: https://github.com/mkvesala/ESP32-Crowpanel-compass/releases/tag/v1.0.0
[v0.4.0]: https://github.com/mkvesala/ESP32-Crowpanel-compass/releases/tag/v0.4.0
[v0.3.0]: https://github.com/mkvesala/ESP32-Crowpanel-compass/releases/tag/v0.3.0
[v0.2.0]: https://github.com/mkvesala/ESP32-Crowpanel-compass/releases/tag/v0.2.0
[v0.1.0]: https://github.com/mkvesala/ESP32-Crowpanel-compass/releases/tag/v0.1.0
[v0.0.2]: https://github.com/mkvesala/ESP32-Crowpanel-compass/releases/tag/v0.0.2
[v0.0.1]: https://github.com/mkvesala/ESP32-Crowpanel-compass/releases/tag/v0.0.1 
