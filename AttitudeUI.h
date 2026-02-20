#pragma once

#include <Arduino.h>
#include <lvgl.h>
#include "ui.h"
#include "ESPNowReceiver.h"
#include "espnow_protocol.h"

// === C L A S S  A T T I T U D E U I ===
//
// - Class AttitudeUI - responsible for managing SquareLine generated UI elements on AttitudeScreen
// - Initialize: attitudeUI.begin()
// - Update in loop():
//   - attitudeUI.update(..)
//   - attitudeUI.updateLevelState(..)
// - Provides public API to:
//   - Handle knob button press (activates attitude leveling)
//   - Update attitude leveling state
//   - Cancel attitude leveling
//   - Get current attitude leveling state
// - UI logic:
//   - Pitch: bow down, pitch down - artificial horizon up and vice versa
//   - Roll: roll port side, roll down - artificial horizon tilted starboard and vice versa
//   - Knob press: CONFIRM_WAIT (show dialog)
//   - Knob press again: SENDING (send command to compass)
//   - Response or timeout: SUCCESS/FAILED - IDLE
// - Uses: ESPNowReceiver, LevelState
// - Owned by: CrowPanelApplication

class AttitudeUI {

public:
    
    AttitudeUI(ESPNowReceiver &receiver);

    void begin();
    void update(const HeadingData& data, bool connected, float packetsPerSec);
    void showDisconnected();
    bool handleButtonPress();
    void updateLevelState();
    void cancelLevelOperation();

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

    bool _initialized;

    // Level state machine
    LevelState _levelState;
    uint32_t _stateStartTime;

    // Level dialog update
    void setLevelState(LevelState newState);
    void updateLevelDialog();

    // Timeouts (ms)
    static constexpr uint32_t CONFIRM_TIMEOUT_MS = 3000;
    static constexpr uint32_t SENDING_TIMEOUT_MS = 3000;
    static constexpr uint32_t SUCCESS_DISPLAY_MS = 1500;
    static constexpr uint32_t FAILED_DISPLAY_MS = 2000;

};
