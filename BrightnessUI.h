#pragma once

#include <Arduino.h>
#include <lvgl.h>

/**
 * @brief Brightness adjustment state machine
 */
enum class BrightnessState {
    IDLE,       // Display only, rotation switches screens
    ADJUSTING   // Arc visible, rotation adjusts brightness
};

/**
 * @brief Brightness UI adapter SquareLine Studio -generoidulle UI:lle
 *
 * Hallinnoi BrightnessScreen-näytön UI-elementtejä ja taustavalon
 * kirkkauden säätöä rotary encoderilla. Kaksi tilaa: IDLE (pelkkä
 * näyttö) ja ADJUSTING (arc näkyvissä, knobin pyöritys säätää).
 *
 * Kirkkauden säätö:
 * - Knob press → ADJUSTING (näytä arc)
 * - Knob rotation → ±5% kirkkautta, arc+label+taustavalo päivittyvät
 * - 3 sek timeout → tallenna NVS:ään, piilota arc, palaa IDLE
 *
 * Käyttö:
 *   ui_init();
 *   brightnessUI.begin(pwmChannel);
 *
 *   // loop():ssa kun BrightnessScreen on aktiivinen
 *   brightnessUI.updateState();
 */
class BrightnessUI {
public:
    BrightnessUI();

    /**
     * @brief Alusta adapter
     *
     * Kutsutaan setup():ssa ui_init():n jälkeen.
     * Lataa tallennetun kirkkauden NVS:stä, asettaa taustavalon
     * ja päivittää UI-elementit.
     *
     * @param pwmChannel LEDC PWM -kanava taustavalon ohjaukseen
     */
    void begin(int pwmChannel);

    /**
     * @brief Käsittele knob-painallus
     *
     * IDLE → ADJUSTING (näytä arc)
     * ADJUSTING → ei toimintoa
     */
    void handleButtonPress();

    /**
     * @brief Käsittele knobin pyöritys ADJUSTING-tilassa
     *
     * @param direction -1 (CCW) tai +1 (CW)
     *
     * Jokainen askel muuttaa kirkkautta ±5%.
     * Päivittää arcin, labelin ja taustavalon reaaliajassa.
     * Nollaa auto-save -ajastimen.
     */
    void handleRotation(int8_t direction);

    /**
     * @brief Päivitä tilakone (kutsutaan joka loop-iteraatiossa)
     *
     * Tarkistaa auto-save timeoutin. Jos 3 sek kulunut
     * viimeisestä pyörityksestä: tallenna NVS, piilota arc, palaa IDLE.
     */
    void updateState();

    /**
     * @brief Peruuta säätötila (kutsutaan screen switchissä)
     *
     * Jos ADJUSTING-tilassa: tallenna kirkkaus ja palaa IDLE.
     */
    void cancelAdjustment();

    /**
     * @brief Tarkista onko säätötilassa
     *
     * Main loop käyttää tätä päättääkseen reititetäänkö
     * knobin pyöritys ScreenManagerille vai tälle luokalle.
     */
    bool isAdjusting() const { return _state == BrightnessState::ADJUSTING; }

    /**
     * @brief Hae nykyinen tila
     */
    BrightnessState getState() const { return _state; }

private:
    // Tila
    BrightnessState _state;
    bool _initialized;

    // Kirkkausarvo (0–100 prosenttia)
    int8_t _brightnessPercent;

    // Hardware
    int _pwmChannel;

    // Ajoitus
    uint32_t _lastRotationTime;

    // Auto-save timeout
    static constexpr uint32_t AUTOSAVE_TIMEOUT_MS = 3000;

    // Kirkkauden muutosaskel per encoder-klikkaus
    static constexpr int8_t BRIGHTNESS_STEP = 5;

    // Kirkkauden rajat (0% = näyttö pimeänä, ei toivottua)
    static constexpr int8_t MIN_BRIGHTNESS_PERCENT = 5;
    static constexpr int8_t MAX_BRIGHTNESS_PERCENT = 100;

    // NVS namespace ja avain
    static constexpr const char* NVS_NAMESPACE = "display";
    static constexpr const char* NVS_KEY_BRIGHTNESS = "brightness";
    static constexpr int8_t DEFAULT_BRIGHTNESS_PERCENT = 78;  // ~200/255

    // Sisäiset apufunktiot
    void setState(BrightnessState newState);
    void updateUI();
    void applyBrightness();
    void saveBrightness();
    int8_t loadBrightness();

    /**
     * @brief Muunna prosentti (0–100) PWM duty -arvoksi (0–255)
     *
     * Lineaarinen: duty = percent * 255 / 100
     */
    static uint8_t percentToPwm(int8_t percent);
};
