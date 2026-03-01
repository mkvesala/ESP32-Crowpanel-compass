#include "AttitudeUI.h"
#include "ui.h"

// === P U B L I C ===

// Constructor
AttitudeUI::AttitudeUI(ESPNowReceiver &receiver)
    : _receiver(receiver)
    , _last_pitch_x10(0x7FFF)
    , _last_roll_x10(0x7FFF)
    , _last_pitch_deg(0x7FFF)
    , _last_roll_deg(0x7FFF) {}

// Realizes getLvglScreen(): Return the LVGL screen object for this UI
lv_obj_t* AttitudeUI::getLvglScreen() const {
    return ui_AttitudeScreen;
}

// Realizes begin(): Initialize
void AttitudeUI::begin() {
    if (_initialized) return;

    // Set rotation pivot to the center of ImageHorizon element
    // PNG is 680x4, center point is 340, 2
    lv_img_set_pivot(ui_ImageHorizon, 340, 2);

    // Set ContainerLevelingDialog hidden
    lv_obj_add_flag(ui_ContainerLevelingDialog, LV_OBJ_FLAG_HIDDEN);

    _level_state = LevelState::IDLE;
    _initialized = true;

    this->showWaiting();
}

// Realizes update(): fetch data from receiver and update UI
void AttitudeUI::update() {
    if (!_initialized) return;

    bool is_connected = _receiver.isConnected(CONNECTION_TIMEOUT_MS);

    if (!is_connected) {
        if (_last_connected) {
            // Transition: connected → disconnected
            this->showWaiting();
            _last_connected = false;
        }
    } else {
        _last_connected = true;

        if (_receiver.hasNewData()) {
            HeadingData data = _receiver.getData();
            this->updateHorizon(data.pitch_x10, data.roll_x10);
            this->updatePitchLabel(data.getPitchDeg());
            this->updateRollLabel(data.getRollDeg());
        }
    }

    // Always tick the level state machine regardless of connection
    this->updateLevelState();
}

// Realizes onButtonPress(): level state machine — handle knob press
void AttitudeUI::onButtonPress() {
    if (!_initialized) return;

    switch (_level_state) {
        case LevelState::IDLE:
            // First press: show confirmation dialog
            this->setLevelState(LevelState::CONFIRM_WAIT);
            break;

        case LevelState::CONFIRM_WAIT:
            // Second press: send level command
            if (_receiver.sendLevelCommand()) {
                this->setLevelState(LevelState::SENDING);
            } else {
                this->setLevelState(LevelState::FAILED);
            }
            break;

        case LevelState::SENDING:
        case LevelState::SUCCESS:
        case LevelState::FAILED:
            // Ignore presses during these states
            break;
    }
}

// Realizes onLeave(): cancel level operation when leaving screen
void AttitudeUI::onLeave() {
    this->cancelLevelOperation();
}

// === P R I V A T E ===

// Level state machine — cancel operation and return to idle
void AttitudeUI::cancelLevelOperation() {
    if (_level_state != LevelState::IDLE) this->setLevelState(LevelState::IDLE);
}

// Level state machine — advance timeouts and check responses
void AttitudeUI::updateLevelState() {
    if (_level_state == LevelState::IDLE) return;

    uint32_t elapsed = millis() - _state_start_time;

    switch (_level_state) {
        case LevelState::CONFIRM_WAIT:
            if (elapsed >= CONFIRM_TIMEOUT_MS) this->setLevelState(LevelState::IDLE);
            break;

        case LevelState::SENDING:
            if (_receiver.hasLevelResponse()) {
                bool success = _receiver.getLevelResult();
                this->setLevelState(success ? LevelState::SUCCESS : LevelState::FAILED);
            } else if (elapsed >= SENDING_TIMEOUT_MS) {
                this->setLevelState(LevelState::FAILED);
            }
            break;

        case LevelState::SUCCESS:
            if (elapsed >= SUCCESS_DISPLAY_MS) this->setLevelState(LevelState::IDLE);
            break;

        case LevelState::FAILED:
            if (elapsed >= FAILED_DISPLAY_MS) this->setLevelState(LevelState::IDLE);
            break;

        case LevelState::IDLE:
            // Already handled above
            break;
    }
}

// Update AttitudeScreen to show "waiting for data"
void AttitudeUI::showWaiting() {
    if (!_initialized) return;

    // Pitch and roll UI labels to show "---"
    lv_label_set_text(ui_LabelPitch, "---");
    lv_label_set_text(ui_LabelRoll, "---");

    // Horizon to the neutral position
    lv_obj_set_y(ui_ImageHorizon, 0);
    lv_img_set_angle(ui_ImageHorizon, 0);

    // Reset cached values
    _last_pitch_x10 = 0x7FFF;
    _last_roll_x10  = 0x7FFF;
    _last_pitch_deg = 0x7FFF;
    _last_roll_deg  = 0x7FFF;
}

// Update artificial horizon based on pitch and roll values
void AttitudeUI::updateHorizon(int16_t pitch_x10, int16_t roll_x10) {
    // Update only if changed
    if (pitch_x10 == _last_pitch_x10 && roll_x10 == _last_roll_x10) return;
    _last_pitch_x10 = pitch_x10;
    _last_roll_x10  = roll_x10;

    // PITCH: Move ImageHorizon UI element vertically
    // Bow down - pitch down - horizon up
    // lv_obj_set_y: positive value moves object down in relation to align-point
    // ImageHorizon is ALIGN_CENTER, y=0 is the center point
    int16_t y_offset = (pitch_x10 * PITCH_SCALE) / 10;
    lv_obj_set_y(ui_ImageHorizon, y_offset);

    // ROLL: Rotate ImageHorizon UI element
    // Roll port side → horizon rotates starboard
    // lv_img_set_angle: positive angle = clockwise, uses 0.1° resolution
    lv_img_set_angle(ui_ImageHorizon, -roll_x10);
}

// Update UI label element for pitch value
void AttitudeUI::updatePitchLabel(int16_t pitch_deg) {
    if (pitch_deg == _last_pitch_deg) return;
    _last_pitch_deg = pitch_deg;

    char buf[16];
    // Format: "+003°" or "-012°"
    snprintf(buf, sizeof(buf), "%+04d°", pitch_deg);
    lv_label_set_text(ui_LabelPitch, buf);
}

// Update UI label element for roll value
void AttitudeUI::updateRollLabel(int16_t roll_deg) {
    if (roll_deg == _last_roll_deg) return;
    _last_roll_deg = roll_deg;

    char buf[16];
    // Format: "+003°" or "-012°"
    snprintf(buf, sizeof(buf), "%+04d°", roll_deg);
    lv_label_set_text(ui_LabelRoll, buf);
}

// Level state machine — update UI dialog element
void AttitudeUI::updateLevelDialog() {
    switch (_level_state) {
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

// Level state machine — set new state and update dialog
void AttitudeUI::setLevelState(LevelState new_state) {
    _level_state = new_state;
    _state_start_time = millis();
    this->updateLevelDialog();
}
