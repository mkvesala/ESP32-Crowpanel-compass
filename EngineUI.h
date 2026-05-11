#pragma once

#include <Arduino.h>
#include <lvgl.h>
#include <Preferences.h>
#include "IScreenUI.h"
#include "ESPNowReceiver.h"

// === C L A S S  E N G I N E U I ===
//
// - Class EngineUI - responsible for updating LVGL UI elements on EngineScreen
// - Receives: ESPNowPacket<HALMETEngineDelta> (msg type 5) and ESPNowPacket<HALMETTankDelta> (msg type 6)
// - View cycle: EXHAUST (pakoputken lämpötila) → FUEL0 (polttoainesäiliö) → EXHAUST ...
// - Knob button press: cycles EngineView
// - Active view: persisted to NVS on onLeave()
// - EXHAUST: temperature °C, session min/max, EMA trend indicator
// - FUEL0: fuel arc 0-100, litres label (tank capacity 400 L)
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
        EXHAUST = 0,
        FUEL0   = 1,
        COUNT   = 2
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
    uint32_t _last_tank_millis   = 0;
    bool     _last_tank_connected = false;
    int      _last_arc_value     = -1;

    void showView(EngineView view);
    void showWaiting();
    void updateExhaustTemp(float temp_c);
    void updateFuelLevel(float ratio);

    void saveView();
    EngineView loadView();

    static constexpr uint32_t CONNECTION_TIMEOUT_MS  = 6000;
    static constexpr float    TANK_CAPACITY_L        = 400.0f;
    static constexpr float    EXHAUST_EMA_ALPHA       = 0.05f;
    static constexpr float    EXHAUST_TREND_THRESHOLD = 0.5f;

    static constexpr const char* NVS_NAMESPACE = "engine";
    static constexpr const char* NVS_KEY_VIEW  = "view";

};
