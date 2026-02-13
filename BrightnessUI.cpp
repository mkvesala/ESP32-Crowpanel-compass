#include "BrightnessUI.h"
#include "ui.h"
#include <Preferences.h>

BrightnessUI::BrightnessUI()
    : _state(BrightnessState::IDLE)
    , _initialized(false)
    , _brightnessPercent(DEFAULT_BRIGHTNESS_PERCENT)
    , _pwmChannel(0)
    , _lastRotationTime(0)
{
}

void BrightnessUI::begin(int pwmChannel) {
    if (_initialized) return;

    _pwmChannel = pwmChannel;

    // Lataa tallennettu kirkkaus NVS:stä
    _brightnessPercent = loadBrightness();

    // Aseta kirkkaus hardwarelle ja päivitä UI
    applyBrightness();
    updateUI();

    _state = BrightnessState::IDLE;
    _initialized = true;
}

void BrightnessUI::handleButtonPress() {
    if (!_initialized) return;

    switch (_state) {
        case BrightnessState::IDLE:
            setState(BrightnessState::ADJUSTING);
            break;

        case BrightnessState::ADJUSTING:
            // Ei toimintoa säätötilassa — pyöritys hoitaa kaiken
            break;
    }
}

void BrightnessUI::handleRotation(int8_t direction) {
    if (!_initialized) return;
    if (_state != BrightnessState::ADJUSTING) return;

    // Säädä kirkkautta askeleella
    int16_t newPercent = _brightnessPercent + (direction * BRIGHTNESS_STEP);

    // Rajoita sallittuun alueeseen
    if (newPercent < MIN_BRIGHTNESS_PERCENT) newPercent = MIN_BRIGHTNESS_PERCENT;
    if (newPercent > MAX_BRIGHTNESS_PERCENT) newPercent = MAX_BRIGHTNESS_PERCENT;

    _brightnessPercent = (int8_t)newPercent;

    // Päivitä hardware, arc ja label reaaliajassa
    applyBrightness();
    updateUI();

    // Nollaa auto-save -ajastin
    _lastRotationTime = millis();
}

void BrightnessUI::updateState() {
    if (!_initialized) return;
    if (_state != BrightnessState::ADJUSTING) return;

    // Tarkista auto-save timeout
    if (millis() - _lastRotationTime >= AUTOSAVE_TIMEOUT_MS) {
        saveBrightness();
        setState(BrightnessState::IDLE);
    }
}

void BrightnessUI::cancelAdjustment() {
    if (_state == BrightnessState::ADJUSTING) {
        saveBrightness();
        setState(BrightnessState::IDLE);
    }
}

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

void BrightnessUI::updateUI() {
    // Päivitä arc-arvo (range 0–100, vastaa suoraan prosenttia)
    lv_arc_set_value(ui_ArcAdjustment, _brightnessPercent);

    // Päivitä label-teksti
    char buf[8];
    snprintf(buf, sizeof(buf), "%d%%", _brightnessPercent);
    lv_label_set_text(ui_LabelBrightnessValue, buf);
}

void BrightnessUI::applyBrightness() {
    ledcWrite(_pwmChannel, percentToPwm(_brightnessPercent));
}

uint8_t BrightnessUI::percentToPwm(int8_t percent) {
    // Lineaarinen muunnos: 0% → 0, 100% → 255
    return (uint8_t)((uint16_t)percent * 255 / 100);
}

void BrightnessUI::saveBrightness() {
    Preferences prefs;
    prefs.begin(NVS_NAMESPACE, false);  // false = read-write
    prefs.putChar(NVS_KEY_BRIGHTNESS, _brightnessPercent);
    prefs.end();
}

int8_t BrightnessUI::loadBrightness() {
    Preferences prefs;
    prefs.begin(NVS_NAMESPACE, true);  // true = read-only
    int8_t value = prefs.getChar(NVS_KEY_BRIGHTNESS, DEFAULT_BRIGHTNESS_PERCENT);
    prefs.end();

    // Validoi ladattu arvo
    if (value < MIN_BRIGHTNESS_PERCENT) value = MIN_BRIGHTNESS_PERCENT;
    if (value > MAX_BRIGHTNESS_PERCENT) value = MAX_BRIGHTNESS_PERCENT;

    return value;
}
