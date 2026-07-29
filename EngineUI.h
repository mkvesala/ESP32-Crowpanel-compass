#pragma once

#include <Arduino.h>
#include <lvgl.h>
#include <Preferences.h>
#include "IScreenUI.h"
#include "ESPNowReceiver.h"

// === C L A S S  E N G I N E U I ===
//
// - Class EngineUI - responsible for updating LVGL UI elements on EngineScreen
// - Receives: ESPNowPacket<HALMETEngineDelta> (msg type 5), ESPNowPacket<HALMETTankDelta> (msg type 6)
//   and ESPNowPacket<HALMETWaterDelta> (msg type 7)
// - View cycle: EXHAUST (pakoputken lämpötila) → FUEL0 (polttoainesäiliö) → FRESHWATER (vesisäiliö) → EXHAUST ...
// - Knob button press: cycles EngineView
// - Active view: persisted to NVS on onLeave()
// - EXHAUST: temperature °C, session min/max, EMA trend indicator
// - FUEL0: fuel arc 0-100, litres label (tank capacity 400 L)
// - FRESHWATER: water arc 0-100, litres label (tank capacity 100 L)
// - Min/max: tracked runtime only, not persisted to NVS
// - Realizes: IScreenUI
// - Owned by: CrowPanelApplication

class EngineUI : public IScreenUI {

public:

    explicit EngineUI(ESPNowReceiver& receiver);

    void begin() override;
    lv_obj_t* getLvglScreen() const override;
    void update() override;
    void onButtonPress() override;
    void onLeave() override;

private:

    enum class EngineView : uint8_t {
        EXHAUST    = 0,
        FUEL0      = 1,
        FRESHWATER = 2,
        COUNT      = 3
    };

    ESPNowReceiver& _receiver;
    bool _initialized = false;

    EngineView _active_view = EngineView::EXHAUST;

    // EXHAUST: latest value, session min/max, EMA
    float _exhaust_temp_c  = NAN;
    float _exhaust_min_c   = NAN;
    float _exhaust_max_c   = NAN;
    float _exhaust_ema     = NAN;
    float _exhaust_ema_ref = NAN;

    // EXHAUST: connection tracking
    uint32_t _last_engine_millis   = 0;
    bool     _last_engine_connected = false;

    // FUEL0: connection tracking, arc render cache
    uint32_t _last_tank_millis    = 0;
    bool     _last_tank_connected = false;
    int      _last_fuel_arc_value = -1;
    uint32_t _last_fuel_arc_color = 0xFFFFFFFF;  // sentinel: force color set on first update

    // FRESHWATER: connection tracking, arc render cache (own pair — shared cache would
    // suppress writes to whichever arc was not updated last)
    uint32_t _last_water_millis    = 0;
    bool     _last_water_connected = false;
    int      _last_water_arc_value = -1;
    uint32_t _last_water_arc_color = 0xFFFFFFFF;  // sentinel: force color set on first update

    void showView(EngineView view);
    void showWaiting();
    void updateExhaustTemp(float temp_c);
    void updateGauge(lv_obj_t* arc, lv_obj_t* label, float ratio, float capacity_l,
                     int& last_value, uint32_t& last_color);
    void updateFuelLevel(float ratio);
    void updateWaterLevel(float ratio);

    void saveView();
    EngineView loadView();

    static constexpr uint32_t CONNECTION_TIMEOUT_MS  = 6000;
    static constexpr float    FUEL_CAPACITY_L        = 400.0f;
    static constexpr float    WATER_CAPACITY_L       = 100.0f;
    static constexpr float    EXHAUST_EMA_ALPHA       = 0.05f;
    static constexpr float    EXHAUST_TREND_THRESHOLD = 0.001f;

    // Tank arc color thresholds and colors — shared by the fuel and fresh water gauges
    static constexpr float    TANK_THRESHOLD_YELLOW = 0.25f;  // below this: yellow
    static constexpr float    TANK_THRESHOLD_RED    = 0.10f;  // below this: red
    static constexpr uint32_t TANK_COLOR_GREEN      = 0x28C850;
    static constexpr uint32_t TANK_COLOR_YELLOW     = 0xE6B400;
    static constexpr uint32_t TANK_COLOR_RED        = 0xDC2828;

    static constexpr const char* NVS_NAMESPACE = "engine";
    static constexpr const char* NVS_KEY_VIEW  = "view";

};
