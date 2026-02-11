#pragma once

#include <Arduino.h>
#include <lvgl.h>
#include "HeadingData.h"

/**
 * @brief Kompassi-UI adapter SquareLine Studio -generoidulle UI:lle
 *
 * Päivittää SquareLine-generoituja UI-elementtejä HeadingData-datan perusteella.
 * Ei luo omia elementtejä - käyttää ui_* globaaleja suoraan.
 *
 * Käyttö:
 *   ui_init();     // SquareLine-generoitu
 *   ui.begin();    // CompassUI adapter
 *
 *   // loop():ssa
 *   ui.update(headingData, connected, packetsPerSecond);
 */
class CompassUI {
public:
    CompassUI();

    /**
     * @brief Alusta adapter
     *
     * Kutsutaan setup():ssa ui_init():n jälkeen.
     * Ei luo elementtejä, vain alustaa sisäisen tilan.
     */
    void begin();

    /**
     * @brief Päivitä UI uudella datalla
     *
     * @param data Kompassidata
     * @param connected Onko yhteys aktiivinen
     * @param packetsPerSec Paketteja sekunnissa (ei käytetä tällä hetkellä)
     */
    void update(const HeadingData& data, bool connected, float packetsPerSec);

    /**
     * @brief Näytä "waiting for data" tila
     */
    void showWaiting();

    /**
     * @brief Näytä "disconnected" tila
     */
    void showDisconnected();

    /**
     * @brief Vaihda heading-moodia (True ↔ Magnetic)
     *
     * Kutsutaan kun käyttäjä painaa rotary knob -painiketta.
     * Oletuksena näytetään True heading.
     */
    void toggleHeadingMode();

    /**
     * @brief Palauttaa true jos näytetään True heading
     */
    bool isShowingTrueHeading() const { return _use_true_heading; }

private:
    // Päivitysfunktiot SquareLine-elementeille
    void setCompassRotation(uint16_t heading_x10);
    void updateHeadingLabel(uint16_t heading_deg);
    void updateHeadingMode(bool is_true);
    void setConnectionIndicator(bool connected);

    // Compass rose rotation threshold (0.5° = 5 in x10 units)
    // Reduces heavy LVGL re-renders when heading changes are small
    static constexpr uint16_t ROTATION_THRESHOLD_X10 = 5;

    // Cached values (vältetään turhat LVGL-päivitykset)
    uint16_t _last_heading_x10;
    uint16_t _last_heading_deg;
    bool     _last_is_true;
    bool     _last_connected;

    // Heading mode toggle
    bool _use_true_heading;  // true = True heading, false = Magnetic heading

    bool _initialized;
};
