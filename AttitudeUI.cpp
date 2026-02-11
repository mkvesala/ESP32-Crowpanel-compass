#include "AttitudeUI.h"
#include "ESPNowReceiver.h"
#include "ui.h"

AttitudeUI::AttitudeUI()
    : _last_pitch_x10(0x7FFF)  // Invalid initial value
    , _last_roll_x10(0x7FFF)
    , _last_pitch_deg(0x7FFF)
    , _last_roll_deg(0x7FFF)
    , _initialized(false)
    , _levelState(LevelState::IDLE)
    , _stateStartTime(0)
{
}

void AttitudeUI::begin() {
    if (_initialized) return;

    // Aseta rotaation pivot-piste ImageHorizon:n keskelle
    // Kuva on 680x4, joten keskipiste on 340, 2
    lv_img_set_pivot(ui_ImageHorizon, 340, 2);

    // Piilota level-dialogi aluksi
    lv_obj_add_flag(ui_ContainerLevelingDialog, LV_OBJ_FLAG_HIDDEN);

    _levelState = LevelState::IDLE;
    _initialized = true;

    // Näytä aluksi "waiting" tila
    showWaiting();
}

void AttitudeUI::update(const HeadingData& data, bool connected, float packetsPerSec) {
    if (!_initialized) return;

    // Päivitä horisontin sijainti ja rotaatio
    updateHorizon(data.pitch_x10, data.roll_x10);

    // Päivitä labelit
    updatePitchLabel(data.getPitchDeg());
    updateRollLabel(data.getRollDeg());
}

void AttitudeUI::updateHorizon(int16_t pitch_x10, int16_t roll_x10) {
    // Optimointi: päivitä vain jos muuttunut
    if (pitch_x10 == _last_pitch_x10 && roll_x10 == _last_roll_x10) return;
    _last_pitch_x10 = pitch_x10;
    _last_roll_x10 = roll_x10;

    // PITCH: Siirrä horisonttiviivaa Y-suunnassa
    // Horisontin toiminta: keula alas (pitch < 0) → horisontti nousee ylös näytöllä
    // lv_obj_set_y: positiivinen arvo siirtää objektia ALAS suhteessa align-pisteeseen
    // ImageHorizon on ALIGN_CENTER, joten y=0 on keskellä
    // - pitch < 0 (keula alas) → horisontti nousee → tarvitaan negatiivinen y
    // - pitch = -10° (-100 x10) → y = -(-100 * 3) / 10 = +30 → viiva alas → VÄÄRIN
    // Korjaus: y = (pitch_x10 * PITCH_SCALE) / 10
    // - pitch = -10° → y = (-100 * 3) / 10 = -30 → viiva ylös → horisontti nousee ✓
    int16_t y_offset = (pitch_x10 * PITCH_SCALE) / 10;
    lv_obj_set_y(ui_ImageHorizon, y_offset);

    // ROLL: Kuvan rotaatio
    // Horisontin toiminta: kallistus vasemmalle (roll < 0) → horisontti kallistuu oikealle
    // lv_img_set_angle: positiivinen kulma = myötäpäivään, käyttää 0.1° yksikköjä
    // roll < 0 (kallistus vasemmalle) → horisontti kallistuu myötäpäivään (oikealle)
    // roll = -10° (-100 x10) → angle = -(-100) = +100 = +10° myötäpäivään ✓
    lv_img_set_angle(ui_ImageHorizon, -roll_x10);
}

void AttitudeUI::updatePitchLabel(int16_t pitch_deg) {
    if (pitch_deg == _last_pitch_deg) return;
    _last_pitch_deg = pitch_deg;

    char buf[16];
    // Formaatti: "+003°" tai "-012°" (etumerkki aina, 3 numeroa, aste-merkki)
    snprintf(buf, sizeof(buf), "%+04d°", pitch_deg);
    lv_label_set_text(ui_LabelPitch, buf);
}

void AttitudeUI::updateRollLabel(int16_t roll_deg) {
    if (roll_deg == _last_roll_deg) return;
    _last_roll_deg = roll_deg;

    char buf[16];
    // Formaatti: "+003°" tai "-012°"
    snprintf(buf, sizeof(buf), "%+04d°", roll_deg);
    lv_label_set_text(ui_LabelRoll, buf);
}

void AttitudeUI::showWaiting() {
    if (!_initialized) return;

    lv_label_set_text(ui_LabelPitch, "---");
    lv_label_set_text(ui_LabelRoll, "---");

    // Palauta horisontti keskelle
    lv_obj_set_y(ui_ImageHorizon, 0);
    lv_img_set_angle(ui_ImageHorizon, 0);

    // Reset cached values
    _last_pitch_x10 = 0x7FFF;
    _last_roll_x10 = 0x7FFF;
    _last_pitch_deg = 0x7FFF;
    _last_roll_deg = 0x7FFF;
}

void AttitudeUI::showDisconnected() {
    if (!_initialized) return;

    // Näytä viimeiset arvot mutta voisi myös näyttää "---"
    // Tässä toteutuksessa säilytetään viimeiset arvot
}

// === Level State Machine ===

bool AttitudeUI::handleButtonPress(ESPNowReceiver& receiver) {
    if (!_initialized) return false;

    switch (_levelState) {
        case LevelState::IDLE:
            // First press: show confirmation dialog
            setLevelState(LevelState::CONFIRM_WAIT);
            return true;

        case LevelState::CONFIRM_WAIT:
            // Second press: send level command
            if (receiver.sendLevelCommand()) {
                setLevelState(LevelState::SENDING);
            } else {
                setLevelState(LevelState::FAILED);
            }
            return true;

        case LevelState::SENDING:
        case LevelState::SUCCESS:
        case LevelState::FAILED:
            // Ignore presses during these states
            return true;
    }
    return false;
}

void AttitudeUI::updateLevelState(ESPNowReceiver& receiver) {
    if (!_initialized) return;
    if (_levelState == LevelState::IDLE) return;

    uint32_t elapsed = millis() - _stateStartTime;

    switch (_levelState) {
        case LevelState::CONFIRM_WAIT:
            if (elapsed >= CONFIRM_TIMEOUT_MS) {
                setLevelState(LevelState::IDLE);
            }
            break;

        case LevelState::SENDING:
            if (receiver.hasLevelResponse()) {
                bool success = receiver.getLevelResult();
                setLevelState(success ? LevelState::SUCCESS : LevelState::FAILED);
            } else if (elapsed >= SENDING_TIMEOUT_MS) {
                setLevelState(LevelState::FAILED);
            }
            break;

        case LevelState::SUCCESS:
            if (elapsed >= SUCCESS_DISPLAY_MS) {
                setLevelState(LevelState::IDLE);
            }
            break;

        case LevelState::FAILED:
            if (elapsed >= FAILED_DISPLAY_MS) {
                setLevelState(LevelState::IDLE);
            }
            break;

        case LevelState::IDLE:
            // Already handled above
            break;
    }
}

void AttitudeUI::cancelLevelOperation() {
    if (_levelState != LevelState::IDLE) {
        setLevelState(LevelState::IDLE);
    }
}

void AttitudeUI::setLevelState(LevelState newState) {
    _levelState = newState;
    _stateStartTime = millis();
    updateLevelDialog();
}

void AttitudeUI::updateLevelDialog() {
    switch (_levelState) {
        case LevelState::IDLE:
            lv_obj_add_flag(ui_ContainerLevelingDialog, LV_OBJ_FLAG_HIDDEN);
            break;

        case LevelState::CONFIRM_WAIT:
            lv_obj_clear_flag(ui_ContainerLevelingDialog, LV_OBJ_FLAG_HIDDEN);
            lv_label_set_text(ui_LabelLevelingDialog, "Level attitude?\n\nPress knob again\nto confirm.");
            lv_obj_set_style_text_color(ui_LabelLevelingDialog, lv_color_hex(0xFFFF00), 0);  // Yellow
            break;

        case LevelState::SENDING:
            lv_obj_clear_flag(ui_ContainerLevelingDialog, LV_OBJ_FLAG_HIDDEN);
            lv_label_set_text(ui_LabelLevelingDialog, "Leveling...");
            lv_obj_set_style_text_color(ui_LabelLevelingDialog, lv_color_hex(0xFFFFFF), 0);  // White
            break;

        case LevelState::SUCCESS:
            lv_obj_clear_flag(ui_ContainerLevelingDialog, LV_OBJ_FLAG_HIDDEN);
            lv_label_set_text(ui_LabelLevelingDialog, "Success!");
            lv_obj_set_style_text_color(ui_LabelLevelingDialog, lv_color_hex(0x00FF00), 0);  // Green
            break;

        case LevelState::FAILED:
            lv_obj_clear_flag(ui_ContainerLevelingDialog, LV_OBJ_FLAG_HIDDEN);
            lv_label_set_text(ui_LabelLevelingDialog, "Failed!");
            lv_obj_set_style_text_color(ui_LabelLevelingDialog, lv_color_hex(0xFF0000), 0);  // Red
            break;
    }
}
