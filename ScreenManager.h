#pragma once

#include <Arduino.h>
#include <lvgl.h>

// Forward declarations
class AttitudeUI;
class BrightnessUI;

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
// - Provides public API for:
//   - ...
// - Uses: AttitudeUI, CompassUI, BrightnessUI
// - Owned by: CrowPanelApplication

class ScreenManager {

public:
    
    enum class Screen {
        COMPASS,
        ATTITUDE,
        BRIGHTNESS
    };

    ScreenManager();

    void begin(AttitudeUI* attitudeUI = nullptr, BrightnessUI* brightnessUI = nullptr);
    void switchNext();
    void switchPrevious();

    bool isCompassActive() const { return _current == Screen::COMPASS; }
    bool isAttitudeActive() const { return _current == Screen::ATTITUDE; }
    bool isBrightnessActive() const { return _current == Screen::BRIGHTNESS; }

private:

    Screen _current;
    bool _initialized;
    AttitudeUI* _attitudeUI;
    BrightnessUI* _brightnessUI;

    // Animation duration
    static constexpr uint32_t ANIM_DURATION_MS = 300;

    // Helper to clean up when leaving the current screen
    void onLeavingCurrentScreen();

    void switchTo(Screen screen);
};
