![Logo](docs/projectlogo.svg)

# ESP32 CrowPanel Compass Multi-Function Display

[![Platform: ESP32-S3](https://img.shields.io/badge/Platform-ESP32--S3-blue)](https://www.espressif.com/en/sdks/esp-arduino)
[![Display: CrowPanel 2.1"](https://img.shields.io/badge/Display-CrowPanel%202.1%22-lightgrey)](https://www.elecrow.com/wiki/CrowPanel_2.1inch-HMI_ESP32_Rotary_Display_480_IPS_Round_Touch_Knob_Screen.html)
[![Protocol: ESP-NOW](https://img.shields.io/badge/Protocol-ESP--NOW-orange)](https://www.espressif.com/en/solutions/low-power-solutions/esp-now)
[![License: MIT](https://img.shields.io/badge/License-MIT-green.svg)](LICENSE)

Marine instrument display for [Elecrow CrowPanel 2.1" HMI](https://www.elecrow.com/wiki/CrowPanel_2.1inch-HMI_ESP32_Rotary_Display_480_IPS_Round_Touch_Knob_Screen.html) (ESP32-S3, 480×480 IPS round touchscreen, rotary knob). Receives compass heading, pitch and roll via ESP-NOW from [CMPS14-ESP32-SignalK-gateway](https://github.com/mkvesala/CMPS14-ESP32-SignalK-gateway) compass and displays the values on a round LVGL UI.

Three screens selectable with the rotary knob:
- **Compass screen** — rotating compass rose, heading value, True/Magnetic toggle
- **Attitude screen** — artificial horizon, pitch and roll values, attitude leveling
- **Brightness screen** — backlight brightness adjustment with NVS persistence

Developed and tested on:
- [Elecrow CrowPanel 2.1" HMI ESP32 Rotary Display](https://www.elecrow.com/wiki/CrowPanel_2.1inch-HMI_ESP32_Rotary_Display_480_IPS_Round_Touch_Knob_Screen.html)
- [ESP32 board package](https://github.com/espressif/arduino-esp32) (2.0.14)
- [Arduino IDE](https://www.arduino.cc/en/software/) (2.3.6)
- [LVGL](https://lvgl.io/) (8.3.11)
- [SquareLine Studio](https://squareline.io/) (1.6.0) for UI design

Integrated via ESP-NOW with:
- [CMPS14-ESP32-SignalK-gateway](https://github.com/mkvesala/CMPS14-ESP32-SignalK-gateway) — compass sender

## Purpose of the project

This is one of my individual digital boat projects. Use at your own risk. Not for safety-critical navigation.

1. I needed a compact display for heading, pitch and roll near the helm, receiving data wirelessly from the compass, independently from WiFi and SignalK
2. I wanted to learn LVGL and SquareLine Studio for touch-screen UI development
3. I continued learning ESP32 C++ patterns and FreeRTOS from the companion compass project

## Classes

<img src="docs/uml_diagram.png" width="480">

The classes on the UML class diagram are presented with their full public API. The private attributes only to demostrate the class relationships. The diagram includes the `ESPNowBroker` class of the CMPS14-ESP32-SignalK-gateway.

**`CrowPanelApplication`:**
- Owns: `Arduino_ESP32RGBPanel`, `Arduino_ST7701_RGBPanel`, `PCF8574`, `ESPNowReceiver`, `CompassUI`, `AttitudeUI`, `BrightnessUI`, `RotaryEncoder`, `ScreenManager`
- Responsible for: orchestrating everything within the main program

**`ESPNowReceiver`:**
- Responsible for: receiving `HeadingData` broadcasts and sending attitude leveling commands via ESP-NOW
- Owned by: `CrowPanelApplication`

**`CompassUI`:**
- Uses: `HeadingData`
- Responsible for: updating LVGL UI objects on the compass screen based on heading data
- Owned by: `CrowPanelApplication`

**`AttitudeUI`:**
- Uses: `HeadingData`, `ESPNowReceiver`
- Responsible for: updating LVGL UI objects on the attitude screen based on pitch and roll data
- Owned by: `CrowPanelApplication`

**`BrightnessUI`:**
- Uses: `Preferences`
- Responsible for: backlight brightness adjustment with NVS persistence, updating LVGL UI objects on the brightness screen
- Owned by: `CrowPanelApplication`

**`RotaryEncoder`:**
- Uses: `PCF8574`
- Responsible for: reading rotary knob rotation and knob button press
- Owned by: `CrowPanelApplication`

**`ScreenManager`:**
- Uses: `CompassUI`, `AttitudeUI`, `BrightnessUI`
- Responsible for: 3-screen carousel navigation with animated screen transitions
- Owned by: `CrowPanelApplication`

## Features

### Compass screen

<img src="docs/compassscreen.png" width="240"> <img src="docs/compassui.jpeg" width="240">

- Rotating compass rose image (240 x 240 px, rendered at 480 x 480 with LVGL scale)
- Heading value label
- True/Magnetic heading mode toggle with knob button press
- T/M mode indicator label
- *Connected* indicator panel (black = connected, red = disconnected)
- Rotation threshold 0.5°: skips LVGL re-render when heading change is below threshold — reduces ~194 ms re-render cycles on stable heading

### Attitude screen

<img src="docs/attitudescreen.png" width="240"> <img src="docs/attitudeui.jpeg" width="240">

- Artificial horizon: white 680 x 4 px image that rotates and translates based on pitch and roll
- Pitch and roll value labels
- Ship silhouette overlay on the artificial horizon
- Attitude leveling via knob button — two-press confirmation dialog with state machine:
  1. Knob press → confirm dialog ("Level attitude? Press knob again to confirm.", yellow)
  2. Second press → sends `LevelCommand` broadcast via ESP-NOW ("Leveling...", white)
  3. Response received → "Success!" (green) or "Failed!" (red)
  4. Timeout or screen switch → return to idle

### Brightness screen

<img src="docs/brightnessscreen.png" width="240"> <img src="docs/brightnessui.jpeg" width="240">

- Sun icon image and current brightness percentage label
- Knob button press enters ADJUSTING mode: arc overlay appears
- Knob rotation in ADJUSTING mode: ±2% brightness, updates arc, label and backlight in real-time
- 3-second timeout after last rotation → saves to NVS and returns to idle
- Brightness range: 2%–100% (2% minimum prevents screen going completely dark)
- Default: 78% (~200/255)
- Persistence: ESP32 Preferences (NVS), namespace `"display"`, key `"brightness"`
- PWM: GPIO6, LEDC channel 0, 5 kHz, 8-bit

### Knob behavior

| Screen | Button press | Rotation (normal) | Rotation (special) |
|--------|--------------|-------------------|--------------------|
| Compass | Toggle T/M heading mode | Switch screen | — |
| Attitude | Trigger level confirmation dialog | Switch screen | — |
| Brightness | Enter ADJUSTING mode | Switch screen | ±2% brightness (ADJUSTING mode only) |

Screen carousel order:
- **Clockwise:** COMPASS → ATTITUDE → BRIGHTNESS → COMPASS → ...
- **Counter-clockwise:** COMPASS → BRIGHTNESS → ATTITUDE → COMPASS → ...

### ESP-NOW communication

**Receives** at ~20 Hz, in radians (sent by CMPS14-ESP32-SignalK-gateway):
- `HeadingDelta` struct: `heading_rad`, `heading_true_rad`, `pitch_rad`, `roll_rad` (equal to what SignalK server gets from the gateway)
- `HeadingDelta` converted into `HeadingData`, an nternal data struct for CrowPanel implementation

**Sends** attitude leveling command as broadcast:
- `LevelCommand` struct: 4-byte magic `"LVLC"` + 4 reserved bytes

**Receives** leveling response as unicast:
- `LevelResponse` struct: 4-byte magic `"LVLR"` + 1-byte `success` (1 = ok, 0 = failed) + 3 reserved bytes

**Channel:** Both devices must be on the same WiFi channel. Configured to channel 6 (`static constexpr uint8_t ESP_NOW_CHANNEL = 6` in `CrowPanelApplication.h`). Set your router to a fixed channel 6. This is because sending compass has to operate both on WiFi and ESP-NOW, using WiFi's channel for ESP-NOW. Avoid channel jumping by setting a fixed channel in the router.

**Deadband:** Compass sender has 0.25° deadband — no packet sent if heading and attitude change less than 0.25°. CrowPanel has an additional 0.5° threshold for compass rose rotation rendering only.

### Diagnostics/debug

Three lines printed to Serial every 5 seconds:
```
[DIAG] PPS: 18.4 | UI updates: 25 | UI avg: 0.57 ms | UI max: 0.72 ms
[DIAG] LVGL calls: 25 | avg: 201.57 ms | max: 206.75 ms
[DIAG] Heap free: 8133839 | min: 8128811 | Stack loop: 4844 | enc: 1312 | btn: 728
```

Key finding: compass rose `lv_img_set_angle()` causes ~194 ms LVGL software re-render per frame (480 x 480 px). This is the main performance bottleneck on the compass screen. No practical alternative exists on this hardware (no GPU, no hardware rotation support in the display controller).

## Project structure

| File(s) | Description |
|---------|-------------|
| `ESP32-Crowpanel-compass.ino` | Owns `CrowPanelApplication app`, contains `setup()` and `loop()` |
| `CrowPanelApplication.h/.cpp` | Class CrowPanelApplication, the "app" — owns all instances |
| `espnow_protocol.h` | Data structs: `HeadingData`, `LevelCommand`, `LevelResponse` |
| `ESPNowReceiver.h/.cpp` | Class ESPNowReceiver — ESP-NOW receive and level command sender |
| `CompassUI.h/.cpp` | Class CompassUI — compass screen adapter |
| `AttitudeUI.h/.cpp` | Class AttitudeUI — attitude screen adapter + leveling state machine |
| `BrightnessUI.h/.cpp` | Class BrightnessUI — brightness screen adapter + adjustment state machine |
| `RotaryEncoder.h/.cpp` | Class RotaryEncoder — rotary knob rotation and button, FreeRTOS tasks |
| `ScreenManager.h/.cpp` | Class ScreenManager — 3-screen carousel + cleanup on screen leave |
| `ui.h/.c` | SquareLine Studio generated — UI init |
| `ui_CompassScreen.h/.c` | SquareLine Studio generated |
| `ui_AttitudeScreen.h/.c` | SquareLine Studio generated |
| `ui_BrightnessScreen.h/.c` | SquareLine Studio generated |
| `ui_helpers.h/.c` | SquareLine Studio generated |
| `ui_font_*.c` | Custom fonts |
| `ui_img_*.c` | Images (compass rose, horizon line, sun icon) |
| `UI/` | SquareLine Studio project |

## Hardware

### Bill of materials

1. [Elecrow CrowPanel 2.1" HMI ESP32 Rotary Display](https://www.elecrow.com/wiki/CrowPanel_2.1inch-HMI_ESP32_Rotary_Display_480_IPS_Round_Touch_Knob_Screen.html), having:
   - ESP32-S3 module
   - 480x480 IPS round display (ST7701, RGB interface)
   - Rotary encoder with push button (PCF8574 I2C GPIO expander at 0x21)
2. WiFi router with fixed channel 6
3. [CMPS14-ESP32-SignalK-gateway](https://github.com/mkvesala/CMPS14-ESP32-SignalK-gateway) as ESP-NOW sender
4. [3D-printed mounting frame for CrowPanel](docs/CrowPanel_2_1_HMI_mounting.stl):

   <img src="docs/mountingframe.png" width="480">

**No paid partnerships.**

## Software

1. Arduino IDE 2.3.6
2. Espressif Systems esp32 board package 2.0.14
3. Additional libraries:
   - [LVGL](https://lvgl.io/) 8.3.11
   - [Arduino_GFX_Library](https://github.com/moononournation/Arduino_GFX) (by Moon On Our Nation)
   - [PCF8574](https://github.com/RobTillaart/PCF8574) (by Rob Tillaart)
4. [SquareLine Studio](https://squareline.io/) 1.6.0 for UI design and code generation

**Note:** the esp32 board package and LVGL are far beyond the latest versions. The example source code provided by Elecrow did not compile with the newer versions so the development was done on the older libraries. TODO: study if the project can be migrated to the latest versions.

## Installation

1. Clone the repo
   ```
   git clone https://github.com/mkvesala/ESP32-Crowpanel-compass.git
   ```
2. Alternatively, download the code as zip
3. Install required libraries in Arduino IDE (LVGL, Arduino_GFX_Library, PCF8574) - **Note the versions!**
4. Set ESP-NOW channel in `CrowPanelApplication.h` to match your router:
   ```cpp
   static constexpr uint8_t ESP_NOW_CHANNEL = 6;
   ```
5. Connect and power up the CrowPanel with USB
6. Compile and upload with Arduino IDE (board: ESP32S3 Dev Module)
7. Point the CMPS14-ESP32-SignalK-gateway to the same WiFi channel

**SquareLine Studio note:** SquareLine Studio clears the export directory completely on export. Always git commit before exporting from SquareLine Studio and set a temporary directory in project settings for the export folder.

## Todo

- Consider replacing SquareLine Studio with hand-written LVGL to reduce generated code complexity
- Evaluate hardware rotation alternatives to reduce the ~194 ms compass rose re-render cost or replace PNG image object with other UI object types
- Add screens for other ESP-NOW senders' data, like weather, engine, batteries...

## Debug

Performance characteristics on CrowPanel 2.1" (ESP32-S3):

| Screen | UI updates/5s | LVGL avg | Notes |
|--------|--------------|----------|-------|
| Compass (heading changing) | ~25 | ~200 ms | Bottleneck: compass rose software rotation |
| Compass (stable heading) | 48-74 | 1-7 ms | 0.5° threshold prevents unnecessary re-renders |
| Attitude (data flowing) | ~80 | 4-13 ms | Horizon line 680x4 px is cheap to render |
| Attitude (stable) | ~83 | <1 ms | Nothing to render |

Flash usage: ~36% (1,137,857 bytes of 3,145,728).

## Security

**Use at your own risk — not for safety-critical navigation!**

This device receives data only via ESP-NOW broadcast on a local WiFi channel. There is no network server, no authentication and no sensitive data. Keep the device on a private boat WiFi network.

## Credits

Inspired by [example source code by Elecrow](https://github.com/Elecrow-RD/CrowPanel-2.1inch-HMI-ESP32-Rotary-Display-480-480-IPS-Round-Touch-Knob-Screen).

Developed and tested using:
- Elecrow CrowPanel 2.1" HMI
- Espressif Systems esp32 2.0.14 package on Arduino IDE 2.3.6
- LVGL 8.3.11 and SquareLine Studio 1.6.0

Companion project: [CMPS14-ESP32-SignalK-gateway](https://github.com/mkvesala/CMPS14-ESP32-SignalK-gateway). The full overview how these two projects relate:

<img src="docs/full_uml_diagram.jpeg" width="480">

No paid partnerships.

Developed by Matti Vesala in collaboration with Claude. Claude was used for code review, bug finding and C++ design advice throughout the project.

See [CONTRIBUTING.md](CONTRIBUTING.md) for further details on AI-assisted development.

## Gallery

<img src="docs/compassscreen.png" width="240"> <img src="docs/attitudescreen.png" width="240"> <img src="docs/brightnessscreen.png" width="240"> <img src="docs/compassui.jpeg" width="240"> <img src="docs/attitudeui.jpeg" width="240"> <img src="docs/brightnessui.jpeg" width="240"> <img src="docs/uml_diagram.png" width="240"> <img src="docs/full_uml_diagram.jpeg" width="240"> <img src="docs/mountingframe.png" width="240">
