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
class ScreenManager {
public:
    enum class Screen {
        COMPASS,
        ATTITUDE,
        BRIGHTNESS
    };

    ScreenManager();

    /**
     * @brief Alusta screen manager
     *
     * @param attitudeUI Pointer to AttitudeUI for cancel callback
     * @param brightnessUI Pointer to BrightnessUI for cancel callback
     * Kutsutaan setup():ssa ui_init():n jälkeen.
     */
    void begin(AttitudeUI* attitudeUI = nullptr, BrightnessUI* brightnessUI = nullptr);

    /**
     * @brief Vaihda seuraavaan näyttöön (CW-rotaatio)
     *
     * COMPASS → ATTITUDE → BRIGHTNESS → COMPASS → ...
     */
    void switchNext();

    /**
     * @brief Vaihda edelliseen näyttöön (CCW-rotaatio)
     *
     * COMPASS → BRIGHTNESS → ATTITUDE → COMPASS → ...
     */
    void switchPrevious();

    /**
     * @brief Vaihda tiettyyn näyttöön
     *
     * @param screen Kohdenäyttö
     */
    void switchTo(Screen screen);

    /**
     * @brief Hae nykyinen näyttö
     *
     * @return Screen Nykyinen aktiivinen näyttö
     */
    Screen current() const { return _current; }

    /**
     * @brief Tarkista onko tietty näyttö aktiivinen
     */
    bool isCompassActive() const { return _current == Screen::COMPASS; }
    bool isAttitudeActive() const { return _current == Screen::ATTITUDE; }
    bool isBrightnessActive() const { return _current == Screen::BRIGHTNESS; }

private:
    Screen _current;
    bool _initialized;
    AttitudeUI* _attitudeUI;
    BrightnessUI* _brightnessUI;

    // Animaation kesto millisekunteina
    static constexpr uint32_t ANIM_DURATION_MS = 300;

    // Helper to clean up when leaving the current screen
    void onLeavingCurrentScreen();
};
