#pragma once

#include <Arduino.h>
#include <lvgl.h>
#include <Preferences.h>
#include "IScreenUI.h"
#include "ESPNowReceiver.h"

// === C L A S S  C O M P A S S U I ===
//
// - Class CompassUI - responsible for managing SquareLine generated UI elements on CompassScreen
// - Realizes: IScreenUI
// - Fetches heading data (HEADING_DELTA) and GNSS data (GNSS_DELTA) from ESPNowReceiver in update()
// - View cycle: HEADING (HDG T) → COG (COG T) → SOG → HEADING ...
// - Knob button press: cycles CompassView
// - Active view: persisted to NVS on onLeave()
// - HEADING: compass rose rotates to HDG(T), connection = CMPS14, last known values preserved on disconnect
// - COG: compass rose rotates to COG(T), connection = GNSS sender, shows "---°" without fix
// - SOG: speedometer arc, connection = GNSS sender, shows "--.-" without fix
// - Connection indicator (PanelConnected): tracks active view's data source
// - Owned by: CrowPanelApplication

class CompassUI : public IScreenUI {

public:

    explicit CompassUI(ESPNowReceiver &receiver);

    void begin() override;
    lv_obj_t* getLvglScreen() const override;
    void update() override;
    void onButtonPress() override;
    void onLeave() override;

private:

    enum class CompassView : uint8_t {
        HEADING = 0,
        COG     = 1,
        SOG     = 2,
        COUNT   = 3
    };

    ESPNowReceiver &_receiver;
    bool _initialized = false;

    CompassView _active_view = CompassView::HEADING;

    // Compass rose: single rotation cache for what's currently rendered (reset on view switch)
    uint16_t _last_rose_x10 = 0xFFFF;

    // Heading/COG label cache (full degrees, reset on view switch)
    uint16_t _last_label_deg = 0xFFFF;

    // GNSS: connection tracking and latest fix status
    uint32_t _last_gnss_millis = 0;
    bool _last_gnss_fix = false;

    // SOG arc cache (reset on view switch)
    uint16_t _last_sog_x10 = 0xFFFF;

    // SOG EMA state (knots, float; NAN = not yet initialized)
    float _sog_ema     = NAN;
    float _sog_ema_ref = NAN;

    // Connection indicator state cache
    bool _last_connected = false;

    // View management
    void showView(CompassView view);

    // UI update helpers
    void setCompassRotation(uint16_t heading_x10);
    void updateHeadingLabel(uint16_t deg);
    void updateSogDisplay(uint16_t sog_knots_x10, bool fix_ok);
    void setConnectionIndicator(bool connected);
    void showWaiting();

    // NVS
    void saveView();
    CompassView loadView();

    // Rotation deadband threshold (0.5° = 5 in x10 units)
    static constexpr uint16_t ROTATION_THRESHOLD_X10 = 5;

    // SOG arc maximum (10 knots × 10)
    static constexpr uint16_t SOG_ARC_MAX = 100;

    // SOG EMA
    static constexpr float SOG_EMA_ALPHA       = 0.15f;
    static constexpr float SOG_TREND_THRESHOLD = 0.1f;  // knots

    static constexpr uint32_t HEADING_TIMEOUT_MS = 5000;
    static constexpr uint32_t GNSS_TIMEOUT_MS    = 5000;

    static constexpr uint32_t COLOR_CONNECTED    = 0x000000;
    static constexpr uint32_t COLOR_DISCONNECTED = 0xFF0000;

    static constexpr const char* NVS_NAMESPACE = "compass";
    static constexpr const char* NVS_KEY_VIEW  = "view";

};
