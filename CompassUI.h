#pragma once

#include <Arduino.h>
#include <lvgl.h>
#include "IScreenUI.h"
#include "ESPNowReceiver.h"

// === C L A S S  C O M P A S S U I ===
//
// - Class CompassUI - responsible for managing SquareLine generated UI elements on CompassScreen
// - Realizes: IScreenUI
// - Fetches data from ESPNowReceiver in update())
// - Initialize: compassUI.begin()
// - Update in loop(): via ScreenManager → IScreenUI::update()
// - Provides public API to:
//   - Toggle heading mode (TRUE/MAGNETIC) via onButtonPress()
// - Owned by: CrowPanelApplication

class CompassUI : public IScreenUI {

public:

    explicit CompassUI(ESPNowReceiver &receiver);

    void begin() override;
    lv_obj_t* getLvglScreen() const override;
    void update() override; 
    void onButtonPress() override;  

private:

    ESPNowReceiver &_receiver;

    // Connection timeout before showing disconnected state
    static constexpr uint32_t CONNECTION_TIMEOUT_MS = 3000;

    // Methods to update SquareLine UI elements
    void showWaiting();
    void showDisconnected();
    void toggleHeadingMode();
    void setCompassRotation(uint16_t heading_x10);
    void updateHeadingLabel(uint16_t heading_deg);
    void updateHeadingMode(bool is_true);
    void setConnectionIndicator(bool connected);

    // Compass rose rotation threshold (0.5° = 5 in x10 units)
    static constexpr uint16_t ROTATION_THRESHOLD_X10 = 5;

    // Colors for connection panel, "the red dot"
    static constexpr uint32_t COLOR_CONNECTED = 0x000000;  // Black
    static constexpr uint32_t COLOR_DISCONNECTED = 0xFF0000;  // Red

    // Cached values
    uint16_t _last_heading_x10;
    uint16_t _last_heading_deg;
    bool _last_is_true = false;
    bool _last_connected = false;

    // true = True heading, false = Magnetic heading
    bool _use_true_heading;

    bool _initialized = false;

};
