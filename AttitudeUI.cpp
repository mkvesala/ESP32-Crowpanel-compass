#include "AttitudeUI.h"
#include "ui.h"

// === S T A T I C ===

// Shared image descriptor for all three horizon image lines
static uint8_t s_horizonline_buf[680 * 4 * 2];
static lv_image_dsc_t s_horizonline_dsc;

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

    // Initialize shared image descriptor
    memset(s_horizonline_buf, 0xFF, sizeof(s_horizonline_buf));
    s_horizonline_dsc.header.magic  = LV_IMAGE_HEADER_MAGIC;
    s_horizonline_dsc.header.cf     = LV_COLOR_FORMAT_RGB565;
    s_horizonline_dsc.header.w      = 680;
    s_horizonline_dsc.header.h      = 4;
    s_horizonline_dsc.header.stride = 680 * 2;
    s_horizonline_dsc.data_size     = sizeof(s_horizonline_buf);
    s_horizonline_dsc.data          = s_horizonline_buf;

    // Create live horizon image line
    // Parent: ui_ContainerHorizonGroup (680×680, LV_ALIGN_CENTER)
    // inner_align: LV_IMAGE_ALIGN_DEFAULT — LVGL 9 defaults to TILE which silently disables
    //   lv_image_set_rotation(); must be set explicitly BEFORE lv_image_set_pivot (pivot resets to
    //   (0,0) when inner_align is changed in LVGL 9).
    // Pivot (340, 2): center of 680×4 image → maps to container center → screen center (240, 240).
    // lv_obj_set_align(LV_ALIGN_CENTER) + lv_obj_set_y(y_offset): y_offset is relative to the
    //   aligned (centered) position; 0 = neutral horizon, positive = bow up, negative = bow down.
    _img_horizon = lv_image_create(ui_ContainerHorizonGroup);
    lv_image_set_src(_img_horizon, &s_horizonline_dsc);
    lv_obj_set_align(_img_horizon, LV_ALIGN_CENTER);
    lv_image_set_inner_align(_img_horizon, LV_IMAGE_ALIGN_DEFAULT);
    lv_image_set_pivot(_img_horizon, 340, 2);
    lv_obj_remove_flag(_img_horizon,
                       (lv_obj_flag_t)(LV_OBJ_FLAG_PRESS_LOCK | LV_OBJ_FLAG_CLICK_FOCUSABLE |
                                       LV_OBJ_FLAG_GESTURE_BUBBLE | LV_OBJ_FLAG_SNAPPABLE |
                                       LV_OBJ_FLAG_SCROLLABLE | LV_OBJ_FLAG_SCROLL_ELASTIC |
                                       LV_OBJ_FLAG_SCROLL_MOMENTUM | LV_OBJ_FLAG_SCROLL_CHAIN));

    // Create another 680×680 transparent container for roll min/max image lines
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

    // Create green max-roll image line (recolored from white s_horizonline_dsc)
    _img_max_roll = lv_image_create(_container_roll_lines);
    lv_image_set_src(_img_max_roll, &s_horizonline_dsc);
    lv_obj_set_align(_img_max_roll, LV_ALIGN_CENTER);
    lv_image_set_inner_align(_img_max_roll, LV_IMAGE_ALIGN_DEFAULT);  // avoid TILE (LVGL 9)
    lv_image_set_pivot(_img_max_roll, 340, 2); 
    lv_image_set_antialias(_img_max_roll, false);
    lv_obj_set_style_image_recolor(_img_max_roll, lv_color_hex(0x00FF00), 0);
    lv_obj_set_style_image_recolor_opa(_img_max_roll, 255, 0);

    // Create red min-roll image line (recolored from white s_horizonline_dsc)
    _img_min_roll = lv_image_create(_container_roll_lines);
    lv_image_set_src(_img_min_roll, &s_horizonline_dsc);
    lv_obj_set_align(_img_min_roll, LV_ALIGN_CENTER);
    lv_image_set_inner_align(_img_min_roll, LV_IMAGE_ALIGN_DEFAULT);
    lv_image_set_pivot(_img_min_roll, 340, 2);
    lv_image_set_antialias(_img_min_roll, false);
    lv_obj_set_style_image_recolor(_img_min_roll, lv_color_hex(0xFF0000), 0);
    lv_obj_set_style_image_recolor_opa(_img_min_roll, 255, 0);

    // Initialize min/max labels to "---"
    lv_label_set_text(ui_LabelMaxPitch, "---");
    lv_label_set_text(ui_LabelMinPitch, "---");
    lv_label_set_text(ui_LabelMaxRoll, "---");
    lv_label_set_text(ui_LabelMinRoll, "---");

    // Initialize min/max panels at center (no displacement, no rotation)
    lv_obj_set_y(ui_PanelMaxPitch, 0);
    lv_obj_set_y(ui_PanelMinPitch, 0);

    _initialized = true;

    // Apply initial view (sets container visibility)
    this->showView(AttitudeView::ATTITUDE);

    this->showWaiting();

    // Also clears ui_ImageCaution, which SquareLine exports visible
    this->showDepthWaiting();
}

// Realizes update(): Fetch data from receiver and update UI
void AttitudeUI::update() {
    if (!_initialized) return;

    bool is_connected = _receiver.isConnected(CONNECTION_TIMEOUT_MS);

    if (!is_connected) {
        if (_last_connected) {
            // Disconnected: hide navigation lights
            lv_obj_add_flag(ui_PanelStarboard, LV_OBJ_FLAG_HIDDEN);
            lv_obj_add_flag(ui_PanelPortside, LV_OBJ_FLAG_HIDDEN);
            _last_connected = false;
        }
    } else {
        if (!_last_connected) {
            // Reconnected: show navigation lights
            lv_obj_remove_flag(ui_PanelStarboard, LV_OBJ_FLAG_HIDDEN);
            lv_obj_remove_flag(ui_PanelPortside, LV_OBJ_FLAG_HIDDEN);
            _last_connected = true;
        }

        if (_receiver.hasNewData()) {
            HeadingData data = _receiver.getData();
            this->updateHorizon(data.pitch_x10, data.roll_x10);
            this->updatePitchLabel(data.getPitchDeg());
            this->updateRollLabel(data.getRollDeg());
            this->updateMinMaxPanels();
            this->updateMinMaxLabels();
        }
    }

    // Depth has its own gateway and its own freshness rules, independent of the heading
    // link above — only the active view pays for it.
    if (_active_view == AttitudeView::DEPTH) this->updateDepth();

}

// Realizes onButtonPress(): Cycle ATTITUDE → MINMAX → DEPTH → ATTITUDE
void AttitudeUI::onButtonPress() {
    if (!_initialized) return;
    uint8_t next = (static_cast<uint8_t>(_active_view) + 1) % static_cast<uint8_t>(AttitudeView::COUNT);
    this->showView(static_cast<AttitudeView>(next));
}

// Realizes onLeave(): Reset to ATTITUDE view when navigating away
void AttitudeUI::onLeave() {
    this->showView(AttitudeView::ATTITUDE);
}

// === P R I V A T E ===

// Set active view and update container visibility
void AttitudeUI::showView(AttitudeView view) {
    _active_view = view;

    const bool is_attitude = (view == AttitudeView::ATTITUDE);
    const bool is_minmax   = (view == AttitudeView::MINMAX);
    const bool is_depth    = (view == AttitudeView::DEPTH);

    // ContainerHorizonGroup (live horizon line): visible in ATTITUDE only
    if (is_attitude) lv_obj_remove_flag(ui_ContainerHorizonGroup, LV_OBJ_FLAG_HIDDEN);
    else lv_obj_add_flag(ui_ContainerHorizonGroup, LV_OBJ_FLAG_HIDDEN);

    // ContainerAttitudeGroup (live pitch/roll labels): visible in ATTITUDE only
    if (is_attitude) lv_obj_remove_flag(ui_ContainerAttitudeGroup, LV_OBJ_FLAG_HIDDEN);
    else lv_obj_add_flag(ui_ContainerAttitudeGroup, LV_OBJ_FLAG_HIDDEN);

    // ContainerMinMax (4 min/max lines + numeric labels): visible in MINMAX only
    if (is_minmax) lv_obj_remove_flag(ui_ContainerMinMax, LV_OBJ_FLAG_HIDDEN);
    else lv_obj_add_flag(ui_ContainerMinMax, LV_OBJ_FLAG_HIDDEN);

    // ContainerDepth (surface/keel/bottom panels + depth labels): visible in DEPTH only
    if (is_depth) lv_obj_remove_flag(ui_ContainerDepth, LV_OBJ_FLAG_HIDDEN);
    else lv_obj_add_flag(ui_ContainerDepth, LV_OBJ_FLAG_HIDDEN);

    // ContainerVessel (ship silhouette): always visible. It is the last screen child, so in
    // DEPTH the hull draws on top of ui_PanelBottom.
    lv_obj_remove_flag(ui_ContainerVessel, LV_OBJ_FLAG_HIDDEN);

    // Roll image lines container: visible in MINMAX only
    if (is_minmax) lv_obj_remove_flag(_container_roll_lines, LV_OBJ_FLAG_HIDDEN);
    else lv_obj_add_flag(_container_roll_lines, LV_OBJ_FLAG_HIDDEN);

    // Entering DEPTH: drop the render caches so the first update() tick redraws immediately
    if (is_depth) {
        _last_bottom_y        = SENTINEL;
        _last_depth_render_ms = 0;
    }
}

// Update AttitudeScreen live horizon to "waiting for data" state
void AttitudeUI::showWaiting() {
    if (!_initialized) return;

    // Live pitch/roll labels to "---"
    lv_label_set_text(ui_LabelPitch, "---");
    lv_label_set_text(ui_LabelRoll, "---");

    // Horizon to neutral position
    lv_obj_set_y(_img_horizon, 0);
    lv_image_set_rotation(_img_horizon, 0);

    // Reset cached live values
    _last_pitch_x10 = SENTINEL;
    _last_roll_x10  = SENTINEL;
    _last_pitch_deg = SENTINEL;
    _last_roll_deg  = SENTINEL;

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

// Update min/max panel positions and rotations in ContainerMinMax.
// Visual positions are clamped to ±MINMAX_LINE_CLAMP_X10 (±30.0°) for readability.
// Labels in updateMinMaxLabels() always show the true session values (no clamping).
void AttitudeUI::updateMinMaxPanels() {
    const int16_t max_pitch = ESPNowReceiver::getMaxPitch_x10();
    const int16_t min_pitch = ESPNowReceiver::getMinPitch_x10();
    const int16_t max_roll  = ESPNowReceiver::getMaxRoll_x10();
    const int16_t min_roll  = ESPNowReceiver::getMinRoll_x10();
    const int16_t SEN       = ESPNowReceiver::MINMAX_SENTINEL;

    // PanelMaxPitch (yellow): highest pitch = bow highest up → horizon lowest on screen
    // Same formula as live ImageHorizon: positive pitch_x10 → positive y_offset → panel moves down
    if (max_pitch != SEN) {
        int16_t clamped = constrain(max_pitch, -MINMAX_LINE_CLAMP_X10, MINMAX_LINE_CLAMP_X10);
        lv_obj_set_y(ui_PanelMaxPitch, (clamped * PITCH_SCALE) / 10);
    }

    // PanelMinPitch (blue): lowest pitch = bow lowest down → horizon highest on screen
    if (min_pitch != SEN) {
        int16_t clamped = constrain(min_pitch, -MINMAX_LINE_CLAMP_X10, MINMAX_LINE_CLAMP_X10);
        lv_obj_set_y(ui_PanelMinPitch, (clamped * PITCH_SCALE) / 10);
    }

    // Roll image lines: -roll_x10 so ship roll right (positive) tilts line left (negative LVGL angle).
    if (max_roll != SEN) {
        int16_t clamped = constrain(max_roll, -MINMAX_LINE_CLAMP_X10, MINMAX_LINE_CLAMP_X10);
        lv_image_set_rotation(_img_max_roll, -clamped);
    }

    if (min_roll != SEN) {
        int16_t clamped = constrain(min_roll, -MINMAX_LINE_CLAMP_X10, MINMAX_LINE_CLAMP_X10);
        lv_image_set_rotation(_img_min_roll, -clamped);
    }
}

// Update min/max numeric labels in ContainerMinMax
void AttitudeUI::updateMinMaxLabels() {
    const int16_t max_pitch = ESPNowReceiver::getMaxPitch_x10();
    const int16_t min_pitch = ESPNowReceiver::getMinPitch_x10();
    const int16_t max_roll  = ESPNowReceiver::getMaxRoll_x10();
    const int16_t min_roll  = ESPNowReceiver::getMinRoll_x10();
    const int16_t SEN       = ESPNowReceiver::MINMAX_SENTINEL;
    char buf[16];

    if (max_pitch == SEN) lv_label_set_text(ui_LabelMaxPitch, "---");
    else { snprintf(buf, sizeof(buf), "Max %+d°", max_pitch / 10); lv_label_set_text(ui_LabelMaxPitch, buf); }

    if (min_pitch == SEN) lv_label_set_text(ui_LabelMinPitch, "---");
    else { snprintf(buf, sizeof(buf), "Min %+d°", min_pitch / 10); lv_label_set_text(ui_LabelMinPitch, buf); }

    if (max_roll == SEN) lv_label_set_text(ui_LabelMaxRoll, "---");
    else { snprintf(buf, sizeof(buf), "Max %+d°", max_roll / 10); lv_label_set_text(ui_LabelMaxRoll, buf); }

    if (min_roll == SEN) lv_label_set_text(ui_LabelMinRoll, "---");
    else { snprintf(buf, sizeof(buf), "Min %+d°", min_roll / 10); lv_label_set_text(ui_LabelMinRoll, buf); }
}

// Update the depth view: labels, sea bottom position and the shallow-water caution.
// Only called while DEPTH is the active view, and throttled to DEPTH_RENDER_INTERVAL_MS
// because ui_PanelBottom is 484×185 and the sounder only updates at ~1 Hz anyway.
void AttitudeUI::updateDepth() {
    const uint32_t now = millis();
    if ((now - _last_depth_render_ms) < DEPTH_RENDER_INTERVAL_MS) return;
    _last_depth_render_ms = now;

    // getDepthData() returns the latest value whether or not the has-new flag was still set;
    // lastDepthRxMillis() is maintained in the RX callback, so entering this view mid-stream
    // shows the correct value on the first tick instead of blanking for a second.
    const DepthDelta depth = _receiver.getDepthData();
    const uint32_t   rx    = _receiver.lastDepthRxMillis();

    const bool link_ok = (rx > 0) && ((now - rx) < DEPTH_TIMEOUT_MS);
    const bool feed_ok = (depth.age_ms != UINT32_MAX) && (depth.age_ms < DEPTH_FEED_MAX_AGE_MS);

    // Either half can be NAN on its own. The draft is a constant, so whichever half arrived
    // determines the other; only when both are missing is there nothing to show.
    float below_keel    = depth.below_keel_m;
    float below_surface = depth.below_surface_m;
    if (isnan(below_keel)    && !isnan(below_surface)) below_keel    = below_surface - DRAFT_M;
    if (isnan(below_surface) && !isnan(below_keel))    below_surface = below_keel    + DRAFT_M;

    const bool valid = link_ok && feed_ok && !isnan(below_keel);

    if (!valid) {
        // Falling edge only — a frozen depth is worse than none, but blanking every tick
        // would redraw the labels four times a second for nothing.
        if (_last_depth_valid) this->showDepthWaiting();
        _last_depth_valid = false;
        return;
    }
    _last_depth_valid = true;

    // Labels, cached in 0.1 m units so an unchanged reading does not invalidate the label
    char buf[16];
    const int16_t keel_x10 = (int16_t)lroundf(below_keel * 10.0f);
    if (keel_x10 != _last_below_keel_x10) {
        _last_below_keel_x10 = keel_x10;
        snprintf(buf, sizeof(buf), "%.1f", below_keel);
        lv_label_set_text(ui_LabelDptBelowKeel, buf);
    }

    const int16_t surface_x10 = (int16_t)lroundf(below_surface * 10.0f);
    if (surface_x10 != _last_below_surface_x10) {
        _last_below_surface_x10 = surface_x10;
        snprintf(buf, sizeof(buf), "%.1f", below_surface);
        lv_label_set_text(ui_LabelDptBelowSurface, buf);
    }

    // Sea bottom: top edge sits at PANEL_BOTTOM_Y_AGROUND on the keel line, one PX_PER_METRE
    // lower per metre of water under the keel. Negative depth (aground) clamps to the keel.
    // Kept in float until the hidden test, so a wildly deep reading can never wrap the cast.
    const float clamped     = (below_keel > 0.0f) ? below_keel : 0.0f;
    const float y_float     = PANEL_BOTTOM_Y_AGROUND + clamped * PX_PER_METRE;
    const float y_offscreen = PANEL_BOTTOM_Y_AGROUND + PANEL_BOTTOM_H;

    // Past that offset the whole panel has slid below the screen edge — nothing to draw
    const bool bottom_hidden = (y_float >= y_offscreen);
    if (bottom_hidden != _last_bottom_hidden) {
        _last_bottom_hidden = bottom_hidden;
        if (bottom_hidden) lv_obj_add_flag(ui_PanelBottom, LV_OBJ_FLAG_HIDDEN);
        else lv_obj_remove_flag(ui_PanelBottom, LV_OBJ_FLAG_HIDDEN);
    }
    if (!bottom_hidden) {
        const int16_t y = (int16_t)lroundf(y_float);   // bounded to [148, 333] by the test above
        if (y != _last_bottom_y) {
            _last_bottom_y = y;
            lv_obj_set_y(ui_PanelBottom, y);
        }
    }

    // Caution: less than one draft of water under the keel
    const bool caution = (below_keel < DRAFT_M);
    if (caution != _last_caution_shown) {
        _last_caution_shown = caution;
        if (caution) lv_obj_remove_flag(ui_ImageCaution, LV_OBJ_FLAG_HIDDEN);
        else lv_obj_add_flag(ui_ImageCaution, LV_OBJ_FLAG_HIDDEN);
    }
}

// Update the depth view to "no depth data" state: blank labels, no sea bottom, no caution.
// ui_LabelAirHeight and ui_LabelDraft are static vessel dimensions and stay as exported.
void AttitudeUI::showDepthWaiting() {
    lv_label_set_text(ui_LabelDptBelowKeel, "--.-");
    lv_label_set_text(ui_LabelDptBelowSurface, "--.-");

    lv_obj_add_flag(ui_PanelBottom, LV_OBJ_FLAG_HIDDEN);
    lv_obj_add_flag(ui_ImageCaution, LV_OBJ_FLAG_HIDDEN);

    _last_bottom_hidden     = true;
    _last_caution_shown     = false;
    _last_bottom_y          = SENTINEL;
    _last_below_keel_x10    = SENTINEL;
    _last_below_surface_x10 = SENTINEL;
}

