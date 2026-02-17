#include "ScreenManager.h"

// === P U B L I C ===

// Constructor
ScreenManager::ScreenManager(CompassUI &compassUI, AttitudeUI &attitudeUI, BrightnessUI &brightnessUI)
    : _compassUI(compassUI)
    , _attitudeUI(attitudeUI)
    , _brightnessUI(brightnessUI)
    , _current(Screen::COMPASS)
    , _initialized(false) {}

// Initialize (placeholder for heavier initialization)
void ScreenManager::begin() {
    if (_initialized) return;
    _initialized = true;
}

// Clockwise switch of the carousel
void ScreenManager::switchNext() {
    this->switchTo(this->nextScreen(), Direction::CW);
}

// Counter-clockwise swictch of the carousel
void ScreenManager::switchPrevious() {
    this->switchTo(this->previousScreen(), Direction::CCW);
}

// === P R I V A T E ===

// CW carousel order: COMPASS → ATTITUDE → BRIGHTNESS → COMPASS → ...
ScreenManager::Screen ScreenManager::nextScreen() const {
    switch (_current) {
        case Screen::COMPASS:    return Screen::ATTITUDE;
        case Screen::ATTITUDE:   return Screen::BRIGHTNESS;
        case Screen::BRIGHTNESS: return Screen::COMPASS;
    }
    return _current;
}

// CCW carousel order: COMPASS → BRIGHTNESS → ATTITUDE → COMPASS → ...
ScreenManager::Screen ScreenManager::previousScreen() const {
    switch (_current) {
        case Screen::COMPASS:    return Screen::BRIGHTNESS;
        case Screen::ATTITUDE:   return Screen::COMPASS;
        case Screen::BRIGHTNESS: return Screen::ATTITUDE;
    }
    return _current;
}

// Unified screen switch: cleanup, state update, target selection, animation
void ScreenManager::switchTo(Screen screen, Direction dir) {
    if (!_initialized) return;
    if (screen == _current) return;

    this->onLeavingCurrentScreen();
    _current = screen;

    lv_obj_t* target = nullptr;
    switch (screen) {
        case Screen::COMPASS:    target = ui_CompassScreen;    break;
        case Screen::ATTITUDE:   target = ui_AttitudeScreen;   break;
        case Screen::BRIGHTNESS: target = ui_BrightnessScreen; break;
    }

    auto anim = (dir == Direction::CW)
        ? LV_SCR_LOAD_ANIM_MOVE_LEFT
        : LV_SCR_LOAD_ANIM_MOVE_RIGHT;

    lv_scr_load_anim(target, anim, ANIM_DURATION_MS, 0, false);
}

// Cleanup if required for a screen
void ScreenManager::onLeavingCurrentScreen() {
    if (_current == Screen::ATTITUDE) _attitudeUI.cancelLevelOperation();
    if (_current == Screen::BRIGHTNESS) _brightnessUI.cancelAdjustment();
}
