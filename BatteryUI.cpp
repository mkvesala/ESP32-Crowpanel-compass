#include "BatteryUI.h"
#include "ui.h"

// === P U B L I C ===

// Constructor
BatteryUI::BatteryUI(ESPNowReceiver &receiver)
    : _receiver(receiver) {}

// Realizes getLvglScreen(): return the LVGL screen object for this UI
lv_obj_t* BatteryUI::getLvglScreen() const {
    return ui_BatteryScreen;
}

// Realizes begin(): initialize
void BatteryUI::begin() {
    if (_initialized) return;

    // Load saved panel from NVS
    _active_panel = this->loadPanel();

    lv_obj_add_flag(ui_LabelHouse, LV_OBJ_FLAG_HIDDEN);
    lv_obj_add_flag(ui_LabelStarter, LV_OBJ_FLAG_HIDDEN);

    // Trend labels hidden until data arrives
    lv_obj_add_flag(ui_LabelHouseVoltageTrend, LV_OBJ_FLAG_HIDDEN);
    lv_obj_add_flag(ui_LabelHouseAmpsTrend, LV_OBJ_FLAG_HIDDEN);
    lv_obj_add_flag(ui_LabelHouseSocTrend, LV_OBJ_FLAG_HIDDEN);
    lv_obj_add_flag(ui_LabelStarterVoltageTrend, LV_OBJ_FLAG_HIDDEN);

    // Show the active panel and waiting state
    this->showPanel(_active_panel);
    this->showWaiting();

    _initialized = true;
}

// Realizes update(): fetch weather data from receiver and update UI
void BatteryUI::update() {
    if (!_initialized) return;

    if (_receiver.hasNewBatteryData()) {
        BatteryDelta data = _receiver.getBatteryData();
        _last_data_millis = millis();
        this->updateHouseVoltage(data.house_voltage);
        this->updateHouseCurrent(data.house_current);
        this->updateHouseSoc(data.house_soc);
        this->updateStartVoltage(data.start_voltage);
    }

    bool connected = (_last_data_millis > 0 && (millis() - _last_data_millis) < CONNECTION_TIMEOUT_MS);

    if (!connected && _last_connected) this->showWaiting();
    _last_connected = connected;
}

// Realizes onButtonPress(): cycle visible panel
void BatteryUI::onButtonPress() {
    if (!_initialized) return;

    uint8_t next = (static_cast<uint8_t>(_active_panel) + 1) % static_cast<uint8_t>(BatteryPanel::COUNT);
    this->showPanel(static_cast<BatteryPanel>(next));
}

// Realizes onLeave(): save active panel to NVS
void BatteryUI::onLeave() {
    this->savePanel();
}

// === P R I V A T E ===

// Show one panel, hide the others
void BatteryUI::showPanel(BatteryPanel panel) {
    _active_panel = panel;

    // Hide all
    lv_obj_add_flag(ui_PanelHouseVoltage, LV_OBJ_FLAG_HIDDEN);
    lv_obj_add_flag(ui_PanelHouseAmps, LV_OBJ_FLAG_HIDDEN);
    lv_obj_add_flag(ui_PanelHouseSoc, LV_OBJ_FLAG_HIDDEN);
    lv_obj_add_flag(ui_PanelStarterVoltage, LV_OBJ_FLAG_HIDDEN);

    // Show one
    switch (panel) {
        case BatteryPanel::HOUSE_V:
            lv_obj_clear_flag(ui_PanelHouseVoltage, LV_OBJ_FLAG_HIDDEN);
            lv_obj_clear_flag(ui_LabelHouse, LV_OBJ_FLAG_HIDDEN);
            break;
        case BatteryPanel::HOUSE_A:
            lv_obj_clear_flag(ui_PanelHouseAmps, LV_OBJ_FLAG_HIDDEN);
            lv_obj_clear_flag(ui_LabelHouse, LV_OBJ_FLAG_HIDDEN);
            break;
        case BatteryPanel::HOUSE_SOC:
            lv_obj_clear_flag(ui_PanelHouseSoc, LV_OBJ_FLAG_HIDDEN);
            lv_obj_clear_flag(ui_LabelHouse, LV_OBJ_FLAG_HIDDEN);
            break;
        case BatteryPanel::START_V:
            lv_obj_clear_flag(ui_PanelStarterVoltage, LV_OBJ_FLAG_HIDDEN);
            lv_obj_clear_flag(ui_LabelStarter, LV_OBJ_FLAG_HIDDEN);
            break;
        default:
            break;
    }
}

// Show "waiting for data" — set main value labels to "---", hide trend
void BatteryUI::showWaiting() {
    if (!_initialized) return;

    lv_label_set_text(ui_LabelHouseVoltage, "---");
    lv_label_set_text(ui_LabelHouseAmps, "---");
    lv_label_set_text(ui_LabelHouseSoc, "---");
    lv_label_set_text(ui_LabelStarterVoltage, "---");

    lv_obj_add_flag(ui_LabelHouseVoltageTrend, LV_OBJ_FLAG_HIDDEN);
    lv_obj_add_flag(ui_LabelHouseAmpsTrend, LV_OBJ_FLAG_HIDDEN);
    lv_obj_add_flag(ui_LabelHouseSocTrend, LV_OBJ_FLAG_HIDDEN);
    lv_obj_add_flag(ui_LabelStarterVoltageTrend, LV_OBJ_FLAG_HIDDEN);

    // Min/max labels: show "---" only if no session data yet
    if (isnan(_min_house_v)) {
        lv_label_set_text(ui_LabelMinHouseVoltage, "---");
        lv_label_set_text(ui_LabelMaxHouseVoltage, "---");
    }
    if (isnan(_min_house_a)) {
        lv_label_set_text(ui_LabelMinHouseAmps, "---");
        lv_label_set_text(ui_LabelMaxHouseAmps, "---");
    }
    if (isnan(_min_house_soc)) {
        lv_label_set_text(ui_LabelMinHouseSoc, "---");
        lv_label_set_text(ui_LabelMaxHouseSoc, "---");
    }
    if (isnan(_min_start_v)) {
        lv_label_set_text(ui_LabelMinStarterVoltage, "---");
        lv_label_set_text(ui_LabelMaxStarterVoltage, "---");
    }
}

// Update house battery bank voltage (volts)
void BatteryUI::updateHouseVoltage(float house_v) {
    _house_voltage = house_v;

    if (isnan(_min_house_v) || house_v < _min_house_v) _min_house_v = house_v;
    if (isnan(_max_house_v) || house_v > _max_house_v) _max_house_v = house_v;

    char buf[16];
    snprintf(buf, sizeof(buf), "%.1fV", house_v);
    lv_label_set_text(ui_LabelHouseVoltage, buf);

    snprintf(buf, sizeof(buf), "Max %.1fV", _max_house_v);
    lv_label_set_text(ui_LabelMaxHouseVoltage, buf);

    snprintf(buf, sizeof(buf), "Min %.1fV", _min_house_v);
    lv_label_set_text(ui_LabelMinHouseVoltage, buf);

    // Trend indicator update based on EMA
    if (isnan(_house_v_ema)) {
        // First reading — initialize EMA and reference, hide trend
        _house_v_ema = house_v;
        _house_v_ema_ref = house_v;
        lv_obj_add_flag(ui_LabelHouseVoltageTrend, LV_OBJ_FLAG_HIDDEN);
        return;
    }
    _house_v_ema = VOLTAGE_EMA_ALPHA * house_v + (1.0f - VOLTAGE_EMA_ALPHA) * _house_v_ema;

    float diff = _house_v_ema - _house_v_ema_ref;
    if (diff >= VOLTAGE_TREND_THRESHOLD) {
        lv_obj_clear_flag(ui_LabelHouseVoltageTrend, LV_OBJ_FLAG_HIDDEN);
        lv_label_set_text(ui_LabelHouseVoltageTrend, "↑");
    } else if (diff <= -VOLTAGE_TREND_THRESHOLD) {
        lv_obj_clear_flag(ui_LabelHouseVoltageTrend, LV_OBJ_FLAG_HIDDEN);
        lv_label_set_text(ui_LabelHouseVoltageTrend, "↓");
    } else {
        // Neutral zone — hide indicator and drift reference toward EMA
        lv_obj_add_flag(ui_LabelHouseVoltageTrend, LV_OBJ_FLAG_HIDDEN);
        _house_v_ema_ref = _house_v_ema;
    }
}

// Update house battery bank current (amps)
void BatteryUI::updateHouseCurrent(float house_a) {
    _house_current = house_a;

    if (isnan(_min_house_a) || house_a < _min_house_a) _min_house_a = house_a;
    if (isnan(_max_house_a) || house_a > _max_house_a) _max_house_a = house_a;

    char buf[16];
    snprintf(buf, sizeof(buf), "%+.1fA", house_a);
    lv_label_set_text(ui_LabelHouseAmps, buf);

    snprintf(buf, sizeof(buf), "Max %+.1fA", _max_house_a);
    lv_label_set_text(ui_LabelMaxHouseAmps, buf);

    snprintf(buf, sizeof(buf), "Min %+.1fA", _min_house_a);
    lv_label_set_text(ui_LabelMinHouseAmps, buf);

    // Trend indicator update based on EMA
    if (isnan(_house_a_ema)) {
        // First reading — initialize EMA and reference, hide trend
        _house_a_ema     = house_a;
        _house_a_ema_ref = house_a;
        lv_obj_add_flag(ui_LabelHouseAmpsTrend, LV_OBJ_FLAG_HIDDEN);
        return;
    }
    _house_a_ema = CURRENT_EMA_ALPHA * house_a + (1.0f - CURRENT_EMA_ALPHA) * _house_a_ema;

    float diff = _house_a_ema - _house_a_ema_ref;
    if (diff >= CURRENT_TREND_THRESHOLD) {
        lv_obj_clear_flag(ui_LabelHouseAmpsTrend, LV_OBJ_FLAG_HIDDEN);
        lv_label_set_text(ui_LabelHouseAmpsTrend, "↑");
    } else if (diff <= -CURRENT_TREND_THRESHOLD) {
        lv_obj_clear_flag(ui_LabelHouseAmpsTrend, LV_OBJ_FLAG_HIDDEN);
        lv_label_set_text(ui_LabelHouseAmpsTrend, "↓");
    } else {
        // Neutral zone — hide indicator and drift reference toward EMA
        lv_obj_add_flag(ui_LabelHouseAmpsTrend, LV_OBJ_FLAG_HIDDEN);
        _house_a_ema_ref = _house_a_ema;
    }

}

// Update house battery bank state-of-charge (%)
void BatteryUI::updateHouseSoc(float soc_p) {
    _house_soc = soc_p;

    if (isnan(_min_house_soc) || soc_p < _min_house_soc) _min_house_soc = soc_p;
    if (isnan(_max_house_soc) || soc_p > _max_house_soc) _max_house_soc = soc_p;

    char buf[16];
    snprintf(buf, sizeof(buf), "%.0f%%", soc_p);
    lv_label_set_text(ui_LabelHouseSoc, buf);

    snprintf(buf, sizeof(buf), "Max %.0f%%", _max_house_soc);
    lv_label_set_text(ui_LabelMaxHouseSoc, buf);

    snprintf(buf, sizeof(buf), "Min %.0f%%", _min_house_soc);
    lv_label_set_text(ui_LabelMinHouseSoc, buf);

    // Trend indicator update based on EMA
    if (isnan(_house_soc_ema)) {
        // First reading — initialize EMA and reference, hide trend
        _house_soc_ema = soc_p;
        _house_soc_ema_ref = soc_p;
        lv_obj_add_flag(ui_LabelHouseSocTrend, LV_OBJ_FLAG_HIDDEN);
        return;
    }
    _house_soc_ema = SOC_EMA_ALPHA * soc_p + (1.0f - SOC_EMA_ALPHA) * _house_soc_ema;

    float diff = _house_soc_ema - _house_soc_ema_ref;
    if (diff >= SOC_TREND_THRESHOLD) {
        lv_obj_clear_flag(ui_LabelHouseSocTrend, LV_OBJ_FLAG_HIDDEN);
        lv_label_set_text(ui_LabelHouseSocTrend, "↑");
    } else if (diff <= -SOC_TREND_THRESHOLD) {
        lv_obj_clear_flag(ui_LabelHouseSocTrend, LV_OBJ_FLAG_HIDDEN);
        lv_label_set_text(ui_LabelHouseSocTrend, "↓");
    } else {
        // Neutral zone — hide indicator and drift reference toward EMA
        lv_obj_add_flag(ui_LabelHouseSocTrend, LV_OBJ_FLAG_HIDDEN);
        _house_soc_ema_ref = _house_soc_ema;
    }
}

// Update starter battery bank voltage (volts)
void BatteryUI::updateStartVoltage(float start_v) {
    _start_voltage = start_v;

    if (isnan(_min_start_v) || start_v < _min_start_v) _min_start_v = start_v;
    if (isnan(_max_start_v) || start_v > _max_start_v) _max_start_v = start_v;

    char buf[16];
    snprintf(buf, sizeof(buf), "%.1fV", start_v);
    lv_label_set_text(ui_LabelStarterVoltage, buf);

    snprintf(buf, sizeof(buf), "Max %.1fV", _max_start_v);
    lv_label_set_text(ui_LabelMaxStarterVoltage, buf);

    snprintf(buf, sizeof(buf), "Min %.1fV", _min_start_v);
    lv_label_set_text(ui_LabelMinStarterVoltage, buf);

    // Trend indicator update based on EMA
    if (isnan(_start_v_ema)) {
        // First reading — initialize EMA and reference, hide trend
        _start_v_ema = start_v;
        _start_v_ema_ref = start_v;
        lv_obj_add_flag(ui_LabelStarterVoltageTrend, LV_OBJ_FLAG_HIDDEN);
        return;
    }
    _start_v_ema = VOLTAGE_EMA_ALPHA * start_v + (1.0f - VOLTAGE_EMA_ALPHA) * _start_v_ema;

    float diff = _start_v_ema - _start_v_ema_ref;
    if (diff >= VOLTAGE_TREND_THRESHOLD) {
        lv_obj_clear_flag(ui_LabelStarterVoltageTrend, LV_OBJ_FLAG_HIDDEN);
        lv_label_set_text(ui_LabelStarterVoltageTrend, "↑");
    } else if (diff <= -VOLTAGE_TREND_THRESHOLD) {
        lv_obj_clear_flag(ui_LabelStarterVoltageTrend, LV_OBJ_FLAG_HIDDEN);
        lv_label_set_text(ui_LabelStarterVoltageTrend, "↓");
    } else {
        // Neutral zone — hide indicator and drift reference toward EMA
        lv_obj_add_flag(ui_LabelStarterVoltageTrend, LV_OBJ_FLAG_HIDDEN);
        _start_v_ema_ref = _start_v_ema;
    }
}

// Save active panel to NVS
void BatteryUI::savePanel() {
    Preferences prefs;
    prefs.begin(NVS_NAMESPACE, false);
    prefs.putUChar(NVS_KEY_PANEL, static_cast<uint8_t>(_active_panel));
    prefs.end();
}

// Load active panel from NVS
BatteryUI::BatteryPanel BatteryUI::loadPanel() {
    Preferences prefs;
    prefs.begin(NVS_NAMESPACE, true);
    uint8_t val = prefs.getUChar(NVS_KEY_PANEL, 0); 
    prefs.end();

    if (val >= static_cast<uint8_t>(BatteryPanel::COUNT)) val = 0;
    return static_cast<BatteryPanel>(val);
}
