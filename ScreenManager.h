#pragma once

#include <Arduino.h>
#include <lvgl.h>
#include "IScreenUI.h"

// ===  C L A S S  S C R E E N M A N A G E R ===
//
// - Class ScreenManager - responsible for switching between screens
// - Screens are registered via addScreen() before begin()
// - Init: call addScreen() for each screen, then screenMgr.begin()
// - Uses: IScreenUI*
// - Provides public API to switch smoothly (LVGL animation) CW or CCW
// - Carousel order matches registration order (modulo index arithmetic)
// - Owned by: CrowPanelApplication

class ScreenManager {

public:

    static constexpr uint8_t MAX_SCREENS = 8;

    explicit ScreenManager() = default;

    void addScreen(IScreenUI* screen); 
    void begin();                          
    void switchNext();                    
    void switchPrevious();                  
    IScreenUI* getCurrentScreen() const;

private:

    enum class Direction { CW, CCW };

    void switchTo(uint8_t index, Direction dir);
    void onLeavingCurrentScreen();

    uint8_t nextIdx() const { return (_current + 1) % _screen_count; }
    uint8_t previousIdx() const { return (_current + _screen_count - 1) % _screen_count; }

    IScreenUI* _screens[MAX_SCREENS] = {};
    uint8_t _screen_count = 0;
    uint8_t _current = 0;
    bool _initialized = false;

    static constexpr uint32_t ANIM_DURATION_MS = 300;

};
