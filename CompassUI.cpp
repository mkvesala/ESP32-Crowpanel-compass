#include "CompassUI.h"
#include "ui.h"

// === P U B L I C ===

// Constructor
CompassUI::CompassUI()
    : _last_heading_x10(0xFFFF)
    , _last_heading_deg(0xFFFF)
    , _use_true_heading(true) {}

// Initialize
void CompassUI::begin() {
    if (_initialized) return;
    // Rotating compass rose is expensive, remove antialiasing to make it cheaper
    lv_img_set_antialias(ui_ImageCompassRose, false);
    _initialized = true;
    this->showWaiting();
}

// Update UI with latest compass data
void CompassUI::update(const HeadingData& data, bool connected) {
    if (!_initialized) return;

    bool is_true = _use_true_heading;

    uint16_t heading_x10 = is_true ? data.heading_true_x10 : data.heading_mag_x10;
    uint16_t heading_deg = is_true ? data.getHeadingTrueDeg() : data.getHeadingMagDeg();

    // Update UI elements
    this->setCompassRotation(heading_x10);
    this->updateHeadingLabel(heading_deg);
    this->updateHeadingMode(is_true);
    this->setConnectionIndicator(connected);
}

// Toggle heading mode TRUE/MAGNETIC
void CompassUI::toggleHeadingMode() {
    _use_true_heading = !_use_true_heading;

    // Force UI update on next update() call
    _last_heading_x10 = 0xFFFF;
    _last_heading_deg = 0xFFFF;
    _last_is_true = !_use_true_heading;
}

// Show "disconnected" status on UI
void CompassUI::showDisconnected() {
    if (!_initialized) return;

    // "The red dot"
    lv_obj_set_style_bg_color(ui_PanelConnected, lv_color_hex(COLOR_DISCONNECTED), LV_PART_MAIN | LV_STATE_DEFAULT);
    _last_connected = false;
}

// === P R I V A T E ===

// Show "waiting for data" status on UI
void CompassUI::showWaiting() {
    if (!_initialized) return;

    // Empty "--" and "-" values and "the red dot"
    lv_label_set_text(ui_LabelHeading, "---");
    lv_label_set_text(ui_LabelHeadingMode, "-");
    lv_obj_set_style_bg_color(ui_PanelConnected, lv_color_hex(COLOR_DISCONNECTED), LV_PART_MAIN | LV_STATE_DEFAULT);

    // Reset cached values
    _last_heading_x10 = 0xFFFF;
    _last_heading_deg = 0xFFFF;
    _last_is_true = false;
    _last_connected = false;
}

// Rotate the compass rose UI image element based on heading
void CompassUI::setCompassRotation(uint16_t heading_x10) {
    // Threshold to avoid heavy LVGL re-render on small changes
    if (_last_heading_x10 != 0xFFFF) {
        int16_t diff = (int16_t)heading_x10 - (int16_t)_last_heading_x10;
        if (diff > 1800) diff -= 3600;
        if (diff < -1800) diff += 3600;
        if (abs(diff) < ROTATION_THRESHOLD_X10) return;
    }

    _last_heading_x10 = heading_x10;

    // Rotate the rose to the opposite direction
    int16_t angle = -(int16_t)heading_x10;
    lv_img_set_angle(ui_ImageCompassRose, angle);
}

// Update the heading value to the UI label element
void CompassUI::updateHeadingLabel(uint16_t heading_deg) {
    if (heading_deg == _last_heading_deg) return;
    _last_heading_deg = heading_deg;

    char buf[16];
    // UI font has to contain UTF-8 degree sign U+00B0

    snprintf(buf, sizeof(buf), "%03d°", heading_deg);
    lv_label_set_text(ui_LabelHeading, buf);
}

// Update the heading mode value to the UI label element
void CompassUI::updateHeadingMode(bool is_true) {
    if (is_true == _last_is_true) return;
    _last_is_true = is_true;

    lv_label_set_text(ui_LabelHeadingMode, is_true ? "T" : "M");
}

// Set the color of connection indicator UI panel element, "the red dot"
void CompassUI::setConnectionIndicator(bool connected) {
    if (connected == _last_connected) return;
    _last_connected = connected;

    lv_color_t color = connected ? lv_color_hex(COLOR_CONNECTED) : lv_color_hex(COLOR_DISCONNECTED);
    lv_obj_set_style_bg_color(ui_PanelConnected, color, LV_PART_MAIN | LV_STATE_DEFAULT);
}
