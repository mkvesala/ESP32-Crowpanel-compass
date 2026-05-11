#include "EngineUI.h"
#include "ui.h"

// === P U B L I C ===

// Constructor
EngineUI::EngineUI(ESPNowReceiver& receiver)
    : _receiver(receiver) {}

// Realizes getLvglScreen(): return the LVGL screen object for this UI
lv_obj_t* EngineUI::getLvglScreen() const {
    return ui_EngineScreen;
}

// Realizes begin(): initialize
void EngineUI::begin() {
    if (_initialized) return;

    _active_view = loadView();

    // Trend label hidden until data arrives
    lv_obj_add_flag(ui_LabelTrendExhaustTemp, LV_OBJ_FLAG_HIDDEN);

    showView(_active_view);
    showWaiting();

    _initialized = true;
}

// Realizes update(): fetch engine and tank data from receiver and update UI
void EngineUI::update() {
    if (!_initialized) return;

    if (_receiver.hasNewEngineData()) {
        HALMETEngineDelta eng = _receiver.getEngineData();
        if (!isnan(eng.exhaust_temp_k)) {
            _last_engine_millis = millis();
            updateExhaustTemp(eng.exhaust_temp_k - 273.15f);
        }
    }

    if (_receiver.hasNewTankData()) {
        HALMETTankDelta tank = _receiver.getTankData();
        if (!isnan(tank.fuel_level_ratio)) {
            _last_tank_millis = millis();
            updateFuelLevel(tank.fuel_level_ratio);
        }
    }

    bool engine_connected = (_last_engine_millis > 0 && (millis() - _last_engine_millis) < CONNECTION_TIMEOUT_MS);
    bool tank_connected   = (_last_tank_millis   > 0 && (millis() - _last_tank_millis)   < CONNECTION_TIMEOUT_MS);

    if (!engine_connected && _last_engine_connected) {
        lv_label_set_text(ui_LabelExhaustTemp, "---");
        lv_obj_add_flag(ui_LabelTrendExhaustTemp, LV_OBJ_FLAG_HIDDEN);
        if (isnan(_exhaust_min_c)) {
            lv_label_set_text(ui_LabelMinExhaustTemp, "---");
            lv_label_set_text(ui_LabelMaxExhaustTemp, "---");
        }
    }
    _last_engine_connected = engine_connected;

    if (!tank_connected && _last_tank_connected) {
        lv_label_set_text(ui_LabelLitres, "---");
    }
    _last_tank_connected = tank_connected;
}

// Realizes onButtonPress(): cycle visible view
void EngineUI::onButtonPress() {
    if (!_initialized) return;
    uint8_t next = (static_cast<uint8_t>(_active_view) + 1) % static_cast<uint8_t>(EngineView::COUNT);
    showView(static_cast<EngineView>(next));
}

// Realizes onLeave(): save active view to NVS
void EngineUI::onLeave() {
    saveView();
}

// === P R I V A T E ===

// Show one view, hide the other
void EngineUI::showView(EngineView view) {
    _active_view = view;
    _last_arc_value = -1;

    lv_obj_add_flag(ui_PanelExhaustTemp,   LV_OBJ_FLAG_HIDDEN);
    lv_obj_add_flag(ui_ContainerFuelGauge, LV_OBJ_FLAG_HIDDEN);

    switch (view) {
        case EngineView::EXHAUST:
            lv_obj_clear_flag(ui_PanelExhaustTemp, LV_OBJ_FLAG_HIDDEN);
            break;
        case EngineView::FUEL0:
            lv_obj_clear_flag(ui_ContainerFuelGauge, LV_OBJ_FLAG_HIDDEN);
            break;
        default:
            break;
    }
}

// Show "waiting for data" — reset main value labels, hide trend
void EngineUI::showWaiting() {
    if (!_initialized) return;

    lv_label_set_text(ui_LabelExhaustTemp, "---");
    lv_label_set_text(ui_LabelLitres,      "---");
    lv_obj_add_flag(ui_LabelTrendExhaustTemp, LV_OBJ_FLAG_HIDDEN);

    if (isnan(_exhaust_min_c)) {
        lv_label_set_text(ui_LabelMinExhaustTemp, "---");
        lv_label_set_text(ui_LabelMaxExhaustTemp, "---");
    }
}

// Update exhaust temperature, session min/max and trend indicator
void EngineUI::updateExhaustTemp(float temp_c) {
    _exhaust_temp_c = temp_c;

    if (isnan(_exhaust_min_c) || temp_c < _exhaust_min_c) _exhaust_min_c = temp_c;
    if (isnan(_exhaust_max_c) || temp_c > _exhaust_max_c) _exhaust_max_c = temp_c;

    char buf[16];
    snprintf(buf, sizeof(buf), "%+.0f°C", temp_c);
    lv_label_set_text(ui_LabelExhaustTemp, buf);

    snprintf(buf, sizeof(buf), "Max %+.0f°C", _exhaust_max_c);
    lv_label_set_text(ui_LabelMaxExhaustTemp, buf);

    snprintf(buf, sizeof(buf), "Min %+.0f°C", _exhaust_min_c);
    lv_label_set_text(ui_LabelMinExhaustTemp, buf);

    if (isnan(_exhaust_ema)) {
        // First reading — initialize EMA and reference, hide trend
        _exhaust_ema     = temp_c;
        _exhaust_ema_ref = temp_c;
        lv_obj_add_flag(ui_LabelTrendExhaustTemp, LV_OBJ_FLAG_HIDDEN);
        return;
    }
    _exhaust_ema = EXHAUST_EMA_ALPHA * temp_c + (1.0f - EXHAUST_EMA_ALPHA) * _exhaust_ema;

    float diff = _exhaust_ema - _exhaust_ema_ref;
    if (diff >= EXHAUST_TREND_THRESHOLD) {
        lv_obj_clear_flag(ui_LabelTrendExhaustTemp, LV_OBJ_FLAG_HIDDEN);
        lv_label_set_text(ui_LabelTrendExhaustTemp, "↑");
    } else if (diff <= -EXHAUST_TREND_THRESHOLD) {
        lv_obj_clear_flag(ui_LabelTrendExhaustTemp, LV_OBJ_FLAG_HIDDEN);
        lv_label_set_text(ui_LabelTrendExhaustTemp, "↓");
    } else {
        // Neutral zone — hide indicator and drift reference toward EMA
        lv_obj_add_flag(ui_LabelTrendExhaustTemp, LV_OBJ_FLAG_HIDDEN);
        _exhaust_ema_ref = _exhaust_ema;
    }
}

// Update fuel gauge arc and litres label
void EngineUI::updateFuelLevel(float ratio) {
    if (ratio < 0.0f) ratio = 0.0f;
    if (ratio > 1.0f) ratio = 1.0f;

    int arc_val = (int)roundf(ratio * 100.0f);
    if (arc_val != _last_arc_value) {
        lv_arc_set_value(ui_ArcFuel, arc_val);
        _last_arc_value = arc_val;
    }

    char buf[8];
    snprintf(buf, sizeof(buf), "%d", (int)roundf(ratio * TANK_CAPACITY_L));
    lv_label_set_text(ui_LabelLitres, buf);
}

// Save active view to NVS
void EngineUI::saveView() {
    Preferences prefs;
    prefs.begin(NVS_NAMESPACE, false);
    prefs.putUChar(NVS_KEY_VIEW, static_cast<uint8_t>(_active_view));
    prefs.end();
}

// Load active view from NVS
EngineUI::EngineView EngineUI::loadView() {
    Preferences prefs;
    prefs.begin(NVS_NAMESPACE, true);
    uint8_t val = prefs.getUChar(NVS_KEY_VIEW, 0);  // default: EXHAUST
    prefs.end();

    if (val >= static_cast<uint8_t>(EngineView::COUNT)) val = 0;
    return static_cast<EngineView>(val);
}
