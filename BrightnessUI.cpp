#include "BrightnessUI.h"

// === P U B L I C ===

// Constructor
BrightnessUI::BrightnessUI()
    : _state(BrightnessState::IDLE)
    , _initialized(false)
    , _brightnessPercent(DEFAULT_BRIGHTNESS_PERCENT)
    , _pwmChannel(0)
    , _lastRotationTime(0) {}

// Initialize
void BrightnessUI::begin(int pwmChannel) {
    if (_initialized) return;

    _pwmChannel = pwmChannel;

    // Load saved brightness from NVS
    _brightnessPercent = this->loadBrightness();

    // Set HW brightness and update UI
    this->applyBrightness();
    this->updateUI();

    _state = BrightnessState::IDLE;
    _initialized = true;
}

// Handle knob button press
void BrightnessUI::handleButtonPress() {
    if (!_initialized) return;

    switch (_state) {
        case BrightnessState::IDLE:
            this->setState(BrightnessState::ADJUSTING);
            break;

        case BrightnessState::ADJUSTING:
            break;
    }
}

// Handle knob rotation CW and CCW
void BrightnessUI::handleRotation(int8_t direction) {
    if (!_initialized) return;
    if (_state != BrightnessState::ADJUSTING) return;

    // Adjust brightness with one BRIGHTNESS_STEP
    int16_t newPercent = _brightnessPercent + (direction * BRIGHTNESS_STEP);

    // Limit to boundaries
    if (newPercent < MIN_BRIGHTNESS_PERCENT) newPercent = MIN_BRIGHTNESS_PERCENT;
    if (newPercent > MAX_BRIGHTNESS_PERCENT) newPercent = MAX_BRIGHTNESS_PERCENT;

    _brightnessPercent = (int8_t)newPercent;

    // Upodate HW, UI arc element and UI label element real time
    this->applyBrightness();
    this->updateUI();

    // Reset auto-save timer
    _lastRotationTime = millis();
}

// Check the auto-save timer and update state accordingly
void BrightnessUI::updateState() {
    if (!_initialized) return;
    if (_state != BrightnessState::ADJUSTING) return;

    if (millis() - _lastRotationTime >= AUTOSAVE_TIMEOUT_MS) {
        this->saveBrightness(); // Save to NVS
        this->setState(BrightnessState::IDLE); // Back to IDLE
    }
}

// Cancel brightness adjustment
void BrightnessUI::cancelAdjustment() {
    if (_state == BrightnessState::ADJUSTING) {
        this->saveBrightness();
        this->setState(BrightnessState::IDLE);
    }
}

// === P R I V A T E ===

// Manage the state machine
void BrightnessUI::setState(BrightnessState newState) {
    _state = newState;

    switch (newState) {
        case BrightnessState::IDLE:
            lv_obj_add_flag(ui_ContainerAdjustment, LV_OBJ_FLAG_HIDDEN);
            break;

        case BrightnessState::ADJUSTING:
            lv_obj_clear_flag(ui_ContainerAdjustment, LV_OBJ_FLAG_HIDDEN);
            _lastRotationTime = millis();
            break;
    }
}

// Update UI arc and label elements
void BrightnessUI::updateUI() {
    // Update arc to show brightness %
    lv_arc_set_value(ui_ArcAdjustment, _brightnessPercent);

    // Update label text with brightness %
    char buf[8];
    snprintf(buf, sizeof(buf), "%d%%", _brightnessPercent);
    lv_label_set_text(ui_LabelBrightnessValue, buf);
}

// Set HW backlight brightness
void BrightnessUI::applyBrightness() {
    ledcWrite(_pwmChannel, this->percentToPwm(_brightnessPercent));
}

// Translate % to pwm value
uint8_t BrightnessUI::percentToPwm(int8_t percent) {
    // Linear: 0% → 0, 100% → 255
    return (uint8_t)((uint16_t)percent * 255 / 100);
}

// Save brightness % to NVS
void BrightnessUI::saveBrightness() {
    Preferences prefs;
    prefs.begin(NVS_NAMESPACE, false);
    prefs.putChar(NVS_KEY_BRIGHTNESS, _brightnessPercent);
    prefs.end();
}

// Load brightness value from NVS
int8_t BrightnessUI::loadBrightness() {
    Preferences prefs;
    prefs.begin(NVS_NAMESPACE, true);
    int8_t value = prefs.getChar(NVS_KEY_BRIGHTNESS, DEFAULT_BRIGHTNESS_PERCENT);
    prefs.end();

    if (value < MIN_BRIGHTNESS_PERCENT) value = MIN_BRIGHTNESS_PERCENT;
    if (value > MAX_BRIGHTNESS_PERCENT) value = MAX_BRIGHTNESS_PERCENT;

    return value;
}
