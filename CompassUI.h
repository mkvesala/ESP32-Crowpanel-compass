#pragma once

#include <Arduino.h>
#include <lvgl.h>
#include "HeadingData.h"

// === C L A S S  C O M P A S S U I ===
//
// - Class CompassUI - responsible for managing SquareLine generated UI elements on CompassScreen
// - Initialize: compassUI.begin()
// - Update in loop(): compassUI.update(..)
// - Provides public API to:
//   - Show "waiting for data" status
//   - Show "disconnected" status
//   - Toggle heading mode (TRUE/MAGNETIC)
//   - Return current heading mode (TRUE/MAGNETIC)
// - Owned by: CrowPanelApplication

class CompassUI {

public:

    CompassUI();

    void begin();
    void update(const HeadingData& data, bool connected, float packetsPerSec);
    void showWaiting();
    void showDisconnected();
    void toggleHeadingMode();
    bool isShowingTrueHeading() const { return _use_true_heading; }

private:

    // Methods to update SquareLine UI elements
    void setCompassRotation(uint16_t heading_x10);
    void updateHeadingLabel(uint16_t heading_deg);
    void updateHeadingMode(bool is_true);
    void setConnectionIndicator(bool connected);

    // Compass rose rotation threshold (0.5° = 5 in x10 units)
    // Reduces heavy LVGL re-renders when heading changes are small
    static constexpr uint16_t ROTATION_THRESHOLD_X10 = 5;
    
    // Colors for "connected" panel on UI, "the red dot"
    static constexpr uint32_t COLOR_CONNECTED    = 0x000000;  // Black
    static constexpr uint32_t COLOR_DISCONNECTED = 0xFF0000;  // Red

    // Cached values
    uint16_t _last_heading_x10;
    uint16_t _last_heading_deg;
    bool     _last_is_true;
    bool     _last_connected;

    // true = True heading, false = Magnetic heading
    bool _use_true_heading;

    bool _initialized;
    
};
