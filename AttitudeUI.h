#pragma once

#include <Arduino.h>
#include <lvgl.h>
#include "IScreenUI.h"
#include "ESPNowReceiver.h"

// === C L A S S  A T T I T U D E U I ===
//
// - Class AttitudeUI - responsible for managing SquareLine generated UI elements on AttitudeScreen
// - Realizes: IScreenUI
// - Fetches data from ESPNowReceiver in update())
// - Initialize: _attitudeUI.begin()
// - Update in loop(): via ScreenManager → IScreenUI::update()
// - Provides public API to:
//   - Handle knob button press (activates attitude leveling) via onButtonPress()
//   - Cancel attitude leveling on screen leave via onLeave()
// - UI logic:
//   - Pitch: bow down, pitch down - artificial horizon up and vice versa
//   - Roll: roll port side, roll down - artificial horizon tilted starboard and vice versa
//   - Knob press: CONFIRM_WAIT (show dialog)
//   - Knob press again: SENDING (send command to compass)
//   - Response or timeout: SUCCESS/FAILED → IDLE
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

    // State machine for AttitudeScreen
    enum class LevelState {
        IDLE,           // Normal operation, dialog hidden
        CONFIRM_WAIT,   // "Level? Press knob to confirm." visible, waiting for 2nd press
        SENDING,        // "Leveling..." visible, waiting for response
        SUCCESS,        // "Success!" visible briefly
        FAILED          // "Failed!" visible briefly
    };

    ESPNowReceiver &_receiver;

    // Connection timeout before showing waiting state
    static constexpr uint32_t CONNECTION_TIMEOUT_MS = 3000;

    // Methods for updating the SquareLine generated UI elements
    void showWaiting();
    void updatePitchLabel(int16_t pitch_deg);
    void updateRollLabel(int16_t roll_deg);
    void updateHorizon(int16_t pitch_x10, int16_t roll_x10);

    // Scaling, pixels per degree (pitch)
    static constexpr int16_t PITCH_SCALE = 3;

    // Cached values
    int16_t _last_pitch_x10;
    int16_t _last_roll_x10;
    int16_t _last_pitch_deg;
    int16_t _last_roll_deg;
    bool _last_connected = false;

    bool _initialized = false;

    // Level state machine
    LevelState _level_state = LevelState::IDLE;
    uint32_t _state_start_time = 0;

    // Level state machine - internal methods (all private)
    void cancelLevelOperation();
    void updateLevelState();
    void setLevelState(LevelState new_state);
    void updateLevelDialog();

    // Timeouts (ms)
    static constexpr uint32_t CONFIRM_TIMEOUT_MS = 3000;
    static constexpr uint32_t SENDING_TIMEOUT_MS = 3000;
    static constexpr uint32_t SUCCESS_DISPLAY_MS = 2000;
    static constexpr uint32_t FAILED_DISPLAY_MS  = 2000;

};
