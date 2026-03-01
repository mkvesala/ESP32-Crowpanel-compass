#include "ScreenManager.h"

// === P U B L I C ===

// Register a screen — must be called before begin()
void ScreenManager::addScreen(IScreenUI* screen) {
    if (_screen_count >= MAX_SCREENS) return;
    _screens[_screen_count++] = screen;
}

// Initialize: load first screen, mark ready
void ScreenManager::begin() {
    if (_initialized || _screen_count == 0) return;
    _initialized = true;
    lv_scr_load(_screens[0]->getLvglScreen());
    _screens[0]->onEnter();
}

// CW carousel step
void ScreenManager::switchNext() {
    if (_screen_count <= 1) return;
    this->switchTo(this->nextIdx(), Direction::CW);
}

// CCW carousel step
void ScreenManager::switchPrevious() {
    if (_screen_count <= 1) return;
    this->switchTo(this->previousIdx(), Direction::CCW);
}

// Return pointer to the active screen
IScreenUI* ScreenManager::getCurrentScreen() const {
    if (_screen_count == 0) return nullptr;
    return _screens[_current];
}

// === P R I V A T E ===

// Unified screen switch: cleanup current, animate to target, notify new
void ScreenManager::switchTo(uint8_t index, Direction dir) {
    if (!_initialized) return;
    if (index == _current) return;

    this->onLeavingCurrentScreen();
    _current = index;

    auto anim = (dir == Direction::CW)
        ? LV_SCR_LOAD_ANIM_MOVE_LEFT
        : LV_SCR_LOAD_ANIM_MOVE_RIGHT;

    lv_scr_load_anim(_screens[index]->getLvglScreen(), anim, ANIM_DURATION_MS, 0, false);
    _screens[index]->onEnter();
}

// Notify current screen it is being left
void ScreenManager::onLeavingCurrentScreen() {
    if (_screen_count == 0) return;
    _screens[_current]->onLeave();
}
