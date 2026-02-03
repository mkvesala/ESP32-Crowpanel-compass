#pragma once

#include <Arduino.h>
#include <lvgl.h>
#include "HeadingData.h"

/**
 * @brief Attitude/Horizon UI adapter SquareLine Studio -generoidulle UI:lle
 *
 * Päivittää keinohorisontti-näytön UI-elementtejä HeadingData-datan perusteella.
 * Liikuttaa ContainerHorizonGroup:ia pitch- ja roll-arvojen mukaisesti.
 *
 * Horisontin toiminta:
 * - Pitch: Keula alas (pitch < 0) → horisontti nousee ylös
 * - Roll: Kallistus vasemmalle (roll < 0) → horisontti kallistuu oikealle
 *
 * Käyttö:
 *   ui_init();        // SquareLine-generoitu
 *   attitudeUI.begin();
 *
 *   // loop():ssa kun AttitudeScreen on aktiivinen
 *   attitudeUI.update(headingData, connected, packetsPerSecond);
 */
class AttitudeUI {
public:
    AttitudeUI();

    /**
     * @brief Alusta adapter
     *
     * Kutsutaan setup():ssa ui_init():n jälkeen.
     * Asettaa transform pivot -pisteen horisonttiryhmälle.
     */
    void begin();

    /**
     * @brief Päivitä UI uudella datalla
     *
     * @param data Kompassidata (sisältää pitch ja roll)
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

private:
    // Päivitysfunktiot
    void updatePitchLabel(int16_t pitch_deg);
    void updateRollLabel(int16_t roll_deg);
    void updateHorizon(int16_t pitch_x10, int16_t roll_x10);

    // Skaalauskertoimet
    static constexpr int16_t PITCH_SCALE = 3;  // Pikseliä per aste

    // Cached values (vältetään turhat LVGL-päivitykset)
    int16_t _last_pitch_x10;
    int16_t _last_roll_x10;
    int16_t _last_pitch_deg;
    int16_t _last_roll_deg;

    bool _initialized;
};
