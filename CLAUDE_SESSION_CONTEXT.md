# Claude Session Context - ESP32 CrowPanel Compass

**Date:** 2026-02-06 (updated)
**Project:** ESP32-Crowpanel-compass
**Hardware:** Elecrow CrowPanel 2.1" HMI (ESP32-S3, 480x480 IPS, Rotary Knob)

**ESP-NOW sender:** CMPS14-ESP32-SignalK-gateway compass, source code and documentation available in public repository [mkvesala/CMPS14-ESP32-SignalK-gateway](https://github.com/mkvesala/CMPS14-ESP32-SignalK-gateway/tree/feature/crow-panel-integration) branch `feature/crow-panel-integration`.

---

## Project Overview

MVP implementation of a marine instrument that receives ESP-NOW broadcast messages from CMPS14 compass and displays heading/pitch/roll values with LVGL UI on a round touchscreen.

### Screens

1. **CompassScreen** - Compass rose and heading value
2. **AttitudeScreen** - Artificial horizon (white line) and pitch/roll values

---

## Current Session (2026-02-06)

### Implemented Features

| Feature | Status | Files |
|---------|--------|-------|
| Knob button toggles T/M heading mode | ✅ Done | RotaryEncoder, CompassUI, .ino |
| HeadingData simplified (no valid flags) | ✅ Done | HeadingData.h |
| Diagnostics added (PPS, UI timing) | ✅ Done | .ino |
| PanelConnected: black=connected, red=disconnected | ✅ Done | CompassUI.cpp |
| WiFi removed from CrowPanel (ESP-NOW only) | ✅ Done | ESPNowReceiver, .ino |
| PPS issue fixed (channel 6) | ✅ Done | Router + .ino |
| UI update interval 200ms → 101ms | ✅ Done | .ino |
| **Attitude Level (CrowPanel side)** | ✅ Done | See below |

### Attitude Level Feature - CrowPanel COMPLETE

**Full implementation plan:** See `IMPLEMENTATION_PLAN.md`

**CrowPanel changes completed:**

| File | Changes |
|------|---------|
| `HeadingData.h` | Added LevelCommand and LevelResponse structs |
| `ESPNowReceiver.h/.cpp` | Added sendLevelCommand(), hasLevelResponse(), getLevelResult(); removed WiFi code |
| `AttitudeUI.h/.cpp` | Added LevelState enum, state machine, handleButtonPress(), updateLevelState(), cancelLevelOperation() |
| `ScreenManager.h/.cpp` | Added AttitudeUI pointer, calls cancelLevelOperation() on screen switch |
| `ESP32-Crowpanel-compass.ino` | Pass &attitudeUI to screenMgr.begin(), handle button press on AttitudeScreen, call updateLevelState() |

**State machine flow:**
1. Knob press → CONFIRM_WAIT (show "Level attitude? Press knob again to confirm." in yellow)
2. Second press → SENDING (show "Leveling..." in white, send ESP-NOW broadcast)
3. Response received → SUCCESS ("Success!" in green) or FAILED ("Failed!" in red)
4. Timeout or screen switch → return to IDLE (dialog hidden)

**Timeouts:** CONFIRM_WAIT 3s, SENDING 3s, SUCCESS 1.5s, FAILED 2s

### Compass Side - PENDING

Compass (CMPS14-ESP32-SignalK-gateway) `ESPNowBroker` needs:
- Register `esp_now_register_recv_cb()` to receive LevelCommand
- Store sender MAC, call `_compass->level()`
- Send LevelResponse unicast back to CrowPanel

See `IMPLEMENTATION_PLAN.md` section 6 for full code examples.

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

---

## Knob Button Behavior

| Screen | Action |
|--------|--------|
| CompassScreen | Toggle True/Magnetic heading mode |
| AttitudeScreen | Trigger level confirmation dialog |

---

## Important Notes

### ESP-NOW Channel Configuration
- Compass uses `peer.channel = 0` = uses WiFi AP's channel
- CrowPanel configured to channel 6: `#define ESP_NOW_CHANNEL 6`
- Router set to fixed channel 6

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

### SquareLine Studio Export
**WARNING:** SquareLine Studio clears export directory completely! Always git commit or backup before export.

### LVGL Scale Value
- 256 = 100% (original size)
- Formula: (target_size / original_size) × 256
- Example: 480/240 × 256 = 512

### PCF8574 GPIO Expander
- **CRITICAL:** pinMode() must be called BEFORE pcf8574.begin()
- P5 = Rotary encoder button (INPUT_PULLUP)

---

## Fonts

| Font | Usage |
|------|-------|
| `ui_font_FontHeading96b` | CompassScreen: heading |
| `ui_font_FontHeading64b` | CompassScreen: T/M mode |
| `ui_font_FontAttitude84c` | AttitudeScreen: pitch/roll (includes +) |
| `ui_font_FontAttitudeTitle24` | AttitudeScreen: "Pitch", "Roll" |

---

## File Structure

```
ESP32-Crowpanel-compass/
├── ESP32-Crowpanel-compass.ino  # Main program
├── HeadingData.h                # Data structures (incl. LevelCommand/Response)
├── ESPNowReceiver.h/.cpp        # ESP-NOW receiver + level command sender
├── CompassUI.h/.cpp             # Compass screen adapter
├── AttitudeUI.h/.cpp            # Attitude screen adapter + level state machine
├── RotaryEncoder.h/.cpp         # Rotary encoder handler
├── ScreenManager.h/.cpp         # Screen management + level cancel
├── IMPLEMENTATION_PLAN.md       # Attitude Level implementation plan
├── CLAUDE_SESSION_CONTEXT.md    # This file
├── .gitignore
├── ui.h/.c                      # SquareLine generated
├── ui_CompassScreen.h/.c        # SquareLine generated
├── ui_AttitudeScreen.h/.c       # SquareLine generated
├── ui_helpers.h/.c              # SquareLine generated
├── ui_font_*.c                  # Fonts
├── ui_img_*.c                   # Images
├── UI/                          # SquareLine project
└── RotaryScreen_2_1/            # Elecrow demo (reference)
```

---

## Build Info

- **Flash usage:** ~36% (1,137,857 bytes of 3,145,728)
- **Compass rose:** 240×240 px, Scale=512, ~850 KB C-code
- **Horizon line:** 680×4 px, 41 KB

---

## Next Steps

1. ✅ ~~Configure WiFi channel (done: channel 6)~~
2. ✅ ~~UI update interval 200ms → 101ms~~
3. ✅ ~~Implement AttitudeUI state machine for level dialog~~
4. ✅ ~~Add bidirectional ESP-NOW communication (CrowPanel side)~~
5. **Implement compass-side LevelCommand handling** (see IMPLEMENTATION_PLAN.md section 6)
6. **End-to-end testing**
