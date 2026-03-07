#include "WeatherUI.h"
#include "ui.h"

// === P U B L I C ===

// Constructor
WeatherUI::WeatherUI(ESPNowReceiver &receiver)
    : _receiver(receiver) {}

// Realizes getLvglScreen(): return the LVGL screen object for this UI
lv_obj_t* WeatherUI::getLvglScreen() const {
    return ui_WeatherScreen;
}

// Realizes begin(): initialize
void WeatherUI::begin() {
    if (_initialized) return;

    // Load saved panel from NVS
    _active_panel = this->loadPanel();

    // Trend labels hidden until data arrives
    lv_obj_add_flag(ui_LabelTrendTemp, LV_OBJ_FLAG_HIDDEN);
    lv_obj_add_flag(ui_LabelTrend, LV_OBJ_FLAG_HIDDEN);
    lv_obj_add_flag(ui_LabelTrendHumidity, LV_OBJ_FLAG_HIDDEN);

    // Show the active panel and waiting state
    this->showPanel(_active_panel);
    this->showWaiting();

    _initialized = true;
}

// Realizes update(): fetch weather data from receiver and update UI
void WeatherUI::update() {
    if (!_initialized) return;

    if (_receiver.hasNewWeatherData()) {
        WeatherDelta data = _receiver.getWeatherData();
        _last_data_millis = millis();
        this->updateTemperature(data.temperature_c);
        this->updatePressure(data.pressure_hpa);
        this->updateHumidity(data.humidity_p);
    }

    bool connected = (_last_data_millis > 0 && (millis() - _last_data_millis) < CONNECTION_TIMEOUT_MS);

    if (!connected && _last_connected) this->showWaiting();
    _last_connected = connected;
}

// Realizes onButtonPress(): cycle visible panel
void WeatherUI::onButtonPress() {
    if (!_initialized) return;

    uint8_t next = (static_cast<uint8_t>(_active_panel) + 1) % static_cast<uint8_t>(WeatherPanel::COUNT);
    this->showPanel(static_cast<WeatherPanel>(next));
}

// Realizes onLeave(): save active panel to NVS
void WeatherUI::onLeave() {
    this->savePanel();
}

// === P R I V A T E ===

// Show one panel, hide the others
void WeatherUI::showPanel(WeatherPanel panel) {
    _active_panel = panel;

    // Hide all
    lv_obj_add_flag(ui_PanelTemperature, LV_OBJ_FLAG_HIDDEN);
    lv_obj_add_flag(ui_PanelPressure, LV_OBJ_FLAG_HIDDEN);
    lv_obj_add_flag(ui_PanelHumidity, LV_OBJ_FLAG_HIDDEN);

    // Show one
    switch (panel) {
        case WeatherPanel::TEMPERATURE:
            lv_obj_clear_flag(ui_PanelTemperature, LV_OBJ_FLAG_HIDDEN);
            break;
        case WeatherPanel::PRESSURE:
            lv_obj_clear_flag(ui_PanelPressure, LV_OBJ_FLAG_HIDDEN);
            break;
        case WeatherPanel::HUMIDITY:
            lv_obj_clear_flag(ui_PanelHumidity, LV_OBJ_FLAG_HIDDEN);
            break;
        default:
            break;
    }
}

// Show "waiting for data" — set main value labels to "---", hide trend
void WeatherUI::showWaiting() {
    if (!_initialized) return;

    lv_label_set_text(ui_LabelTemp, "---");
    lv_label_set_text(ui_LabelPressure, "---");
    lv_label_set_text(ui_LabelHumidity, "---");
    lv_obj_add_flag(ui_LabelTrendTemp, LV_OBJ_FLAG_HIDDEN);
    lv_obj_add_flag(ui_LabelTrend, LV_OBJ_FLAG_HIDDEN);
    lv_obj_add_flag(ui_LabelTrendHumidity, LV_OBJ_FLAG_HIDDEN);

    // Min/max labels: show "---" only if no session data yet
    if (isnan(_min_temp)) {
        lv_label_set_text(ui_LabelMinTemp, "---");
        lv_label_set_text(ui_LabelMaxTemp, "---");
    }
    if (isnan(_min_pressure)) {
        lv_label_set_text(ui_LabelMinPressure, "---");
        lv_label_set_text(ui_LabelMaxPressure, "---");
    }
    if (isnan(_min_humidity)) {
        lv_label_set_text(ui_LabelMinHumidity, "---");
        lv_label_set_text(ui_LabelMaxHumidity, "---");
    }
}

// Update temperature value and session min/max
void WeatherUI::updateTemperature(float temp_c) {
    _temperature_c = temp_c;

    if (isnan(_min_temp) || temp_c < _min_temp) _min_temp = temp_c;
    if (isnan(_max_temp) || temp_c > _max_temp) _max_temp = temp_c;

    char buf[16];
    snprintf(buf, sizeof(buf), "%+.0f°C", temp_c);
    lv_label_set_text(ui_LabelTemp, buf);

    snprintf(buf, sizeof(buf), "Max %+.0f°C", _max_temp);
    lv_label_set_text(ui_LabelMaxTemp, buf);

    snprintf(buf, sizeof(buf), "Min %+.0f°C", _min_temp);
    lv_label_set_text(ui_LabelMinTemp, buf);

    // Trend indicator update based on EMA
    if (isnan(_temperature_ema)) {
        // First reading — initialize EMA and reference, hide trend
        _temperature_ema = temp_c;
        _temperature_ema_ref = temp_c;
        lv_obj_add_flag(ui_LabelTrendTemp, LV_OBJ_FLAG_HIDDEN);
        return;
    }
    _temperature_ema = TEMPERATURE_EMA_ALPHA * temp_c + (1.0f - TEMPERATURE_EMA_ALPHA) * _temperature_ema;

    float diff = _temperature_ema - _temperature_ema_ref;
    if (diff >= TEMPERATURE_TREND_THRESHOLD) {
        lv_obj_clear_flag(ui_LabelTrendTemp, LV_OBJ_FLAG_HIDDEN);
        lv_label_set_text(ui_LabelTrendTemp, "↑");
    } else if (diff <= -TEMPERATURE_TREND_THRESHOLD) {
        lv_obj_clear_flag(ui_LabelTrendTemp, LV_OBJ_FLAG_HIDDEN);
        lv_label_set_text(ui_LabelTrendTemp, "↓");
    } else {
        // Neutral zone — hide indicator and drift reference toward EMA
        lv_obj_add_flag(ui_LabelTrendTemp, LV_OBJ_FLAG_HIDDEN);
        _temperature_ema_ref = _temperature_ema;
    }
}

// Update pressure value, session min/max, and trend indicator
void WeatherUI::updatePressure(float pres_hpa) {
    _pressure_hpa = pres_hpa;

    if (isnan(_min_pressure) || pres_hpa < _min_pressure) _min_pressure = pres_hpa;
    if (isnan(_max_pressure) || pres_hpa > _max_pressure) _max_pressure = pres_hpa;

    char buf[16];
    snprintf(buf, sizeof(buf), "%.0f", pres_hpa);
    lv_label_set_text(ui_LabelPressure, buf);

    snprintf(buf, sizeof(buf), "Max %.0f", _max_pressure);
    lv_label_set_text(ui_LabelMaxPressure, buf);

    snprintf(buf, sizeof(buf), "Min %.0f", _min_pressure);
    lv_label_set_text(ui_LabelMinPressure, buf);

    // Trend indicator update based on EMA
    if (isnan(_pressure_ema)) {
        // First reading — initialize EMA and reference, hide trend
        _pressure_ema     = pres_hpa;
        _pressure_ema_ref = pres_hpa;
        lv_obj_add_flag(ui_LabelTrend, LV_OBJ_FLAG_HIDDEN);
        return;
    }
    _pressure_ema = PRESSURE_EMA_ALPHA * pres_hpa + (1.0f - PRESSURE_EMA_ALPHA) * _pressure_ema;

    float diff = _pressure_ema - _pressure_ema_ref;
    if (diff >= PRESSURE_TREND_THRESHOLD) {
        lv_obj_clear_flag(ui_LabelTrend, LV_OBJ_FLAG_HIDDEN);
        lv_label_set_text(ui_LabelTrend, "↑");
    } else if (diff <= -PRESSURE_TREND_THRESHOLD) {
        lv_obj_clear_flag(ui_LabelTrend, LV_OBJ_FLAG_HIDDEN);
        lv_label_set_text(ui_LabelTrend, "↓");
    } else {
        // Neutral zone — hide indicator and drift reference toward EMA
        lv_obj_add_flag(ui_LabelTrend, LV_OBJ_FLAG_HIDDEN);
        _pressure_ema_ref = _pressure_ema;
    }

}

// Update humidity value and session min/max
void WeatherUI::updateHumidity(float hum_p) {
    _humidity_p = hum_p;

    if (isnan(_min_humidity) || hum_p < _min_humidity) _min_humidity = hum_p;
    if (isnan(_max_humidity) || hum_p > _max_humidity) _max_humidity = hum_p;

    char buf[16];
    snprintf(buf, sizeof(buf), "%.0f%%", hum_p);
    lv_label_set_text(ui_LabelHumidity, buf);

    snprintf(buf, sizeof(buf), "Max %.0f%%", _max_humidity);
    lv_label_set_text(ui_LabelMaxHumidity, buf);

    snprintf(buf, sizeof(buf), "Min %.0f%%", _min_humidity);
    lv_label_set_text(ui_LabelMinHumidity, buf);

    // Trend indicator update based on EMA
    if (isnan(_humidity_ema)) {
        // First reading — initialize EMA and reference, hide trend
        _humidity_ema = hum_p;
        _humidity_ema_ref = hum_p;
        lv_obj_add_flag(ui_LabelTrendHumidity, LV_OBJ_FLAG_HIDDEN);
        return;
    }
    _humidity_ema = HUMIDITY_EMA_ALPHA * hum_p + (1.0f - HUMIDITY_EMA_ALPHA) * _humidity_ema;

    float diff = _humidity_ema - _humidity_ema_ref;
    if (diff >= HUMIDITY_TREND_THRESHOLD) {
        lv_obj_clear_flag(ui_LabelTrendHumidity, LV_OBJ_FLAG_HIDDEN);
        lv_label_set_text(ui_LabelTrendHumidity, "↑");
    } else if (diff <= -HUMIDITY_TREND_THRESHOLD) {
        lv_obj_clear_flag(ui_LabelTrendHumidity, LV_OBJ_FLAG_HIDDEN);
        lv_label_set_text(ui_LabelTrendHumidity, "↓");
    } else {
        // Neutral zone — hide indicator and drift reference toward EMA
        lv_obj_add_flag(ui_LabelTrendHumidity, LV_OBJ_FLAG_HIDDEN);
        _humidity_ema_ref = _humidity_ema;
    }
}

// Save active panel to NVS
void WeatherUI::savePanel() {
    Preferences prefs;
    prefs.begin(NVS_NAMESPACE, false);
    prefs.putUChar(NVS_KEY_PANEL, static_cast<uint8_t>(_active_panel));
    prefs.end();
}

// Load active panel from NVS
WeatherUI::WeatherPanel WeatherUI::loadPanel() {
    Preferences prefs;
    prefs.begin(NVS_NAMESPACE, true);
    uint8_t val = prefs.getUChar(NVS_KEY_PANEL, 0);  // default: TEMPERATURE
    prefs.end();

    if (val >= static_cast<uint8_t>(WeatherPanel::COUNT)) val = 0;
    return static_cast<WeatherPanel>(val);
}
