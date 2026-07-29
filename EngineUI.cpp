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

// Realizes update(): fetch engine, fuel tank and fresh water tank data from receiver and update UI
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

    if (_receiver.hasNewWaterData()) {
        HALMETWaterDelta water = _receiver.getWaterData();
        if (!isnan(water.water_level_ratio)) {
            _last_water_millis = millis();
            updateWaterLevel(water.water_level_ratio);
        }
    }

    bool engine_connected = (_last_engine_millis > 0 && (millis() - _last_engine_millis) < CONNECTION_TIMEOUT_MS);
    bool tank_connected   = (_last_tank_millis   > 0 && (millis() - _last_tank_millis)   < CONNECTION_TIMEOUT_MS);
    bool water_connected  = (_last_water_millis  > 0 && (millis() - _last_water_millis)  < CONNECTION_TIMEOUT_MS);

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

    if (!water_connected && _last_water_connected) {
        lv_label_set_text(ui_LabelWtrLitres, "---");
    }
    _last_water_connected = water_connected;
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

// Show one view, hide the others
void EngineUI::showView(EngineView view) {
    _active_view = view;
    _last_fuel_arc_value  = -1;
    _last_fuel_arc_color  = 0xFFFFFFFF;
    _last_water_arc_value = -1;
    _last_water_arc_color = 0xFFFFFFFF;

    lv_obj_add_flag(ui_PanelExhaustTemp,    LV_OBJ_FLAG_HIDDEN);
    lv_obj_add_flag(ui_ContainerFuelGauge,  LV_OBJ_FLAG_HIDDEN);
    lv_obj_add_flag(ui_ContainerWaterGauge, LV_OBJ_FLAG_HIDDEN);

    switch (view) {
        case EngineView::EXHAUST:
            lv_obj_clear_flag(ui_PanelExhaustTemp, LV_OBJ_FLAG_HIDDEN);
            break;
        case EngineView::FUEL0:
            lv_obj_clear_flag(ui_ContainerFuelGauge, LV_OBJ_FLAG_HIDDEN);
            break;
        case EngineView::FRESHWATER:
            lv_obj_clear_flag(ui_ContainerWaterGauge, LV_OBJ_FLAG_HIDDEN);
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
    lv_label_set_text(ui_LabelWtrLitres,   "---");
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

// Update one tank gauge: arc value, arc color and litres label.
// The arc uses the LVGL default range 0-100, so the level ratio maps straight to percent.
// The visible "needle" is the arc KNOB part, so indicator and knob are colored together.
void EngineUI::updateGauge(lv_obj_t* arc, lv_obj_t* label, float ratio, float capacity_l,
                           int& last_value, uint32_t& last_color) {
    if (ratio < 0.0f) ratio = 0.0f;
    if (ratio > 1.0f) ratio = 1.0f;

    int arc_val = (int)roundf(ratio * 100.0f);
    if (arc_val != last_value) {
        lv_arc_set_value(arc, arc_val);
        last_value = arc_val;
    }

    uint32_t color = (ratio >= TANK_THRESHOLD_YELLOW) ? TANK_COLOR_GREEN
                   : (ratio >= TANK_THRESHOLD_RED)    ? TANK_COLOR_YELLOW
                                                      : TANK_COLOR_RED;
    if (color != last_color) {
        lv_obj_set_style_arc_color(arc, lv_color_hex(color), LV_PART_INDICATOR | LV_STATE_DEFAULT);
        lv_obj_set_style_bg_color(arc,  lv_color_hex(color), LV_PART_KNOB      | LV_STATE_DEFAULT);
        last_color = color;
    }

    char buf[8];
    snprintf(buf, sizeof(buf), "%d", (int)roundf(ratio * capacity_l));
    lv_label_set_text(label, buf);
}

// Update fuel gauge arc and litres label
void EngineUI::updateFuelLevel(float ratio) {
    updateGauge(ui_ArcFuel, ui_LabelLitres, ratio, FUEL_CAPACITY_L,
                _last_fuel_arc_value, _last_fuel_arc_color);
}

// Update fresh water gauge arc and litres label
void EngineUI::updateWaterLevel(float ratio) {
    updateGauge(ui_ArcWater, ui_LabelWtrLitres, ratio, WATER_CAPACITY_L,
                _last_water_arc_value, _last_water_arc_color);
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
