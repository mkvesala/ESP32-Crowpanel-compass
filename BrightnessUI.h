#pragma once

#include <Arduino.h>
#include <lvgl.h>
#include <Preferences.h>
#include "ui.h"

// === C L A S S  B R I G H T N E S S U I ===
//
// - Class BrightnessUI - responsible for:
//  - Updating SquareLine generated UI elements on BrightnessScreen
//  - Adjusting display backlight brightness with RotaryEncoder
// - Init: brightnessUI.begin(pwmChannel)
// - Loop: brightnessUI.updateState() - when on BrightnessScreen
// - Provides public API to:
//  - handle knob button press
//  - handle knob rotation to CW and CCW
//  - update state
//  - cancel brightness adjustment
// - Uses: BrightnessState (enum)
// - Owned by: CrowPanelApplication
// - UI logic:
//  - Knob press - ADJUSTING (show ArcAdjustment UI element)
//  - Knob rotation - adjust brightness in 5 % steps, update arc, label and backlight
//  - 3 secs timeout - save brightness value to NVS, hide arc, retur to IDLE

class BrightnessUI {

public:

    BrightnessUI();

    void begin(int pwmChannel);
    void handleButtonPress();
    void handleRotation(int8_t direction);
    void updateState();
    void cancelAdjustment();
    bool isAdjusting() const { return _state == BrightnessState::ADJUSTING; }

private:

    // State machine for BrightnessScreen
    enum class BrightnessState {
        IDLE,       // Display only, rotation switches screens
        ADJUSTING   // Arc visible, rotation adjusts brightness
    };

    // State
    BrightnessState _state;
    bool _initialized;

    // Brightness %
    int8_t _brightnessPercent;

    // Hardware
    int _pwmChannel;

    // Timing
    uint32_t _lastRotationTime;

    // Auto-save timeout
    static constexpr uint32_t AUTOSAVE_TIMEOUT_MS = 3000;

    // Brightness adjustment (% change of one step of rotating the knob)
    static constexpr int8_t BRIGHTNESS_STEP = 2;

    // Boundaries for brightness adjustment
    static constexpr int8_t MIN_BRIGHTNESS_PERCENT = 2;
    static constexpr int8_t MAX_BRIGHTNESS_PERCENT = 100;

    // NVS namespace ja key, default brightness value
    static constexpr const char* NVS_NAMESPACE = "display";
    static constexpr const char* NVS_KEY_BRIGHTNESS = "brightness";
    static constexpr int8_t DEFAULT_BRIGHTNESS_PERCENT = 78;  // 200/255 pwm value

    // Helpers
    void setState(BrightnessState newState);
    void updateUI();
    void applyBrightness();
    void saveBrightness();
    int8_t loadBrightness();

    // % to pwm value
    static uint8_t percentToPwm(int8_t percent);
};
