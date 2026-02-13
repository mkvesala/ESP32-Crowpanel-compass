#include "ScreenManager.h"
#include "AttitudeUI.h"
#include "BrightnessUI.h"
#include "ui.h"

ScreenManager::ScreenManager()
    : _current(Screen::COMPASS)
    , _initialized(false)
    , _attitudeUI(nullptr)
    , _brightnessUI(nullptr)
{
}

void ScreenManager::begin(AttitudeUI* attitudeUI, BrightnessUI* brightnessUI) {
    if (_initialized) return;

    _attitudeUI = attitudeUI;
    _brightnessUI = brightnessUI;

    // Aloitusnäyttö on CompassScreen (ladataan ui_init():ssa)
    _current = Screen::COMPASS;
    _initialized = true;
}

void ScreenManager::switchNext() {
    // CW: COMPASS → ATTITUDE → BRIGHTNESS → COMPASS → ...
    switch (_current) {
        case Screen::COMPASS:
            switchTo(Screen::ATTITUDE);
            break;
        case Screen::ATTITUDE:
            switchTo(Screen::BRIGHTNESS);
            break;
        case Screen::BRIGHTNESS:
            switchTo(Screen::COMPASS);
            break;
    }
}

void ScreenManager::switchPrevious() {
    // CCW: COMPASS → BRIGHTNESS → ATTITUDE → COMPASS → ...
    if (!_initialized) return;

    lv_obj_t* target = nullptr;

    switch (_current) {
        case Screen::COMPASS:
            onLeavingCurrentScreen();
            _current = Screen::BRIGHTNESS;
            target = ui_BrightnessScreen;
            break;
        case Screen::ATTITUDE:
            onLeavingCurrentScreen();
            _current = Screen::COMPASS;
            target = ui_CompassScreen;
            break;
        case Screen::BRIGHTNESS:
            onLeavingCurrentScreen();
            _current = Screen::ATTITUDE;
            target = ui_AttitudeScreen;
            break;
    }

    if (target) {
        lv_scr_load_anim(target, LV_SCR_LOAD_ANIM_MOVE_RIGHT, ANIM_DURATION_MS, 0, false);
    }
}

void ScreenManager::switchTo(Screen screen) {
    if (!_initialized) return;
    if (screen == _current) return;

    // Siivoa edellinen näyttö
    onLeavingCurrentScreen();

    _current = screen;

    lv_obj_t* target = nullptr;
    switch (screen) {
        case Screen::COMPASS:
            target = ui_CompassScreen;
            break;
        case Screen::ATTITUDE:
            target = ui_AttitudeScreen;
            break;
        case Screen::BRIGHTNESS:
            target = ui_BrightnessScreen;
            break;
    }

    // Animoi vasemmalle (CW-suunta)
    lv_scr_load_anim(target, LV_SCR_LOAD_ANIM_MOVE_LEFT, ANIM_DURATION_MS, 0, false);
}

void ScreenManager::onLeavingCurrentScreen() {
    if (_current == Screen::ATTITUDE && _attitudeUI) {
        _attitudeUI->cancelLevelOperation();
    }
    if (_current == Screen::BRIGHTNESS && _brightnessUI) {
        _brightnessUI->cancelAdjustment();
    }
}
