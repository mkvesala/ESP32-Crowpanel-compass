#pragma once

#include <Arduino.h>
#include <lvgl.h>
#include <Preferences.h>
#include "IScreenUI.h"
#include "ESPNowReceiver.h"

// === C L A S S  W E A T H E R U I ===
//
// - Class WeatherUI - responsible for updating LVGL UI elements on WeatherScreen
// - Receives: ESPNowPacket<WeatherDelta> via ESPNowReceiver (sent at ~2s intervals)
// - Knob press: cycles visible panel (TEMPERATURE → PRESSURE → HUMIDITY → ...)
// - Min/Max: tracked runtime only, not persisted to NVS
// - Active panel: persisted to NVS on onLeave()
// - Realizes: IScreenUI
// - Owned by: CrowPanelApplication

class WeatherUI : public IScreenUI {

public:

    explicit WeatherUI(ESPNowReceiver &receiver);

    void begin() override;
    lv_obj_t* getLvglScreen() const override;
    void update() override;
    void onButtonPress() override;
    void onLeave() override;

private:

    enum class WeatherPanel : uint8_t {
        TEMPERATURE = 0,
        PRESSURE    = 1,
        HUMIDITY    = 2,
        COUNT       = 3
    };

    ESPNowReceiver &_receiver;
    bool _initialized = false;

    // Active panel
    WeatherPanel _active_panel = WeatherPanel::TEMPERATURE;

    // Latest received values (NAN = not yet received)
    float _temperature_c = NAN;
    float _humidity_p = NAN;
    float _pressure_hpa = NAN;

     // exponential moving average
    float _temperature_ema = NAN;
    float _temperature_ema_ref = NAN;
    float _pressure_ema = NAN;
    float _pressure_ema_ref = NAN;
    float _humidity_ema = NAN;
    float _humidity_ema_ref = NAN;

    // Session min/max (NAN = not yet received, not saved to NVS)
    float _max_temp = NAN;
    float _min_temp = NAN;
    float _max_pressure = NAN;
    float _min_pressure = NAN;
    float _max_humidity = NAN;
    float _min_humidity = NAN;

    // Connection tracking (weather sender is separate from compass sender)
    uint32_t _last_data_millis = 0;
    bool _last_connected = false;

    // Panel visibility
    void showPanel(WeatherPanel panel);

    // Data update
    void updateTemperature(float temp_c);
    void updatePressure(float pres_hpa);
    void updateHumidity(float hum_p);
    void showWaiting();

    // NVS
    void savePanel();
    WeatherPanel loadPanel();

    static constexpr uint32_t CONNECTION_TIMEOUT_MS = 6000;
    static constexpr float TEMPERATURE_EMA_ALPHA = 0.05f;
    static constexpr float PRESSURE_EMA_ALPHA = 0.05f;
    static constexpr float HUMIDITY_EMA_ALPHA = 0.05f;
    static constexpr float TEMPERATURE_TREND_THRESHOLD = 0.05f;
    static constexpr float PRESSURE_TREND_THRESHOLD = 0.05f;
    static constexpr float HUMIDITY_TREND_THRESHOLD = 0.05f;

    static constexpr const char* NVS_NAMESPACE = "weather";
    static constexpr const char* NVS_KEY_PANEL = "panel";

};
