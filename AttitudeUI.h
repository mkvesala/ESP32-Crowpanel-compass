#pragma once

#include <Arduino.h>
#include <lvgl.h>
#include "IScreenUI.h"
#include "ESPNowReceiver.h"

// === C L A S S  A T T I T U D E U I ===
//
// - Class AttitudeUI - responsible for managing SquareLine generated UI elements on AttitudeScreen
// - Realizes: IScreenUI
// - Fetches data from ESPNowReceiver in update()
// - Initialize: _attitudeUI.begin()
// - Update in loop(): via ScreenManager → IScreenUI::update()
// - Provides public API to:
//   - Cycle internal views (ATTITUDE → MINMAX → LEVELING) via onButtonPress()
//   - Cancel leveling on screen leave via onLeave()
// - Views (cycled with knob button press):
//   1. ATTITUDE  — live horizon + pitch/roll labels + ship silhouette
//   2. MINMAX    — 4 static min/max lines + numeric labels + ship silhouette
//   3. LEVELING  — countdown dialog, auto-sends command at 0, knob press cancels
// - Pitch: bow down → pitch negative → horizon moves up
// - Roll:  roll port → roll negative → horizon tilts starboard (clockwise)
// - Min/max: runtime only, not persisted to NVS, resets on reboot
// - Owned by: CrowPanelApplication

class AttitudeUI : public IScreenUI {

public:

    explicit AttitudeUI(ESPNowReceiver &receiver);

    void begin() override;
    lv_obj_t* getLvglScreen() const override;
    void update() override;
    void onButtonPress() override;
    void onLeave() override;

private:

    // === V I E W ===

    // Internal view (cycled with knob button press)
    enum class AttitudeView {
        ATTITUDE,   // Live horizon, pitch/roll labels, ship silhouette
        MINMAX,     // Static min/max lines, numeric labels, ship silhouette
        LEVELING    // Countdown dialog (auto-sends command)
    };

    // === L E V E L  S T A T E ===

    // Leveling sub-state machine (active only in LEVELING view)
    enum class LevelState {
        IDLE,       // No leveling in progress
        COUNTDOWN,  // Countdown running (5 s), label updated every second
        SENDING,    // Command sent, waiting for response
        SUCCESS,    // "Success!" visible briefly
        FAILED      // "Failed!" visible briefly
    };

    ESPNowReceiver &_receiver;

    // Connection timeout before showing waiting state
    static constexpr uint32_t CONNECTION_TIMEOUT_MS = 3000;

    // Scaling: pixels per degree (pitch vertical displacement)
    static constexpr int16_t PITCH_SCALE = 3;

    // Visual clamp for MINMAX lines: lines stop moving at this angle for readability.
    // Labels always show the true session min/max values regardless of clamping.
    static constexpr int16_t MINMAX_LINE_CLAMP_X10 = 300;  // ±30.0°

    // Sentinel for unset min/max (int16_t, out of range for pitch ±900 and roll ±1800)
    static constexpr int16_t SENTINEL = 0x7FFF;

    // === S T A T E ===

    AttitudeView _active_view = AttitudeView::ATTITUDE;
    LevelState   _level_state = LevelState::IDLE;
    uint32_t     _state_start_time = 0;
    bool         _initialized = false;

    // Cached live horizon values (sentinels = not yet set)
    int16_t _last_pitch_x10  = SENTINEL;
    int16_t _last_roll_x10   = SENTINEL;
    int16_t _last_pitch_deg  = SENTINEL;
    int16_t _last_roll_deg   = SENTINEL;
    bool    _last_connected  = false;

    // Session min/max (SENTINEL = no data yet)
    int16_t _min_pitch_x10 = SENTINEL;
    int16_t _max_pitch_x10 = SENTINEL;
    int16_t _min_roll_x10  = SENTINEL;
    int16_t _max_roll_x10  = SENTINEL;

    // Countdown display — last rendered second (avoid redundant label updates)
    uint8_t _last_countdown_s = 0;

    // === T I M E O U T S  (ms) ===
    static constexpr uint32_t LEVELING_COUNTDOWN_MS = 5000;
    static constexpr uint32_t SENDING_TIMEOUT_MS    = 3000;
    static constexpr uint32_t SUCCESS_DISPLAY_MS    = 2000;
    static constexpr uint32_t FAILED_DISPLAY_MS     = 2000;

    // Programmatically created image lines (lv_image — lv_image_set_rotation() proven safe in LVGL 9).
    // All three horizon lines share the same source descriptor (s_horizonline_dsc, 680×4 px,
    // white RGB565 rectangle defined in AttitudeUI.cpp) and pivot (340, 2) —
    // maps to screen center via their respective 680×680 containers. No PNG file required.
    //
    // _img_horizon: live artificial horizon (child of ui_ContainerHorizonGroup).
    //   Created here instead of SquareLine to ensure LV_IMAGE_ALIGN_DEFAULT (not TILE) and
    //   correct pivot from the start — avoiding the two fixup calls that were needed before.
    //
    // _img_max_roll / _img_min_roll: MINMAX roll lines (children of _container_roll_lines).
    //   ui_PanelMaxRoll / ui_PanelMinRoll were removed from SquareLine —
    //   lv_obj_set_style_transform_rotation() on plain lv_obj causes lv_timer_handler() hang
    //   in LVGL 9 PARTIAL mode. Roll lines are entirely programmatic lv_image objects.
    lv_obj_t* _img_horizon          = nullptr;  // white, live horizon, child of ContainerHorizonGroup
    lv_obj_t* _container_roll_lines = nullptr;  // 680×680 transparent, shown in MINMAX only
    lv_obj_t* _img_max_roll         = nullptr;  // green, lv_image_set_rotation()
    lv_obj_t* _img_min_roll         = nullptr;  // red,   lv_image_set_rotation()

    // === P R I V A T E  M E T H O D S ===

    // View management
    void showView(AttitudeView view);

    // Live horizon updates
    void showWaiting();
    void updateHorizon(int16_t pitch_x10, int16_t roll_x10);
    void updatePitchLabel(int16_t pitch_deg);
    void updateRollLabel(int16_t roll_deg);

    // Min/max tracking and display
    void updateMinMax(int16_t pitch_x10, int16_t roll_x10);
    void updateMinMaxPanels();
    void updateMinMaxLabels();

    // Level state machine
    void setLevelState(LevelState new_state);
    void updateLevelState();
    void updateLevelDialog();

};
