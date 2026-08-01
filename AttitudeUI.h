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
//   3. DEPTH     — surface/keel lines + moving sea bottom + depth labels + ship silhouette
// - Pitch: bow down → pitch negative → horizon moves up
// - Roll:  roll port → roll negative → horizon tilts starboard (clockwise)
// - Min/max: runtime only, not persisted to NVS, resets on reboot
// - Active view: runtime only, resets to ATTITUDE on onLeave()
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
    enum class AttitudeView : uint8_t {
        ATTITUDE,   // Live horizon, pitch/roll labels, ship silhouette
        MINMAX,     // Static min/max lines, numeric labels, ship silhouette
        DEPTH,      // Surface/keel lines, moving sea bottom, depth labels, ship silhouette
        COUNT
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

    // Depth is relayed, never measured here, so it carries no freshness of its own. Both
    // gates are required: the ESP-NOW gateway must be alive (RX timestamp) AND the sounder
    // feed behind it must be fresh (DepthDelta.age_ms). Either stale → "--.-", never a
    // frozen depth under the keel.
    static constexpr uint32_t DEPTH_TIMEOUT_MS      = 6000;  // ESP-NOW gateway broadcast timeout
    static constexpr uint32_t DEPTH_FEED_MAX_AGE_MS = 5000;  // in-payload age of the depth feed

    // ui_PanelBottom is 484×185 and moving it invalidates two full-width bands; the sounder
    // only updates at ~1 Hz, so redrawing faster than this buys nothing.
    static constexpr uint32_t DEPTH_RENDER_INTERVAL_MS = 250;

    // Vessel draft [m] — matches the static ui_LabelDraft text. Also the caution threshold:
    // less than one draft of water under the keel is where the margin runs out.
    static constexpr float DRAFT_M = 1.2f;

    // ui_PanelSurface (y=0) to ui_PanelKeel (y=55) is 55 px and represents DRAFT_M
    static constexpr float PX_PER_METRE = 55.0f / DRAFT_M;  // 45.83 px/m ≈ 2.18 cm/px

    // ui_PanelBottom y-offset when its top edge sits on the keel line (aground, depth below
    // keel = 0). At this offset the 185 px panel exactly fills the screen down to y=479;
    // SquareLine's own y=203 is this value plus one draft, i.e. 1.2 m under the keel.
    static constexpr int16_t PANEL_BOTTOM_Y_AGROUND = 148;
    static constexpr int16_t PANEL_BOTTOM_H         = 185;

    // State
    AttitudeView _active_view = AttitudeView::ATTITUDE;
    bool         _initialized = false;

    // Cached live horizon values (sentinels = not yet set)
    int16_t _last_pitch_x10  = SENTINEL;
    int16_t _last_roll_x10   = SENTINEL;
    int16_t _last_pitch_deg  = SENTINEL;
    int16_t _last_roll_deg   = SENTINEL;
    bool _last_connected  = false;

    // Depth view state
    int16_t  _last_bottom_y        = SENTINEL;  // render cache, skips redundant lv_obj_set_y
    uint32_t _last_depth_render_ms = 0;
    bool     _last_depth_valid     = true;      // falling edge blanks the labels once
    bool     _last_bottom_hidden   = false;
    bool     _last_caution_shown   = false;
    int16_t  _last_below_keel_x10    = SENTINEL;  // label caches, in 0.1 m units
    int16_t  _last_below_surface_x10 = SENTINEL;

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

    // Depth display
    void updateDepth();
    void showDepthWaiting();

};

