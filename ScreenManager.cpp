#include "ScreenManager.h"
#include "AttitudeUI.h"
#include "ui.h"

ScreenManager::ScreenManager()
    : _current(Screen::COMPASS)
    , _initialized(false)
    , _attitudeUI(nullptr)
{
}

void ScreenManager::begin(AttitudeUI* attitudeUI) {
    if (_initialized) return;

    _attitudeUI = attitudeUI;

    // Aloitusnäyttö on CompassScreen (ladataan ui_init():ssa)
    _current = Screen::COMPASS;
    _initialized = true;
}

void ScreenManager::switchNext() {
    // Kahden näytön järjestelmässä: COMPASS → ATTITUDE → COMPASS → ...
    if (_current == Screen::COMPASS) {
        switchTo(Screen::ATTITUDE);
    } else {
        switchTo(Screen::COMPASS);
    }
}

void ScreenManager::switchPrevious() {
    // Kahden näytön järjestelmässä: sama kuin switchNext mutta eri animaatiosuunta
    if (_current == Screen::COMPASS) {
        // Vaihda ATTITUDE:en, animaatio oikealta
        if (!_initialized) return;
        _current = Screen::ATTITUDE;
        lv_scr_load_anim(ui_AttitudeScreen, LV_SCR_LOAD_ANIM_MOVE_RIGHT, ANIM_DURATION_MS, 0, false);
    } else {
        // Vaihda COMPASS:iin, animaatio vasemmalta
        if (!_initialized) return;
        onLeavingAttitudeScreen();
        _current = Screen::COMPASS;
        lv_scr_load_anim(ui_CompassScreen, LV_SCR_LOAD_ANIM_MOVE_LEFT, ANIM_DURATION_MS, 0, false);
    }
}

void ScreenManager::switchTo(Screen screen) {
    if (!_initialized) return;
    if (screen == _current) return;

    // Cancel level operation if leaving AttitudeScreen
    if (_current == Screen::ATTITUDE) {
        onLeavingAttitudeScreen();
    }

    _current = screen;

    if (screen == Screen::ATTITUDE) {
        // Siirry Attitude-näyttöön, animaatio vasemmalta (näyttö tulee vasemmalta)
        lv_scr_load_anim(ui_AttitudeScreen, LV_SCR_LOAD_ANIM_MOVE_LEFT, ANIM_DURATION_MS, 0, false);
    } else {
        // Siirry Compass-näyttöön, animaatio oikealta
        lv_scr_load_anim(ui_CompassScreen, LV_SCR_LOAD_ANIM_MOVE_RIGHT, ANIM_DURATION_MS, 0, false);
    }
}

void ScreenManager::onLeavingAttitudeScreen() {
    if (_attitudeUI) {
        _attitudeUI->cancelLevelOperation();
    }
}
