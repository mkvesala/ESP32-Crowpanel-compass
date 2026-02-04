# Claude Session Context - ESP32 CrowPanel Compass

**Date:** 2026-02-04 (updated)
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

## Current Session (2026-02-04)

### Implemented Features

| Feature | Status | Files |
|---------|--------|-------|
| Knob button toggles T/M heading mode | Done | RotaryEncoder, CompassUI, .ino |
| HeadingData simplified (no valid flags, no normalization) | Done | HeadingData.h |
| Diagnostics added (PPS, UI timing) | Done | .ino |
| PanelConnected: black=connected, red=disconnected | Done | CompassUI.cpp |
| Git history cleaned (WiFi credentials removed) | Done | New repo |

### Planned: Attitude Level Feature

**Full implementation plan:** See `IMPLEMENTATION_PLAN.md`

**Summary:**
- Knob press on AttitudeScreen triggers level confirmation dialog
- Second press sends ESP-NOW LevelCommand to compass
- Compass calls `CMPS14Processor::level()` and sends LevelResponse back
- CrowPanel shows success/failure status

**Key design decisions:**
- ESP-NOW bidirectional (hybrid: broadcast command, unicast response)
- WiFi removed from CrowPanel, ESP-NOW only
- Push-release validation (50-500ms) to prevent accidental triggers
- Screen switch cancels pending level operation (non-blocking)
- State machine in AttitudeUI for level dialog flow

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
└── ui_LabelLevelStatus (hidden, for level dialog) [TO BE ADDED]
```

---

## Knob Button Behavior

| Screen | Action |
|--------|--------|
| CompassScreen | Toggle True/Magnetic heading mode |
| AttitudeScreen | Trigger level confirmation (planned) |

**Safety:** Push-release validation (50-500ms duration required)

---

## Important Notes

### SquareLine Studio Export
**WARNING:** SquareLine Studio clears export directory completely! Always git commit or backup before export.

### LVGL Scale Value
- 256 = 100% (original size)
- Formula: (target_size / original_size) × 256
- Example: 480/240 × 256 = 512

### HeadingData
- Compass always sends valid data (no validation flags needed)
- Compass handles normalization (no wrap logic needed in CrowPanel)
- Simple conversion: radians → degrees × 10

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
├── HeadingData.h                # Data structures
├── ESPNowReceiver.h/.cpp        # ESP-NOW receiver
├── CompassUI.h/.cpp             # Compass screen adapter
├── AttitudeUI.h/.cpp            # Attitude screen adapter
├── RotaryEncoder.h/.cpp         # Rotary encoder handler
├── ScreenManager.h/.cpp         # Screen management
├── secrets.h                    # WiFi credentials (not in git) - TO BE REMOVED
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

1. Add `ui_LabelLevelStatus` in SquareLine Studio
2. Implement push-release validation in RotaryEncoder
3. Remove WiFi, implement ESP-NOW only mode
4. Implement AttitudeUI state machine for level dialog
5. Add bidirectional ESP-NOW communication
6. Implement compass-side LevelCommand handling
