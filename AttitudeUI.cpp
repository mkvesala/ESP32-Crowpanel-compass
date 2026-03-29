#include "AttitudeUI.h"
#include "ui.h"

// === P U B L I C ===

// Constructor
AttitudeUI::AttitudeUI(ESPNowReceiver &receiver)
    : _receiver(receiver) {}

// Realizes getLvglScreen(): Return the LVGL screen object for this UI
lv_obj_t* AttitudeUI::getLvglScreen() const {
    return ui_AttitudeScreen;
}

// Realizes begin(): Initialize
void AttitudeUI::begin() {
    if (_initialized) return;

    // Create live horizon image line programmatically.
    // Parent: ui_ContainerHorizonGroup (680×680, LV_ALIGN_CENTER on screen).
    // Source: ui_img_horizonline_png (680×4 px, LV_COLOR_FORMAT_NATIVE_WITH_ALPHA).
    // inner_align: LV_IMAGE_ALIGN_DEFAULT — LVGL 9 defaults to TILE which silently disables
    //   lv_image_set_rotation(); must be set explicitly BEFORE lv_image_set_pivot (pivot resets to
    //   (0,0) when inner_align is changed in LVGL 9).
    // Pivot (340, 2): center of 680×4 image → maps to container center → screen center (240, 240).
    // lv_obj_set_align(LV_ALIGN_CENTER) + lv_obj_set_y(y_offset): y_offset is relative to the
    //   aligned (centered) position; 0 = neutral horizon, positive = bow up, negative = bow down.
    _img_horizon = lv_image_create(ui_ContainerHorizonGroup);
    lv_image_set_src(_img_horizon, &ui_img_horizonline_png);
    lv_obj_set_align(_img_horizon, LV_ALIGN_CENTER);
    lv_image_set_inner_align(_img_horizon, LV_IMAGE_ALIGN_DEFAULT);
    lv_image_set_pivot(_img_horizon, 340, 2);
    lv_obj_remove_flag(_img_horizon,
                       (lv_obj_flag_t)(LV_OBJ_FLAG_PRESS_LOCK | LV_OBJ_FLAG_CLICK_FOCUSABLE |
                                       LV_OBJ_FLAG_GESTURE_BUBBLE | LV_OBJ_FLAG_SNAPPABLE |
                                       LV_OBJ_FLAG_SCROLLABLE | LV_OBJ_FLAG_SCROLL_ELASTIC |
                                       LV_OBJ_FLAG_SCROLL_MOMENTUM | LV_OBJ_FLAG_SCROLL_CHAIN));

    // Permanently hide SquareLine-generated roll panels.
    // lv_obj_set_style_transform_rotation() on a plain lv_obj (484×4 px) causes
    // lv_timer_handler() to hang in LVGL 9 PARTIAL rendering mode — device freezes.
    // Roll lines are replaced by programmatic lv_image objects below.
    lv_obj_add_flag(ui_PanelMaxRoll, LV_OBJ_FLAG_HIDDEN);
    lv_obj_add_flag(ui_PanelMinRoll, LV_OBJ_FLAG_HIDDEN);

    // Create 680×680 transparent container for roll min/max image lines.
    // Same size as ContainerHorizonGroup: ensures the 680-px-wide image lines cover the
    // full screen width at any rotation angle without clipping at the corners.
    _container_roll_lines = lv_obj_create(ui_AttitudeScreen);
    lv_obj_remove_style_all(_container_roll_lines);
    lv_obj_set_size(_container_roll_lines, 680, 680);
    lv_obj_set_align(_container_roll_lines, LV_ALIGN_CENTER);
    lv_obj_remove_flag(_container_roll_lines,
                       (lv_obj_flag_t)(LV_OBJ_FLAG_CLICKABLE | LV_OBJ_FLAG_PRESS_LOCK |
                                       LV_OBJ_FLAG_CLICK_FOCUSABLE | LV_OBJ_FLAG_GESTURE_BUBBLE |
                                       LV_OBJ_FLAG_SNAPPABLE | LV_OBJ_FLAG_SCROLLABLE |
                                       LV_OBJ_FLAG_SCROLL_ELASTIC | LV_OBJ_FLAG_SCROLL_MOMENTUM |
                                       LV_OBJ_FLAG_SCROLL_CHAIN));
    lv_obj_add_flag(_container_roll_lines, LV_OBJ_FLAG_HIDDEN);
    // Z-index 0: drawn first (behind ContainerMinMax labels and ContainerVessel)
    lv_obj_move_to_index(_container_roll_lines, 0);

    // Create green max-roll image line (recolored copy of ui_img_horizonline_png)
    _img_max_roll = lv_image_create(_container_roll_lines);
    lv_image_set_src(_img_max_roll, &ui_img_horizonline_png);
    lv_obj_set_align(_img_max_roll, LV_ALIGN_CENTER);
    lv_image_set_inner_align(_img_max_roll, LV_IMAGE_ALIGN_DEFAULT);  // avoid TILE (LVGL 9 quirk)
    lv_image_set_pivot(_img_max_roll, 340, 2);                        // center of 680×4 image
    lv_image_set_antialias(_img_max_roll, false);
    lv_obj_set_style_image_recolor(_img_max_roll, lv_color_hex(0x00FF00), 0);
    lv_obj_set_style_image_recolor_opa(_img_max_roll, 255, 0);

    // Create red min-roll image line
    _img_min_roll = lv_image_create(_container_roll_lines);
    lv_image_set_src(_img_min_roll, &ui_img_horizonline_png);
    lv_obj_set_align(_img_min_roll, LV_ALIGN_CENTER);
    lv_image_set_inner_align(_img_min_roll, LV_IMAGE_ALIGN_DEFAULT);
    lv_image_set_pivot(_img_min_roll, 340, 2);
    lv_image_set_antialias(_img_min_roll, false);
    lv_obj_set_style_image_recolor(_img_min_roll, lv_color_hex(0xFF0000), 0);
    lv_obj_set_style_image_recolor_opa(_img_min_roll, 255, 0);

    // Reset session min/max (runtime only, not persisted)
    _min_pitch_x10 = SENTINEL;
    _max_pitch_x10 = SENTINEL;
    _min_roll_x10  = SENTINEL;
    _max_roll_x10  = SENTINEL;

    // Initialize min/max labels to "---"
    lv_label_set_text(ui_LabelMaxPitch, "---");
    lv_label_set_text(ui_LabelMinPitch, "---");
    lv_label_set_text(ui_LabelMaxRoll,  "---");
    lv_label_set_text(ui_LabelMinRoll,  "---");

    // Initialize min/max panels at center (no displacement, no rotation)
    lv_obj_set_y(ui_PanelMaxPitch, 0);
    lv_obj_set_y(ui_PanelMinPitch, 0);

    _level_state = LevelState::IDLE;
    _initialized = true;

    // Apply initial view (sets container visibility)
    this->showView(AttitudeView::ATTITUDE);

    this->showWaiting();
}

// Realizes update(): Fetch data from receiver and update UI
void AttitudeUI::update() {
    if (!_initialized) return;

    bool is_connected = _receiver.isConnected(CONNECTION_TIMEOUT_MS);

    if (!is_connected) {
        if (_last_connected) {
            // Disconnected: hide navigation lights only.
            // Last known pitch/roll values and horizon position are preserved.
            lv_obj_add_flag(ui_PanelStarboard, LV_OBJ_FLAG_HIDDEN);
            lv_obj_add_flag(ui_PanelPortside,  LV_OBJ_FLAG_HIDDEN);
            _last_connected = false;
        }
    } else {
        if (!_last_connected) {
            // Reconnected: show navigation lights
            lv_obj_remove_flag(ui_PanelStarboard, LV_OBJ_FLAG_HIDDEN);
            lv_obj_remove_flag(ui_PanelPortside,  LV_OBJ_FLAG_HIDDEN);
            _last_connected = true;
        }

        if (_receiver.hasNewData()) {
            HeadingData data = _receiver.getData();
            this->updateHorizon(data.pitch_x10, data.roll_x10);
            this->updatePitchLabel(data.getPitchDeg());
            this->updateRollLabel(data.getRollDeg());
            this->updateMinMax(data.pitch_x10, data.roll_x10);
            this->updateMinMaxPanels();
            this->updateMinMaxLabels();
        }
    }

    // Always tick the level state machine regardless of connection state
    this->updateLevelState();
}

// Realizes onButtonPress(): Cycle through internal views
void AttitudeUI::onButtonPress() {
    if (!_initialized) return;

    switch (_active_view) {
        case AttitudeView::ATTITUDE:
            this->showView(AttitudeView::MINMAX);
            break;
        case AttitudeView::MINMAX:
            this->showView(AttitudeView::LEVELING);
            break;
        case AttitudeView::LEVELING:
            // Cancel countdown/leveling and return to ATTITUDE view
            this->showView(AttitudeView::ATTITUDE);
            break;
    }
}

// Realizes onLeave(): Called when screen carousel switches away
void AttitudeUI::onLeave() {
    // Cancel any leveling operation and reset to ATTITUDE view
    this->showView(AttitudeView::ATTITUDE);
}

// === P R I V A T E ===

// Set active view and update container visibility + level state
void AttitudeUI::showView(AttitudeView view) {
    _active_view = view;

    const bool is_attitude = (view == AttitudeView::ATTITUDE);
    const bool is_minmax   = (view == AttitudeView::MINMAX);
    const bool is_leveling = (view == AttitudeView::LEVELING);

    // ContainerHorizonGroup (live horizon line): visible in ATTITUDE only
    if (is_attitude)
        lv_obj_remove_flag(ui_ContainerHorizonGroup, LV_OBJ_FLAG_HIDDEN);
    else
        lv_obj_add_flag(ui_ContainerHorizonGroup, LV_OBJ_FLAG_HIDDEN);

    // ContainerAttitudeGroup (live pitch/roll labels): visible in ATTITUDE only
    if (is_attitude)
        lv_obj_remove_flag(ui_ContainerAttitudeGroup, LV_OBJ_FLAG_HIDDEN);
    else
        lv_obj_add_flag(ui_ContainerAttitudeGroup, LV_OBJ_FLAG_HIDDEN);

    // ContainerMinMax (4 min/max lines + numeric labels): visible in MINMAX only
    if (is_minmax)
        lv_obj_remove_flag(ui_ContainerMinMax, LV_OBJ_FLAG_HIDDEN);
    else
        lv_obj_add_flag(ui_ContainerMinMax, LV_OBJ_FLAG_HIDDEN);

    // ContainerVessel (ship silhouette): visible in ATTITUDE and MINMAX
    if (!is_leveling)
        lv_obj_remove_flag(ui_ContainerVessel, LV_OBJ_FLAG_HIDDEN);
    else
        lv_obj_add_flag(ui_ContainerVessel, LV_OBJ_FLAG_HIDDEN);

    // ContainerLevelingDialog: visible in LEVELING only
    if (is_leveling)
        lv_obj_remove_flag(ui_ContainerLevelingDialog, LV_OBJ_FLAG_HIDDEN);
    else
        lv_obj_add_flag(ui_ContainerLevelingDialog, LV_OBJ_FLAG_HIDDEN);

    // Roll image lines container: visible in MINMAX only
    if (is_minmax)
        lv_obj_remove_flag(_container_roll_lines, LV_OBJ_FLAG_HIDDEN);
    else
        lv_obj_add_flag(_container_roll_lines, LV_OBJ_FLAG_HIDDEN);

    // Level state transitions
    if (is_leveling) {
        // Entering LEVELING view: start the countdown
        this->setLevelState(LevelState::COUNTDOWN);
    } else {
        // Leaving LEVELING view (or initializing to ATTITUDE/MINMAX): cancel any operation
        if (_level_state != LevelState::IDLE) {
            _level_state = LevelState::IDLE;
            _state_start_time = millis();
        }
    }
}

// Update AttitudeScreen live horizon to "waiting for data" state
void AttitudeUI::showWaiting() {
    if (!_initialized) return;

    // Live pitch/roll labels to "---"
    lv_label_set_text(ui_LabelPitch, "---");
    lv_label_set_text(ui_LabelRoll,  "---");

    // Horizon to neutral position
    lv_obj_set_y(_img_horizon, 0);
    lv_image_set_rotation(_img_horizon, 0);

    // Reset cached live values
    _last_pitch_x10 = SENTINEL;
    _last_roll_x10  = SENTINEL;
    _last_pitch_deg = SENTINEL;
    _last_roll_deg  = SENTINEL;

    // NOTE: Session min/max is NOT reset on disconnect — preserved until reboot
}

// Update live artificial horizon based on pitch and roll
void AttitudeUI::updateHorizon(int16_t pitch_x10, int16_t roll_x10) {
    if (pitch_x10 == _last_pitch_x10 && roll_x10 == _last_roll_x10) return;
    _last_pitch_x10 = pitch_x10;
    _last_roll_x10  = roll_x10;

    // PITCH: Move ImageHorizon vertically
    // Bow down (negative pitch) → horizon moves up (negative y in LVGL)
    int16_t y_offset = (pitch_x10 * PITCH_SCALE) / 10;
    lv_obj_set_y(_img_horizon, y_offset);

    // ROLL: Rotate ImageHorizon
    // Roll port (negative roll) → horizon tilts starboard (clockwise = positive angle in LVGL)
    lv_image_set_rotation(_img_horizon, -roll_x10);
}

// Update live pitch label
void AttitudeUI::updatePitchLabel(int16_t pitch_deg) {
    if (pitch_deg == _last_pitch_deg) return;
    _last_pitch_deg = pitch_deg;

    char buf[16];
    snprintf(buf, sizeof(buf), "%+04d°", pitch_deg);
    lv_label_set_text(ui_LabelPitch, buf);
}

// Update live roll label
void AttitudeUI::updateRollLabel(int16_t roll_deg) {
    if (roll_deg == _last_roll_deg) return;
    _last_roll_deg = roll_deg;

    char buf[16];
    snprintf(buf, sizeof(buf), "%+04d°", roll_deg);
    lv_label_set_text(ui_LabelRoll, buf);
}

// Update session min/max tracking
void AttitudeUI::updateMinMax(int16_t pitch_x10, int16_t roll_x10) {
    if (_max_pitch_x10 == SENTINEL || pitch_x10 > _max_pitch_x10) _max_pitch_x10 = pitch_x10;
    if (_min_pitch_x10 == SENTINEL || pitch_x10 < _min_pitch_x10) _min_pitch_x10 = pitch_x10;
    if (_max_roll_x10  == SENTINEL || roll_x10  > _max_roll_x10)  _max_roll_x10  = roll_x10;
    if (_min_roll_x10  == SENTINEL || roll_x10  < _min_roll_x10)  _min_roll_x10  = roll_x10;
}

// Update min/max panel positions and rotations in ContainerMinMax.
// Visual positions are clamped to ±MINMAX_LINE_CLAMP_X10 (±30.0°) for readability.
// Labels in updateMinMaxLabels() always show the true session values (no clamping).
void AttitudeUI::updateMinMaxPanels() {
    // PanelMaxPitch (yellow): highest pitch = bow highest up → horizon lowest on screen
    // Same formula as live ImageHorizon: positive pitch_x10 → positive y_offset → panel moves down
    if (_max_pitch_x10 != SENTINEL) {
        int16_t clamped = constrain(_max_pitch_x10, -MINMAX_LINE_CLAMP_X10, MINMAX_LINE_CLAMP_X10);
        lv_obj_set_y(ui_PanelMaxPitch, (clamped * PITCH_SCALE) / 10);
    }

    // PanelMinPitch (blue): lowest pitch = bow lowest down → horizon highest on screen
    if (_min_pitch_x10 != SENTINEL) {
        int16_t clamped = constrain(_min_pitch_x10, -MINMAX_LINE_CLAMP_X10, MINMAX_LINE_CLAMP_X10);
        lv_obj_set_y(ui_PanelMinPitch, (clamped * PITCH_SCALE) / 10);
    }

    // Roll image lines: use lv_image_set_rotation() — same proven API as live ImageHorizon.
    // Pivot (340, 2) = center of 680×4 image = screen center (LV_ALIGN_CENTER in 680×680 container).
    // Convention: -roll_x10 so ship roll right (positive) tilts line left (negative LVGL angle).
    if (_max_roll_x10 != SENTINEL) {
        int16_t clamped = constrain(_max_roll_x10, -MINMAX_LINE_CLAMP_X10, MINMAX_LINE_CLAMP_X10);
        lv_image_set_rotation(_img_max_roll, -clamped);
    }

    if (_min_roll_x10 != SENTINEL) {
        int16_t clamped = constrain(_min_roll_x10, -MINMAX_LINE_CLAMP_X10, MINMAX_LINE_CLAMP_X10);
        lv_image_set_rotation(_img_min_roll, -clamped);
    }
}

// Update min/max numeric labels in ContainerMinMax
void AttitudeUI::updateMinMaxLabels() {
    char buf[16];

    if (_max_pitch_x10 == SENTINEL) {
        lv_label_set_text(ui_LabelMaxPitch, "---");
    } else {
        snprintf(buf, sizeof(buf), "Max %+d°", _max_pitch_x10 / 10);
        lv_label_set_text(ui_LabelMaxPitch, buf);
    }

    if (_min_pitch_x10 == SENTINEL) {
        lv_label_set_text(ui_LabelMinPitch, "---");
    } else {
        snprintf(buf, sizeof(buf), "Min %+d°", _min_pitch_x10 / 10);
        lv_label_set_text(ui_LabelMinPitch, buf);
    }

    if (_max_roll_x10 == SENTINEL) {
        lv_label_set_text(ui_LabelMaxRoll, "---");
    } else {
        snprintf(buf, sizeof(buf), "Max %+d°", _max_roll_x10 / 10);
        lv_label_set_text(ui_LabelMaxRoll, buf);
    }

    if (_min_roll_x10 == SENTINEL) {
        lv_label_set_text(ui_LabelMinRoll, "---");
    } else {
        snprintf(buf, sizeof(buf), "Min %+d°", _min_roll_x10 / 10);
        lv_label_set_text(ui_LabelMinRoll, buf);
    }
}

// Level state machine — advance timeouts and auto-send
void AttitudeUI::updateLevelState() {
    if (_level_state == LevelState::IDLE) return;

    uint32_t elapsed = millis() - _state_start_time;

    switch (_level_state) {
        case LevelState::COUNTDOWN: {
            if (elapsed >= LEVELING_COUNTDOWN_MS) {
                // Countdown complete: send command
                if (_receiver.sendLevelCommand()) this->setLevelState(LevelState::SENDING);
                else                               this->setLevelState(LevelState::FAILED);
            } else {
                // Update countdown label once per second
                uint8_t remaining_s = (uint8_t)((LEVELING_COUNTDOWN_MS - elapsed + 999) / 1000);
                if (remaining_s != _last_countdown_s) {
                    _last_countdown_s = remaining_s;
                    char buf[32];
                    snprintf(buf, sizeof(buf), "Leveling in\n%d s", remaining_s);
                    lv_label_set_text(ui_LabelLevelingDialog, buf);
                }
            }
            break;
        }

        case LevelState::SENDING:
            if (_receiver.hasLevelResponse()) {
                bool success = _receiver.getLevelResult();
                this->setLevelState(success ? LevelState::SUCCESS : LevelState::FAILED);
            } else if (elapsed >= SENDING_TIMEOUT_MS) {
                this->setLevelState(LevelState::FAILED);
            }
            break;

        case LevelState::SUCCESS:
            if (elapsed >= SUCCESS_DISPLAY_MS) this->showView(AttitudeView::ATTITUDE);
            break;

        case LevelState::FAILED:
            if (elapsed >= FAILED_DISPLAY_MS)  this->showView(AttitudeView::ATTITUDE);
            break;

        case LevelState::IDLE:
            break;
    }
}

// Level state machine — update dialog label and color for the new state
void AttitudeUI::updateLevelDialog() {
    switch (_level_state) {
        case LevelState::IDLE:
            // Dialog hidden by showView() — nothing to update
            break;

        case LevelState::COUNTDOWN:
            // Label text updated each second in updateLevelState()
            // Set initial text and color here (state entry)
            lv_obj_set_style_text_color(ui_LabelLevelingDialog, lv_color_hex(0xFFFFFF), 0);
            _last_countdown_s = 0;  // Force label update on first tick
            break;

        case LevelState::SENDING:
            lv_label_set_text(ui_LabelLevelingDialog, "Leveling...");
            lv_obj_set_style_text_color(ui_LabelLevelingDialog, lv_color_hex(0xFFFFFF), 0);
            break;

        case LevelState::SUCCESS:
            lv_label_set_text(ui_LabelLevelingDialog, "Success!");
            lv_obj_set_style_text_color(ui_LabelLevelingDialog, lv_color_hex(0x00FF00), 0);
            break;

        case LevelState::FAILED:
            lv_label_set_text(ui_LabelLevelingDialog, "Failed!");
            lv_obj_set_style_text_color(ui_LabelLevelingDialog, lv_color_hex(0xFF0000), 0);
            break;
    }
}

// Level state machine — set new state, record timestamp, update dialog
void AttitudeUI::setLevelState(LevelState new_state) {
    _level_state = new_state;
    _state_start_time = millis();
    this->updateLevelDialog();
}
