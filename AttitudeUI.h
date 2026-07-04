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
//   - Cycle internal views (ATTITUDE → MINMAX) via onButtonPress()
//   - Reset to ATTITUDE view on screen leave via onLeave()
// - Views (cycled with knob button press):
//   1. ATTITUDE  — live horizon + pitch/roll labels + ship silhouette
//   2. MINMAX    — 4 static min/max lines + numeric labels + ship silhouette
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

    // Internal view (cycled with knob button press)
    enum class AttitudeView {
        ATTITUDE,   // Live horizon, pitch/roll labels, ship silhouette
        MINMAX,     // Static min/max lines, numeric labels, ship silhouette
    };

    ESPNowReceiver &_receiver;

    // Connection timeout before showing waiting state
    static constexpr uint32_t CONNECTION_TIMEOUT_MS = 5000;

    // Scaling: pixels per degree (pitch vertical displacement)
    static constexpr int16_t PITCH_SCALE = 3;

    // Visual clamp for MINMAX lines: lines stop moving at this angle for readability.
    // Labels always show the true session min/max values regardless of clamping.
    static constexpr int16_t MINMAX_LINE_CLAMP_X10 = 300;  // ±30.0°

    // Sentinel for unset min/max (int16_t, out of range for pitch ±900 and roll ±1800)
    static constexpr int16_t SENTINEL = 0x7FFF;

    // State
    AttitudeView _active_view = AttitudeView::ATTITUDE;
    bool         _initialized = false;

    // Cached live horizon values (sentinels = not yet set)
    int16_t _last_pitch_x10  = SENTINEL;
    int16_t _last_roll_x10   = SENTINEL;
    int16_t _last_pitch_deg  = SENTINEL;
    int16_t _last_roll_deg   = SENTINEL;
    bool _last_connected  = false;

    // Programmatically created image lines (not part of SquareLine Studio export)
    // _img_horizon: live artificial horizon (child of ui_ContainerHorizonGroup)
    // _img_max_roll / _img_min_roll: MINMAX roll lines (children of _container_roll_lines)
    lv_obj_t* _img_horizon          = nullptr;  // white, live horizon
    lv_obj_t* _container_roll_lines = nullptr;  // 680×680 transparent
    lv_obj_t* _img_max_roll         = nullptr;  // green
    lv_obj_t* _img_min_roll         = nullptr;  // red

    // View management
    void showView(AttitudeView view);

    // Live horizon updates
    void showWaiting();
    void updateHorizon(int16_t pitch_x10, int16_t roll_x10);
    void updatePitchLabel(int16_t pitch_deg);
    void updateRollLabel(int16_t roll_deg);

    // Min/max display
    void updateMinMaxPanels();
    void updateMinMaxLabels();

};

