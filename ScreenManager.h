#pragma once

#include <Arduino.h>
#include <lvgl.h>
#include "CompassUI.h"
#include "AttitudeUI.h"
#include "BrightnessUI.h"
#include "ui.h"

// ===  C L A S S  S C R E E N M A N A G E R ===
//
// - Class ScreenManager - responsible for switching between screens
// - Init: screenMgr.begin();
// - Provides public API to switch smoothly (lvgl animation) between screens CW or CCW.
// - Uses: AttitudeUI, CompassUI, BrightnessUI
// - Owned by: CrowPanelApplication

class ScreenManager {

public:

    ScreenManager(CompassUI &compassUI, AttitudeUI &attitudeUI, BrightnessUI &brightnessUI);

    void begin();
    void switchNext();
    void switchPrevious();

    bool isCompassActive() const { return _current == Screen::COMPASS; }
    bool isAttitudeActive() const { return _current == Screen::ATTITUDE; }
    bool isBrightnessActive() const { return _current == Screen::BRIGHTNESS; }

private:

    // Animation direction
    enum class Direction { CW, CCW };

    // Current screen
    enum class Screen {
        COMPASS,
        ATTITUDE,
        BRIGHTNESS
    };

    bool _initialized;
    Screen _current;

    CompassUI &_compassUI;
    AttitudeUI &_attitudeUI;
    BrightnessUI &_brightnessUI;

    // Animation duration
    static constexpr uint32_t ANIM_DURATION_MS = 300;

    // Carousel order helpers
    Screen nextScreen() const;
    Screen previousScreen() const;

    // Unified screen switch with animation direction
    void switchTo(Screen screen, Direction dir);

    // Helper to clean up when leaving the current screen
    void onLeavingCurrentScreen();
};
