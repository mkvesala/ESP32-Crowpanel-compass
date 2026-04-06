![Logo](docs/projectlogo.svg)

# ESP32 CrowPanel Compass & Multi-Function Display

[![Platform: ESP32-S3](https://img.shields.io/badge/Platform-ESP32--S3-blue)](https://www.espressif.com/en/sdks/esp-arduino)
[![Display: CrowPanel 2.1" Rotary Knob](https://img.shields.io/badge/Display-CrowPanel%202.1%22%20Rotary%20Knob-lightgrey)](https://www.elecrow.com/wiki/CrowPanel_2.1inch-HMI_ESP32_Rotary_Display_480_IPS_Round_Touch_Knob_Screen.html)
[![Protocol: ESP-NOW](https://img.shields.io/badge/Protocol-ESP--NOW-orange)](https://www.espressif.com/en/solutions/low-power-solutions/esp-now)
[![License: MIT](https://img.shields.io/badge/License-MIT-green.svg)](LICENSE)
[![UI: LVGL9](https://img.shields.io/badge/UI-LVGL9-black)](https://lvgl.io)

Marine instrument display for [Elecrow CrowPanel 2.1" HMI](https://www.elecrow.com/wiki/CrowPanel_2.1inch-HMI_ESP32_Rotary_Display_480_IPS_Round_Touch_Knob_Screen.html) (ESP32-S3, 480×480 IPS round touchscreen, rotary knob). Receives via ESP-NOW:
- Compass heading, pitch and roll from [CMPS14-ESP32-SignalK-gateway](https://github.com/mkvesala/CMPS14-ESP32-SignalK-gateway) compass
- Temperature, air pressure and relative humidity from [BME280-ESP32-SignalK-gateway](https://github.com/mkvesala/BME280-ESP32-SignalK-gateway)
- House battery bank voltage, current and SoC as well as starter battery voltage from a VEDirect based sender

Displays values on a round LVGL UI. User interaction via rotary knob (rotate or press). No touch screen implementation yet.

Different screens selectable by rotating the knob:
- **Compass screen** — rotating compass rose, heading value, True/Magnetic toggle
- **Attitude screen** — artificial horizon, pitch and roll values, pitch and roll min/max values, attitude leveling
- **Weather screen** — toggle between temperature, pressure and humidity views
- **Battery screen** - toggle between house voltage, house current, house SoC and starter voltage views
- **Brightness screen** — backlight brightness adjustment with NVS persistence

Developed and tested on:
- [Elecrow CrowPanel 2.1" HMI ESP32 Rotary Display](https://www.elecrow.com/wiki/CrowPanel_2.1inch-HMI_ESP32_Rotary_Display_480_IPS_Round_Touch_Knob_Screen.html)
- [ESP32 board package](https://github.com/espressif/arduino-esp32) (3.3.7)
- [Arduino IDE](https://www.arduino.cc/en/software/) (2.3.8)
- [LVGL](https://lvgl.io/) (9.5.0)
- [Arduino GFX Library](https://github.com/moononournation/Arduino_GFX) (1.6.5)
- [PCF8574 Library](https://github.com/xreef/PCF8574_library) (2.4.0)
- [SquareLine Studio](https://squareline.io/) (1.6.0) for UI design

Integrated via ESP-NOW with:
- [CMPS14-ESP32-SignalK-gateway](https://github.com/mkvesala/CMPS14-ESP32-SignalK-gateway) (v1.3.0) compass sender
- [BME280-ESP32-SignalK-gateway](https://github.com/mkvesala/BME280-ESP32-SignalK-gateway) (v1.0.0) weather data sender
- VEDirect-ESP32-SignalK-gateway (v1.0.0) battery data sender

## Purpose of the project

This is one of my individual digital boat projects. Use at your own risk. Not for safety-critical navigation.

1. I needed a compact multi-function display near the helm, receiving data wirelessly from the compass and other devices, independently from WiFi and SignalK
2. I wanted to learn LVGL and SquareLine Studio for UI development
3. I continued learning ESP32 C++ patterns and FreeRTOS from the companion compass project

## Release history

| Release | Comment |
|---------|---------|
| v3.1.1 | Latest release. Patching documentation only. |
| v3.1.0 | AttitudeScreen redesigned with separate views for real-time attitude, min/max tracking and for performing attitude leveling (now triggered by count-down, canceled by button press/rotate). See [CHANGELOG](CHANGELOG.md) for details.
| v3.0.0 | Library upgrade: ESP32 board package 2.0.14 → 3.3.7, LVGL 8.3.6 → 9.5.0, Arduino GFX Library 1.3.1 → 1.6.5. This is a compatibility change - v2.1.0 does not compile on the new libraries. See [CHANGELOG](CHANGELOG.md) for details.
| v2.1.0 | Introduces BatteryScreen and `BatteryUI` UI adapter class. Minor modifications to WeatherScreen and `WeatherUI`. See [CHANGELOG](CHANGELOG.md) for details.
| v2.0.0 | Refactored for scalability in screen management. Introduces `IScreenUI` interface as an abstract base class for the actual UI adapter classes. Breaking change in ESP-NOW protocol: updated with framed packets, introducing `ESPNowPacket` and `ESPNowHeader` structs. Adds `WeatherUI` UI adapter class and WeatherScreen UI to show temperature, humidity and pressure. See [CHANGELOG](CHANGELOG.md) for details. |
| v1.0.0 | First stable release. See [CHANGELOG](CHANGELOG.md) for details - including pre-releases. |

## Classes

Class diagram including the companion projects:

<img src="docs/full_uml_diagram.jpeg" width="480">

**`CrowPanelApplication`:**
- Owns: `Arduino_ESP32RGBPanel`, `Arduino_RGB_Display`, `Arduino_SWSPI`, `PCF8574`, `ESPNowReceiver`, `CompassUI`, `AttitudeUI`, `WeatherUI`, `BatteryUI`, `BrightnessUI`, `RotaryEncoder`, `ScreenManager`
- Responsible for: orchestrating everything within the main program

**`ESPNowReceiver`:**
- Responsible for: receiving `HeadingData` broadcasts and sending attitude leveling commands via ESP-NOW
- Owned by: `CrowPanelApplication`

**`RotaryEncoder`:**
- Uses: `PCF8574`
- Responsible for: reading rotary knob rotation and knob button press
- Owned by: `CrowPanelApplication`

**`IScreenUI`:**
- Abstract base class for UI adapter class implementations

**`CompassUI`:**
- Realizes: `IScreenUI`
- Uses: `ESPNowReceiver`
- Responsible for: updating LVGL UI objects on the compass screen based on heading data
- Owned by: `CrowPanelApplication`

**`AttitudeUI`:**
- Realizes: `IScreenUI`
- Uses: `ESPNowReceiver`
- Responsible for: updating LVGL UI objects on the attitude screen based on pitch and roll data
- Owned by: `CrowPanelApplication`

**`WeatherUI`:**
- Realizes: `IScreenUI`
- Uses: `ESPNowReceiver`
- Responsible for: updating LVGL UI objects on the weather screen based on temperature, pressure and humidity data.
- Owned by: `CrowPanelApplication`  

**`BatteryUI`:**
- Realizes: `IScreenUI`
- Uses: `ESPNowReceiver`
- Responsible for: updating LVGL UI objects on the battery screen based on battery data.
- Owned by: `CrowPanelApplication`  

**`BrightnessUI`:**
- Realizes: `IScreenUI`
- Uses: `Preferences`
- Responsible for: backlight brightness adjustment with NVS persistence, updating LVGL UI objects on the brightness screen
- Owned by: `CrowPanelApplication`

**`ScreenManager`:**
- Depends on: `IScreenUI*`
- Responsible for: Screen carousel management
- Owned by: `CrowPanelApplication`

## Features

### Compass screen

<img src="docs/compassscreen.png" height="240"> <img src="docs/compassui.jpeg" height="240">

- Rotating compass rose image (240x240 px source, rendered at 480x480 with LVGL zoom=512, no alpha, antialias off)
- Heading value label
- True/Magnetic heading mode toggle with knob button press
- T/M mode indicator label
- Connected indicator panel (black = connected, red = disconnected)
- Rotation threshold 0.5°: skips LVGL re-render when heading change is below threshold

### Attitude screen

<img src="docs/attitudescreen1.png" height="240"> <img src="docs/attitudescreen2.png" height="240"> <img src="docs/attitudescreen3.png" height="240"> <img src="docs/attitudeui.jpeg" height="240"> <img src="docs/attitudeui2.jpeg" height="240"> <img src="docs/attitudeui3.jpeg" height="240">

- Pitch and roll min/max values recorded runtime, no persistent storage in NVS
- Pressing the knob button toggles between ATTITUDE → MINMAX → LEVELING → ATTITUDE view
- Returning to the screen always loads ATTITUDE view
- ATTITUDE view:
  - Artificial horizon: white 680 x 4 px image that rotates and translates based on pitch and roll
  - Pitch and roll value labels
- MINMAX view:
  - Four horizon lines, all 680 x 4 px images, to show recorded pitch and roll min/max values
    - Yellow, placed horizontally to the max pitch, showing highest bow up position
    - Blue, placed horizontally to the min pitch, showing lowest bow down position
    - Green, pivot at the center, rotated to show max roll, furthest roll position to starboard
    - Red, pivot at the center, rotated to show min roll, furthest roll position to port side
  - Pitch and roll min/max value labels
- LEVELING view:
  - Bubble leveling tool 120 x 120 px icon 
  - Count-down to leveling (default 5 s, set in `LEVELING_COUNTDOWN_MS` constant
  - Knob button press or rotation cancels count-down and returns to ATTITUDE view or switches to another screen 
  - When executing the command, "Leveling..." message is shown
  - "Success!" or "Failed!" message shown based on the success of the leveling operation
  - Successful leveling resets pitch and roll min/max values
- Ship silhouette overlay on ATTITUDE and MINMAX view
  - The red and green "navigation lights" of the ship silhouette hidden when disconnected, shown again when data received from the compass

### Weather screen

<img src="docs/weatherscreen1.png" height="240"> <img src="docs/weatherui1.jpeg" height="240"> <img src="docs/weatherscreen2.png" height="240"> <img src="docs/weatherui2.jpeg" height="240"> <img src="docs/weatherscreen3.png" height="240"> <img src="docs/weatherui3.jpeg" height="240">

- Pressing the knob button toggles between TEMPERATURE → PRESSURE → HUMIDITY → TEMPERATURE view
- Last view stored in NVS `onLeave()`
- Stored view retrieved from NVS when returning to the screen (default: temperature)
- Temperature view: Temperature °C, maximum and minimum
- Pressure view: Pressure hPA, maximum and minimum
- Humidity view: Humidity %, maximum and minimum
- Min and max values are runtime only, not persistent in NVS
- Trend indicators based on EMA. Alpha (0.05) and threshold (0.001) can be adjusted via constants for each view separately

### Battery screen

<img src="docs/batteryscreenhousev.png" height="240"> <img src="docs/batteryuihousev.jpeg" height="240"> <img src="docs/batteryscreenhousea.png" height="240"> <img src="docs/batteryuihousea.jpeg" height="240"> <img src="docs/batteryscreenhousesoc.png" height="240"> <img src="docs/batteryuihousesoc.jpeg" height="240"> <img src="docs/batteryscreenstart.png" height="240"> <img src="docs/batteryuistart.jpeg" height="240">

- Pressing the knob button toggles between HOUSE VOLTAGE → HOUSE CURRENT → HOUSE SOC → STARTER VOLTAGE → HOUSE VOLTAGE view
- Last view stored in NVS `onLeave()`
- Stored view retrieved from NVS when returning to the screen (default: house voltage)
- House voltage view: voltage V, maximum and minimum
- House current view: current A, maximum and minimum
- House SoC view: state-of-charge %, maximum and minimum
- Min and max values are runtime only, not persistent in NVS
- Trend indicators based on EMA. Alpha (0.05) and threshold (0.001) can be adjusted via constants for each view separately

### Brightness screen

<img src="docs/brightnessscreen.png" height="240"> <img src="docs/brightnessui.jpeg" height="240">

- Sun icon image and current brightness percentage label
- Knob button press enters ADJUSTING mode: arc overlay appears
- Knob rotation in ADJUSTING mode: ±2% brightness, updates arc, label and backlight in real-time
- 3-second timeout after last rotation → saves to NVS and returns to idle
- Brightness range: 2%–100% (2% minimum prevents screen going completely dark)
- Default: 48% (~122/255)
- Persistence: ESP32 Preferences (NVS), namespace `"display"`, key `"brightness"`
- PWM: GPIO6, 5 kHz, 8-bit

### Knob behavior

| Screen | Button press | Rotation (normal) | Rotation (special) |
|--------|--------------|-------------------|--------------------|
| Compass | Toggle T/M heading mode | Switch screen | — |
| Attitude | Toggle ATTITUDE/MINMAX/LEVELING view | Switch screen | — |
| Weather | Toggle TEMPERATURE/PRESSURE/HUMIDITY view | Switch screen | — |
| Battery | Toggle HOUSE VOLTAGE/HOUSE CURRENT/HOUSE SOC/STARTER VOLTAGE view | Switch screen | - |
| Brightness | Enter ADJUSTING mode | Switch screen | ±2% brightness (ADJUSTING mode only) |

Screen carousel order:
- **Clockwise:** COMPASS → ATTITUDE → WEATHER → BATTERY → ... → BRIGHTNESS → COMPASS
- **Counter-clockwise:** COMPASS → BRIGHTNESS → ... → BATTERY → WEATHER → ATTITUDE → COMPASS

Screen carousel is scalable, new screens may be added.

### ESP-NOW communication

All ESP-NOW messages are wrapped in the payload of `ESPNowPacket`.
```cpp
template <typename TPayload>
  struct ESPNowPacket {
    ESPNowHeader hdr;
    TPayload payload;
} __attribute__((packed));
```

`ESPNowHeader` contains `ESPNOW_MAGIC = 0x45534E57' ("ESNW") which identifies the packets from others on the same channel.

```cpp
struct ESPNowHeader {
   uint32_t magic;           // ESPNOW_MAGIC ('E''S''N''W')
   uint8_t  msg_type;        // ESPNowMsgType
   uint8_t  payload_len;     // payload length in bytes (max 250)
   uint8_t  reserved[2];     // padding, set to zero
} __attribute__((packed));
```

`ESPNowMsgType` identifies the content delivered, topped with the payload length information in the header.

Sample types:

```cpp
enum class ESPNowMsgType : uint8_t {
   HEADING_DELTA   = 1,      // CMPS14-ESP32-SignalK-gateway
   BATTERY_DELTA   = 2,      // VEDirect based sender
   WEATHER_DELTA   = 3,      // BME280-ESP32-SignalK-gateway
   LEVEL_COMMAND   = 10,     // CMPS14-ESP32-SignalK-gateway
   LEVEL_RESPONSE  = 11,     // CMPS14-ESP32-SignalK-gateway
};
```
Sample payloads:

```cpp
struct HeadingDelta {
   float heading_rad;       // Magnetic heading (radians)
   float heading_true_rad;  // True heading (radians)
   float pitch_rad;         // Pitch (radians)
   float roll_rad;          // Roll (radians)
};

struct WeatherDelta {
   float temperature_c;   // °C
   float humidity_p;      // percent
   float pressure_hpa;    // hPa
};

struct BatteryDelta {
   float house_voltage;   // house bank volts
   float house_current;   // house bank amps
   float house_power;     // house bank watts
   float house_soc;       // house bank soc percent
   float start_voltage;   // starter battery volts
};
```

**Receives** at ~20 Hz, in radians (sent by CMPS14-ESP32-SignalK-gateway), as broadcast:
- `ESPNowPacket<HeadingDelta>`:
  - 24 B packet, 8 B header + 16 B payload
  - Payload: `HeadingDelta` struct (`heading_rad`, `heading_true_rad`, `pitch_rad`, `roll_rad` - equal to what SignalK server gets from the gateway)
  - `HeadingDelta` converted into `HeadingData`, an internal data struct for CrowPanel implementation

**Receives** at ~0.5 Hz, in °C, % and hPA (sent by BME280-ESP32-SignalK-gateway), as broadcast:
- `ESPNowPacket<WeatherDelta>`:
  - 20 B packet, 8 B header + 12 B payload
  - Payload: `WeatherDelta` struct (`temperature_c`, `humidity_p`, `pressure_hpa`)

**Receives** at ~1 Hz, in V, A and % (sent by VEDirect based sender), as broadcast:
- `ESPNowPacket<BatteryDelta>`:
  - 28 B packet, 8 B header + 20 B payload
  - Payload: `BatteryDelta` struct (`house_voltage`, `house_current`, `house_power`, `house_soc`, `start_voltage`)

**Sends** attitude leveling command as broadcast:
- `ESPNowPacket<LevelCommand>`:
  - 16 B, 8 B header + 8 B payload

**Receives** leveling response as unicast:
- `ESPNowPacket<LevelResponse>`:
  - 16 B, 8 B header + 8 B payload

**Channel:** ESP-NOW evices must be on the same WiFi channel. Configured to channel 6 (`static constexpr uint8_t ESP_NOW_CHANNEL = 6` in `CrowPanelApplication.h`). Set your router to a fixed channel 6. This allows senders to operate both on WiFi and ESP-NOW, using WiFi's channel for ESP-NOW. Avoid channel jumping by setting a fixed channel in the router.

**Deadband:** Compass sender has 0.25° deadband — no packet sent if heading and attitude change less than 0.25°. CrowPanel has an additional 0.5° threshold for compass rose rotation rendering only.

**NOTE:** Requires CMPS14-ESP32-SignalK-gateway v1.3.0 and BME280-ESP32-SignalK-gateway v1.0.0 or newer.

## Project structure

| File(s) | Description |
|---------|-------------|
| `ESP32-Crowpanel-compass.ino` | Owns `CrowPanelApplication app`, contains `setup()` and `loop()` |
| `CrowPanelApplication.h/.cpp` | Class CrowPanelApplication, the "app" — owns all instances |
| `espnow_protocol.h` | Wire protocol (namespace `ESPNow`): `ESPNowHeader`, `ESPNowPacket<T>`, `ESPNowMsgType`, `HeadingData/Delta`, `LevelCommand/Response` |
| `IScreenUI.h` | Abstract base class for all UI adapter class implementations |
| `ESPNowReceiver.h/.cpp` | Class `ESPNowReceiver` — ESP-NOW receive and level command sender |
| `CompassUI.h/.cpp` | Class `CompassUI` — compass screen adapter, realizes `IScreenUI` |
| `AttitudeUI.h/.cpp` | Class `AttitudeUI` — attitude screen adapter + leveling state machine, realizes `IScreenUI` |
| `WeatherUI.h/cpp` | Class `WeatherUI` - weather screen adapter, realizes `IScreenUI` |
| `BatteryUI.h/cpp` | Class `BatteryUI` - battery screen adapter, realizes `IScreenUI` |
| `BrightnessUI.h/.cpp` | Class `BrightnessUI` — brightness screen adapter + adjustment state machine, realizes `IScreenUI` |
| `RotaryEncoder.h/.cpp` | Class `RotaryEncoder` — rotary knob rotation and button, FreeRTOS tasks |
| `ScreenManager.h/.cpp` | Class `ScreenManager` — Scalable screen carousel management |
| `lv_conf.h` | LVGL configuration file based on the template provided by LVGL library |
| `Crowpanel_ST7701_Init.h/.cpp` | `Arduino_RGB_Display` init table for CrowPanel display - `crowpanel_st7701_type5_init_operations` |
| `ui.h/.c` | SquareLine Studio generated — UI init |
| `ui_CompassScreen.h/.c` | SquareLine Studio generated |
| `ui_AttitudeScreen.h/.c` | SquareLine Studio generated |
| `ui_WeatherScreen.h/.c` | SquareLine Studio generated | 
| `ui_BatteryScreen.h/.c` | SquareLine Studio generated |
| `ui_BrightnessScreen.h/.c` | SquareLine Studio generated |
| `ui_helpers.h/.c` | SquareLine Studio generated |
| `ui_font_*.c` | Custom fonts |
| `ui_img_*.c` | Images (compass rose, icons) |
| `UI/` | SquareLine Studio project |
| `docs/`| Other documents |

## Hardware

### Bill of materials

1. [Elecrow CrowPanel 2.1" HMI ESP32 Rotary Display](https://www.elecrow.com/wiki/CrowPanel_2.1inch-HMI_ESP32_Rotary_Display_480_IPS_Round_Touch_Knob_Screen.html), having:
   - ESP32-S3 module
   - 480x480 IPS round display (ST7701, RGB interface)
   - Rotary encoder with push button (PCF8574 I2C GPIO expander at 0x21)
2. WiFi router with fixed channel 6
3. [CMPS14-ESP32-SignalK-gateway](https://github.com/mkvesala/CMPS14-ESP32-SignalK-gateway) as ESP-NOW sender
4. [BME280-ESP32-SignalK-gateway](https://github.com/mkvesala/BME280-ESP32-SignalK-gateway) as ESP-NOW sender
5. VEDirect-ESP32-SignalK-gateway as ESP-NOW sender
6. [3D-printed mounting frame for CrowPanel](docs/CrowPanel_2_1_HMI_mounting.stl):

   <img src="docs/mountingframe.png" width="480">

**No paid partnerships.**

## Software

1. Arduino IDE 2.3.8
2. Espressif Systems esp32 board package 3.3.7
3. Additional libraries:
   - LVGL 9.5.0
   - Arduino GFX Library (by Moon On Our Nation) 1.6.5
   - PCF8574 (by Renzo Mischianti) 2.4.0
4. SquareLine Studio 1.6.0 for UI design and code generation
5. CMPS14-ESP32-SignalK-gateway v1.3.0
6. BME280-ESP32-SignalK-gateway v1.0.0
7. VEDirect-ESP32-SignalK-gateway v1.0.0

## Installation

1. Clone the repo
   ```
   git clone https://github.com/mkvesala/ESP32-Crowpanel-compass.git
   ```
2. Alternatively, download the code as zip
3. Install required libraries in Arduino IDE (LVGL, Arduino_GFX_Library, PCF8574)
4. Set ESP-NOW channel in `CrowPanelApplication.h` to match your router:
   ```cpp
   static constexpr uint8_t ESP_NOW_CHANNEL = 6;
   ```
5. Connect and power up the CrowPanel with USB
6. Compile and upload with Arduino IDE (board: ESP32S3 Dev Module)
7. Point the ESP-NOW senders to the same WiFi channel

**Note** that `lv_conf.h` is in project root (with default values from the library template). If you are using LVGL elsewhere, this file is probably under `Arduino/libraries/` folder next to `lvgl` library folder. Check the settings and use either one to avoid conflicts.

**SquareLine Studio note:** SquareLine Studio clears the export directory completely on export. Always git commit before exporting from SquareLine Studio and set a temporary directory in project settings for the export folder.

## Todo

Check [open issues](https://github.com/mkvesala/ESP32-Crowpanel-compass/issues).

## Debug

Performance characteristics on CrowPanel 2.1" (ESP32-S3):

| Screen | UI updates/5s | LVGL avg | LVGL max | Notes |
|--------|--------------|----------|----------|-------|
| Compass (heading changing) | ~52 | ~48 ms | ~160 ms | 240x240 zoom=512, no alpha, antialias off, draw buffer 120 lines, adaptive LVGL tick scheduling |
| Compass (stable heading) | 48-74 | 1-7 ms | — | 0.5° threshold prevents unnecessary re-renders |
| Attitude (data flowing) | ~80 | 4-13 ms | — | Horizon line 680x4 px is cheap to render |
| Attitude (stable) | ~83 | <1 ms | — | Nothing to render |

Compass rose `lv_image_set_rotation()` is the main performance bottleneck (only on the compass screen). PNG image stored in the image object is 240x240 pixels, no alpha, scaled with LVGL factor 512 to 480x480 pixels. Antialiasing is off. LVGL rendering is based on partial mode, using buffer of 480x120.

Flash usage: ~54%.

## Security

**Use at your own risk — not for safety-critical navigation!**

This device receives data only via ESP-NOW broadcast on a local WiFi channel. There is no network server, no authentication and no sensitive data. Keep the device on a private boat WiFi network.

## Credits

Software and libraries used are documented in the above sections.

Inspired by [example source code by Elecrow](https://github.com/Elecrow-RD/CrowPanel-2.1inch-HMI-ESP32-Rotary-Display-480-480-IPS-Round-Touch-Knob-Screen).

[Humidity icons created by Freepik - Flaticon](https://www.flaticon.com/free-icons/humidity)

[Temperature icons created by Freepik - Flaticon](https://www.flaticon.com/free-icons/temperature)

[Sun icons created by Freepik - Flaticon](https://www.flaticon.com/free-icons/sun)

[Battery icons created by Freepik - Flaticon](https://www.flaticon.com/free-icons/battery)

[Pressure icons created by Muhammad Ali - Flaticon](https://www.flaticon.com/free-icons/pressure)

[Bubble level icons created by vectorsmarket15 - Flaticon](https://www.flaticon.com/free-icons/bubble-level)

This is a companion project to my [CMPS14-ESP32-SignalK-gateway](https://github.com/mkvesala/CMPS14-ESP32-SignalK-gateway) and [BME280-ESP32-SignalK-gateway](https://github.com/mkvesala/BME280-ESP32-SignalK-gateway). Check the UML diagram below to see how these projects relate:

<img src="docs/full_uml_diagram.jpeg" width="480">

No paid partnerships.

Developed by Matti Vesala in collaboration with Claude. Claude was used for code review, bug finding and C++ design advice throughout the project.

See [CONTRIBUTING.md](CONTRIBUTING.md) for further details on AI-assisted development.

## Gallery

<img src="docs/compassscreen.png" width="240"> <img src="docs/attitudescreen1.png" width="240"> <img src="docs/attitudescreen2.png" width="240"> <img src="docs/attitudescreen3.png" width="240"> <img src="docs/weatherscreen1.png" width="240"> <img src="docs/weatherscreen2.png" width="240"> <img src="docs/weatherscreen3.png" width="240"> <img src="docs/batteryscreenhousev.png" width="240"> <img src="docs/batteryscreenhousea.png" width="240"> <img src="docs/batteryscreenhousesoc.png" width="240"> <img src="docs/batteryscreenstart.png" width="240"> <img src="docs/brightnessscreen.png" width="240"> <img src="docs/compassui.jpeg" width="240"> <img src="docs/attitudeui.jpeg" width="240"> <img src="docs/attitudeui2.jpeg" height="240"> <img src="docs/attitudeui3.jpeg" height="240"> <img src="docs/weatherui1.jpeg" width="240"> <img src="docs/weatherui2.jpeg" width="240"> <img src="docs/weatherui3.jpeg" width="240"> <img src="docs/batteryuihousev.jpeg" width="240"> <img src="docs/batteryuihousea.jpeg" width="240"> <img src="docs/batteryuihousesoc.jpeg" width="240"> <img src="docs/batteryuistart.jpeg" width="240"> <img src="docs/brightnessui.jpeg" width="240"> <img src="docs/full_uml_diagram.jpeg" width="240"> <img src="docs/mountingframe.png" width="240">
