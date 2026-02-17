#pragma once

#include <Arduino.h>
#include <lvgl.h>
#include "CompassUI.h"
#include "AttitudeUI.h"
#include "BrightnessUI.h"
#include "ui.h"

/**
 * @brief Näyttöjen hallinta ja vaihto
 *
 * Hallitsee siirtymistä Compass-, Attitude- ja Brightness-näyttöjen välillä.
 * Käyttää LVGL:n screen load -animaatioita sulaviin siirtymiin.
 * Kutsuu AttitudeUI::cancelLevelOperation() kun vaihdetaan pois AttitudeScreenistä.
 * Kutsuu BrightnessUI::cancelAdjustment() kun vaihdetaan pois BrightnessScreenistä.
 */

// ===  C L A S S  S C R E E N M A N A G E R ===
//
// - Class ScreenManager - responsible for switching between screens
// - Init: 
// - Provides public API to switch smoothly between screens CW or CCW.
// - Uses: AttitudeUI, CompassUI, BrightnessUI
// - Owned by: CrowPanelApplication

class ScreenManager {

public:
    
    enum class Screen {
        COMPASS,
        ATTITUDE,
        BRIGHTNESS
    };

    ScreenManager(CompassUI &compassUI, AttitudeUI &attitudeUI, BrightnessUI &brightnessUI);

    void begin();
    void switchNext();
    void switchPrevious();

    bool isCompassActive() const { return _current == Screen::COMPASS; }
    bool isAttitudeActive() const { return _current == Screen::ATTITUDE; }
    bool isBrightnessActive() const { return _current == Screen::BRIGHTNESS; }

private:

    bool _initialized;
    Screen _current;

    CompassUI &_compassUI;
    AttitudeUI &_attitudeUI;
    BrightnessUI &_brightnessUI;

    // Animation duration
    static constexpr uint32_t ANIM_DURATION_MS = 300;

    // Animation direction (internal implementation detail)
    enum class Direction { CW, CCW };

    // Carousel order helpers
    Screen nextScreen() const;
    Screen previousScreen() const;

    // Unified screen switch with animation direction
    void switchTo(Screen screen, Direction dir);

    // Helper to clean up when leaving the current screen
    void onLeavingCurrentScreen();
};
