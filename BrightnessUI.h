#pragma once

#include <Arduino.h>
#include <lvgl.h>
#include <Preferences.h>
#include "IScreenUI.h"

// === C L A S S  B R I G H T N E S S U I ===
//
// - Class BrightnessUI - responsible for:
//  - Updating SquareLine generated UI elements on BrightnessScreen
//  - Adjusting display backlight brightness with RotaryEncoder
// - Realizes: IScreenUI
// - Init: _brightnessUI.begin()
// - Update in loop(): via ScreenManager → IScreenUI::update()
// - Provides public API (via IScreenUI) to:
//  - Handle knob button press via onButtonPress()
//  - Handle knob rotation via onRotation()
//  - Intercept rotation when adjusting via interceptsRotation()
//  - Cancel brightness adjustment on screen leave via onLeave()
// - UI logic:
//  - Knob press - ADJUSTING (show ArcAdjustment UI element)
//  - Knob rotation - adjust brightness in BRIGHTNESS_STEP steps, update arc, label and backlight
//  - 3 secs timeout - save brightness value to NVS, hide arc, return to IDLE
// - Owned by: CrowPanelApplication

class BrightnessUI : public IScreenUI {

public:

    explicit BrightnessUI(int pwm_channel);

    void begin() override;
    lv_obj_t* getLvglScreen() const override;
    void update() override;
    void onButtonPress() override;
    void onRotation(int8_t dir) override; 
    bool interceptsRotation() const override;
    void onLeave() override;

private:

    // State machine for BrightnessScreen
    enum class BrightnessState {
        IDLE,       // Display only, rotation switches screens
        ADJUSTING   // Arc visible, rotation adjusts brightness
    };

    // State
    BrightnessState _state = BrightnessState::IDLE;
    bool _initialized = false;

    // Brightness %
    int8_t _brightness_percent = DEFAULT_BRIGHTNESS_PERCENT;

    // Hardware
    int _pwm_channel = 0;

    // Timing
    uint32_t _last_rotation_time = 0;

    // Internal helpers (all private)
    bool isAdjusting() const { return _state == BrightnessState::ADJUSTING; }
    void handleButtonPress();
    void handleRotation(int8_t direction);
    void updateState();
    void cancelAdjustment();
    void setState(BrightnessState new_state);
    void updateUI();
    void applyBrightness();
    void saveBrightness();
    int8_t loadBrightness();
    uint8_t percentToPwm(int8_t percent);

    // Auto-save timeout
    static constexpr uint32_t AUTOSAVE_TIMEOUT_MS = 3000;

    // Brightness adjustment (% change per knob step)
    static constexpr int8_t BRIGHTNESS_STEP = 2;

    // Boundaries for brightness adjustment
    static constexpr int8_t MIN_BRIGHTNESS_PERCENT = 2;
    static constexpr int8_t MAX_BRIGHTNESS_PERCENT = 100;

    // NVS namespace and key, default brightness value
    static constexpr const char* NVS_NAMESPACE = "display";
    static constexpr const char* NVS_KEY_BRIGHTNESS = "brightness";
    static constexpr int8_t DEFAULT_BRIGHTNESS_PERCENT = 48;  // 122/255 pwm value
};
