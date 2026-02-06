#pragma once

#include <Arduino.h>
#include <lvgl.h>

// Forward declaration
class AttitudeUI;

/**
 * @brief Näyttöjen hallinta ja vaihto
 *
 * Hallitsee siirtymistä Compass- ja Attitude-näyttöjen välillä.
 * Käyttää LVGL:n screen load -animaatioita sulaviin siirtymiin.
 * Kutsuu AttitudeUI::cancelLevelOperation() kun vaihdetaan pois AttitudeScreenistä.
 */
class ScreenManager {
public:
    enum class Screen {
        COMPASS,
        ATTITUDE
    };

    ScreenManager();

    /**
     * @brief Alusta screen manager
     *
     * @param attitudeUI Pointer to AttitudeUI for cancel callback
     * Kutsutaan setup():ssa ui_init():n jälkeen.
     */
    void begin(AttitudeUI* attitudeUI = nullptr);

    /**
     * @brief Vaihda seuraavaan näyttöön (CW-rotaatio)
     */
    void switchNext();

    /**
     * @brief Vaihda edelliseen näyttöön (CCW-rotaatio)
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

private:
    Screen _current;
    bool _initialized;
    AttitudeUI* _attitudeUI;

    // Animaation kesto millisekunteina
    static constexpr uint32_t ANIM_DURATION_MS = 300;

    // Helper to cancel level operation when leaving AttitudeScreen
    void onLeavingAttitudeScreen();
};
