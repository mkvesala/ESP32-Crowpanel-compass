#include "CompassUI.h"
#include "ui.h"

// Värit connection-indikaattorille
#define COLOR_CONNECTED     lv_color_hex(0x000000)  // Musta (piilossa)
#define COLOR_DISCONNECTED  lv_color_hex(0xFF0000)  // Punainen

CompassUI::CompassUI()
    : _last_heading_x10(0xFFFF)
    , _last_heading_deg(0xFFFF)
    , _last_is_true(false)
    , _last_connected(false)
    , _use_true_heading(true)  // Oletuksena True heading
    , _initialized(false)
{
}

void CompassUI::begin() {
    if (_initialized) return;

    _initialized = true;

    // Näytä aluksi "waiting" tila
    showWaiting();
}

void CompassUI::update(const HeadingData& data, bool connected, float packetsPerSec) {
    if (!_initialized) return;

    // Käytä käyttäjän valitsemaa moodia (toggle painikkeella)
    // Kompassi lähettää aina validit arvot molemmille
    bool is_true = _use_true_heading;

    // Hae oikea heading-arvo
    uint16_t heading_x10 = is_true ? data.heading_true_x10 : data.heading_mag_x10;
    uint16_t heading_deg = is_true ? data.getHeadingTrueDeg() : data.getHeadingMagDeg();

    // Päivitä UI-elementit
    setCompassRotation(heading_x10);
    updateHeadingLabel(heading_deg);
    updateHeadingMode(is_true);
    setConnectionIndicator(connected);
}

void CompassUI::toggleHeadingMode() {
    _use_true_heading = !_use_true_heading;

    // Pakota UI päivitys seuraavalla update()-kutsulla
    _last_heading_x10 = 0xFFFF;
    _last_heading_deg = 0xFFFF;
    _last_is_true = !_use_true_heading;  // Pakota päivitys
}

void CompassUI::setCompassRotation(uint16_t heading_x10) {
    // Threshold to avoid heavy LVGL re-render on small changes
    // _last_heading_x10 == 0xFFFF is sentinel for first render (always draw)
    // Handles 360°→0° wraparound: e.g. 3599→1 = diff 2, not 3598
    if (_last_heading_x10 != 0xFFFF) {
        int16_t diff = (int16_t)heading_x10 - (int16_t)_last_heading_x10;
        if (diff > 1800) diff -= 3600;
        if (diff < -1800) diff += 3600;
        if (abs(diff) < ROTATION_THRESHOLD_X10) return;
    }

    _last_heading_x10 = heading_x10;

    // LVGL: 0.1° yksikköä, negatiivinen = myötäpäivään
    // Kun heading=90° (itä), ruusu kääntyy -90° jotta nuoli osoittaa itään
    int16_t angle = -(int16_t)heading_x10;
    lv_img_set_angle(ui_ImageCompassRose, angle);
}

void CompassUI::updateHeadingLabel(uint16_t heading_deg) {
    if (heading_deg == _last_heading_deg) return;
    _last_heading_deg = heading_deg;

    char buf[16];
    // Käytä UTF-8 astetta (° = 0xC2 0xB0) - fonttiin pitää olla lisätty U+00B0
    snprintf(buf, sizeof(buf), "%03d°", heading_deg);
    lv_label_set_text(ui_LabelHeading, buf);
}

void CompassUI::updateHeadingMode(bool is_true) {
    if (is_true == _last_is_true) return;
    _last_is_true = is_true;

    lv_label_set_text(ui_LabelHeadingMode, is_true ? "T" : "M");
}

void CompassUI::setConnectionIndicator(bool connected) {
    if (connected == _last_connected) return;
    _last_connected = connected;

    lv_color_t color = connected ? COLOR_CONNECTED : COLOR_DISCONNECTED;
    lv_obj_set_style_bg_color(ui_PanelConnected, color, LV_PART_MAIN | LV_STATE_DEFAULT);
}

void CompassUI::showWaiting() {
    if (!_initialized) return;

    lv_label_set_text(ui_LabelHeading, "---");
    lv_label_set_text(ui_LabelHeadingMode, "-");
    lv_obj_set_style_bg_color(ui_PanelConnected, COLOR_DISCONNECTED, LV_PART_MAIN | LV_STATE_DEFAULT);

    // Reset cached values
    _last_heading_x10 = 0xFFFF;
    _last_heading_deg = 0xFFFF;
    _last_is_true = false;
    _last_connected = false;
}

void CompassUI::showDisconnected() {
    if (!_initialized) return;

    lv_obj_set_style_bg_color(ui_PanelConnected, COLOR_DISCONNECTED, LV_PART_MAIN | LV_STATE_DEFAULT);
    _last_connected = false;
}
