#include "CompassUI.h"
#include "ui.h"

// === P U B L I C ===

// Constructor
CompassUI::CompassUI(ESPNowReceiver &receiver)
    : _receiver(receiver) {}

// Realizes getLvglScreen(): return the LVGL screen object for this UI
lv_obj_t* CompassUI::getLvglScreen() const {
    return ui_CompassScreen;
}

// Realizes begin(): initialize
void CompassUI::begin() {
    if (_initialized) return;

    _active_view = this->loadView();

    // Compass rose: disable antialiasing (rotation is expensive to render)
    lv_image_set_antialias(ui_ImageCompassRose, false);

    // SOG arc: set range 0–100 (= 0.0–10.0 kn × 10)
    lv_arc_set_range(ui_ArcSog, 0, SOG_ARC_MAX);

    this->showView(_active_view);

    _initialized = true;
    this->showWaiting();
}

// Realizes update(): fetch data and update UI based on active view
void CompassUI::update() {
    if (!_initialized) return;

    if (_active_view == CompassView::HEADING) {
        bool connected = _receiver.isConnected(HEADING_TIMEOUT_MS);
        this->setConnectionIndicator(connected);

        if (!connected) return;  // Keep last known heading on disconnect

        if (_receiver.hasNewData()) {
            HeadingData data = _receiver.getData();
            this->setCompassRotation(data.heading_true_x10);
            this->updateHeadingLabel(data.getHeadingTrueDeg());
        }

    } else {
        // COG and SOG both consume GNSS_DELTA packets
        if (_receiver.hasNewGnssData()) {
            GnssData gnss = _receiver.getGnssData();
            _last_gnss_millis = millis();
            _last_gnss_fix = gnss.hasFix();

            if (_active_view == CompassView::COG) {
                // COG needs a fix AND a defined course. At anchor (fix but stationary,
                // COG undefined) show dashes, not a false 000° — fix_ok no longer folds in
                // COG validity, so gate the course view on hasCog() separately.
                if (_last_gnss_fix && gnss.hasCog()) {
                    this->setCompassRotation(gnss.cog_true_x10);
                    this->updateHeadingLabel(gnss.getCogDeg());
                } else {
                    // No fix, or stationary (no course): reset to dashes
                    lv_label_set_text(ui_LabelHeading, "---°");
                    _last_rose_x10  = 0xFFFF;
                    _last_label_deg = 0xFFFF;
                }
            } else {
                this->updateSogDisplay(gnss.sog_knots_x10, _last_gnss_fix);
            }
        }

        bool connected = (_last_gnss_millis > 0 && (millis() - _last_gnss_millis) < GNSS_TIMEOUT_MS);
        if (!connected && _last_connected) this->showWaiting();
        this->setConnectionIndicator(connected);
    }
}

// Realizes onButtonPress(): cycle to next view
void CompassUI::onButtonPress() {
    if (!_initialized) return;

    uint8_t next = (static_cast<uint8_t>(_active_view) + 1) % static_cast<uint8_t>(CompassView::COUNT);
    this->showView(static_cast<CompassView>(next));
}

// Realizes onLeave(): save active view to NVS
void CompassUI::onLeave() {
    this->saveView();
}

// === P R I V A T E ===

// Show one view, hide the other; update mode label and reset render caches
void CompassUI::showView(CompassView view) {
    _active_view = view;

    // Reset render caches so UI updates immediately with first data
    _last_rose_x10  = 0xFFFF;
    _last_label_deg = 0xFFFF;
    _last_sog_x10   = 0xFFFF;
    _sog_ema        = NAN;
    _sog_ema_ref    = NAN;

    // Force connection dot to red; setConnectionIndicator() will update on next update()
    _last_connected = false;
    lv_obj_set_style_bg_color(ui_PanelConnected, lv_color_hex(COLOR_DISCONNECTED), LV_PART_MAIN | LV_STATE_DEFAULT);

    switch (view) {
        case CompassView::HEADING:
            lv_obj_clear_flag(ui_ContainerCompass, LV_OBJ_FLAG_HIDDEN);
            lv_obj_add_flag(ui_ContainerSog, LV_OBJ_FLAG_HIDDEN);
            lv_label_set_text(ui_LabelHeadingMode, "HDG(T)");
            break;
        case CompassView::COG:
            lv_obj_clear_flag(ui_ContainerCompass, LV_OBJ_FLAG_HIDDEN);
            lv_obj_add_flag(ui_ContainerSog, LV_OBJ_FLAG_HIDDEN);
            lv_label_set_text(ui_LabelHeadingMode, "COG(T)");
            lv_label_set_text(ui_LabelHeading, "---°");
            break;
        case CompassView::SOG:
            lv_obj_add_flag(ui_ContainerCompass, LV_OBJ_FLAG_HIDDEN);
            lv_obj_clear_flag(ui_ContainerSog, LV_OBJ_FLAG_HIDDEN);
            lv_label_set_text(ui_LabelSog, "--.-");
            lv_arc_set_value(ui_ArcSog, 0);
            break;
        default:
            break;
    }
}

// Rotate compass rose — deadband prevents re-render on sub-0.5° changes
void CompassUI::setCompassRotation(uint16_t heading_x10) {
    if (_last_rose_x10 != 0xFFFF) {
        int16_t diff = (int16_t)heading_x10 - (int16_t)_last_rose_x10;
        if (diff > 1800) diff -= 3600;
        if (diff < -1800) diff += 3600;
        if (abs(diff) < ROTATION_THRESHOLD_X10) return;
    }
    _last_rose_x10 = heading_x10;
    lv_image_set_rotation(ui_ImageCompassRose, -(int16_t)heading_x10);
}

// Update heading/COG label with 3-digit leading-zero format e.g. "090°"
void CompassUI::updateHeadingLabel(uint16_t deg) {
    if (deg == _last_label_deg) return;
    _last_label_deg = deg;

    char buf[8];
    snprintf(buf, sizeof(buf), "%03d°", deg);
    lv_label_set_text(ui_LabelHeading, buf);
}

// Update SOG arc and label with EMA smoothing; shows "--.-" when fix_ok is false
void CompassUI::updateSogDisplay(uint16_t sog_knots_x10, bool fix_ok) {
    if (!fix_ok) {
        if (_last_sog_x10 != 0xFFFF) {
            lv_label_set_text(ui_LabelSog, "--.-");
            lv_arc_set_value(ui_ArcSog, 0);
            _last_sog_x10 = 0xFFFF;
            _sog_ema      = NAN;
            _sog_ema_ref  = NAN;
        }
        return;
    }

    float raw_knots = sog_knots_x10 / 10.0f;

    if (isnan(_sog_ema)) {
        // First reading: initialize EMA and reference, display without smoothing
        _sog_ema     = raw_knots;
        _sog_ema_ref = raw_knots;
    } else {
        _sog_ema = SOG_EMA_ALPHA * raw_knots + (1.0f - SOG_EMA_ALPHA) * _sog_ema;
    }

    uint16_t smoothed_x10 = (uint16_t)(_sog_ema * 10.0f);
    if (smoothed_x10 == _last_sog_x10) return;
    _last_sog_x10 = smoothed_x10;

    uint16_t arc_val = (smoothed_x10 > SOG_ARC_MAX) ? SOG_ARC_MAX : smoothed_x10;
    lv_arc_set_value(ui_ArcSog, arc_val);

    char buf[8];
    snprintf(buf, sizeof(buf), "%.1f", _sog_ema);
    lv_label_set_text(ui_LabelSog, buf);
}

// Update connection indicator dot color
void CompassUI::setConnectionIndicator(bool connected) {
    if (connected == _last_connected) return;
    _last_connected = connected;

    lv_color_t color = connected ? lv_color_hex(COLOR_CONNECTED) : lv_color_hex(COLOR_DISCONNECTED);
    lv_obj_set_style_bg_color(ui_PanelConnected, color, LV_PART_MAIN | LV_STATE_DEFAULT);
}

// Reset dynamic labels to waiting state (COG/SOG disconnect; initial state)
void CompassUI::showWaiting() {
    if (!_initialized) return;

    lv_label_set_text(ui_LabelHeading, "---°");
    lv_label_set_text(ui_LabelSog, "--.-");
    lv_arc_set_value(ui_ArcSog, 0);

    _last_rose_x10  = 0xFFFF;
    _last_label_deg = 0xFFFF;
    _last_sog_x10   = 0xFFFF;
    _sog_ema        = NAN;
    _sog_ema_ref    = NAN;
}

// Save active view to NVS
void CompassUI::saveView() {
    Preferences prefs;
    prefs.begin(NVS_NAMESPACE, false);
    prefs.putUChar(NVS_KEY_VIEW, static_cast<uint8_t>(_active_view));
    prefs.end();
}

// Load active view from NVS (default: HEADING)
CompassUI::CompassView CompassUI::loadView() {
    Preferences prefs;
    prefs.begin(NVS_NAMESPACE, true);
    uint8_t val = prefs.getUChar(NVS_KEY_VIEW, 0);
    prefs.end();

    if (val >= static_cast<uint8_t>(CompassView::COUNT)) val = 0;
    return static_cast<CompassView>(val);
}
