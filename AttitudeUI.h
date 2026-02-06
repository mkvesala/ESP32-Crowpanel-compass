#pragma once

#include <Arduino.h>
#include <lvgl.h>
#include "HeadingData.h"

// Forward declaration
class ESPNowReceiver;

/**
 * @brief Level operation state machine
 */
enum class LevelState {
    IDLE,           // Normal operation, dialog hidden
    CONFIRM_WAIT,   // "Level? Press to confirm" visible, waiting for 2nd press
    SENDING,        // "Leveling..." visible, waiting for response
    SUCCESS,        // "Success!" visible briefly
    FAILED          // "Failed!" visible briefly
};

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
 * Level-toiminto:
 * - Knob press → CONFIRM_WAIT (näytä dialogi)
 * - Knob press uudelleen → SENDING (lähetä komento)
 * - Vastaus tai timeout → SUCCESS/FAILED → IDLE
 *
 * Käyttö:
 *   ui_init();        // SquareLine-generoitu
 *   attitudeUI.begin();
 *
 *   // loop():ssa kun AttitudeScreen on aktiivinen
 *   attitudeUI.update(headingData, connected, packetsPerSecond);
 *   attitudeUI.updateLevelState(receiver);
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

    /**
     * @brief Käsittele knob-painallus
     * @param receiver ESP-NOW receiver for sending level command
     * @return true jos painallus käsiteltiin (estää muut toiminnot)
     */
    bool handleButtonPress(ESPNowReceiver& receiver);

    /**
     * @brief Päivitä level-tilakoneen tila
     * @param receiver ESP-NOW receiver for checking response
     */
    void updateLevelState(ESPNowReceiver& receiver);

    /**
     * @brief Peruuta level-operaatio (kutsutaan screen switchissä)
     */
    void cancelLevelOperation();

    /**
     * @brief Hae nykyinen level-tila
     */
    LevelState getLevelState() const { return _levelState; }

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

    // Level state machine
    LevelState _levelState;
    uint32_t _stateStartTime;

    // Level dialog update
    void setLevelState(LevelState newState);
    void updateLevelDialog();

    // Timeouts (ms)
    static constexpr uint32_t CONFIRM_TIMEOUT_MS = 3000;
    static constexpr uint32_t SENDING_TIMEOUT_MS = 3000;
    static constexpr uint32_t SUCCESS_DISPLAY_MS = 1500;
    static constexpr uint32_t FAILED_DISPLAY_MS = 2000;
};
