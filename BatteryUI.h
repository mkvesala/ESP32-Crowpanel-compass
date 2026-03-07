#pragma once

#include <Arduino.h>
#include <lvgl.h>
#include <Preferences.h>
#include "IScreenUI.h"
#include "ESPNowReceiver.h"

// === C L A S S  B A T T E R Y U I ===
//
// - Class BatteryUI - responsible for updating LVGL UI elements on BatteryScreen
// - Receives: ESPNowPacket<BatteryDelta> via ESPNowReceiver (sent at ~2s intervals)
// - Knob press: cycles visible panel (HOUSE V → HOUSE A → HOUSE SOC → STARTER V → ...)
// - Min/Max: tracked runtime only, not persisted to NVS
// - Active panel: persisted to NVS on onLeave()
// - Realizes: IScreenUI
// - Owned by: CrowPanelApplication

class BatteryUI : public IScreenUI {

public:

    explicit BatteryUI(ESPNowReceiver &receiver);

    void begin() override;
    lv_obj_t* getLvglScreen() const override;
    void update() override;
    void onButtonPress() override;
    void onLeave() override;

private:

    enum class BatteryPanel : uint8_t {
        HOUSE_V = 0,
        HOUSE_A = 1,
        HOUSE_SOC = 2,
        START_V = 3,
        COUNT = 4
    };

    ESPNowReceiver &_receiver;
    bool _initialized = false;

    // Active panel
    BatteryPanel _active_panel = BatteryPanel::HOUSE_V;

    // Latest received values (NAN = not yet received)
    float _house_voltage = NAN;
    float _house_current = NAN;
    float _house_soc = NAN;
    float _start_voltage = NAN;

     // exponential moving average
    float _house_v_ema = NAN;
    float _house_v_ema_ref = NAN;
    float _house_a_ema = NAN;
    float _house_a_ema_ref = NAN;
    float _house_soc_ema = NAN;
    float _house_soc_ema_ref = NAN;
    float _start_v_ema = NAN;
    float _start_v_ema_ref = NAN;

    // Session min/max (NAN = not yet received, not saved to NVS)
    float _max_house_v = NAN;
    float _min_house_v = NAN;
    float _max_house_a = NAN;
    float _min_house_a = NAN;
    float _max_house_soc = NAN;
    float _min_house_soc = NAN;
    float _max_start_v = NAN;
    float _min_start_v = NAN;

    // Connection tracking (battery sender is separate from compass sender)
    uint32_t _last_data_millis = 0;
    bool _last_connected = false;

    // Panel visibility
    void showPanel(BatteryPanel panel);

    // Data update
    void updateHouseVoltage(float house_v);
    void updateHouseCurrent(float house_a);
    void updateHouseSoc(float house_soc);
    void updateStartVoltage(float start_v);
    void showWaiting();

    // NVS
    void savePanel();
    BatteryPanel loadPanel();

    static constexpr uint32_t CONNECTION_TIMEOUT_MS = 6000;
    static constexpr float VOLTAGE_EMA_ALPHA = 0.05f;
    static constexpr float CURRENT_EMA_ALPHA = 0.05f;
    static constexpr float SOC_EMA_ALPHA = 0.05f;
    static constexpr float VOLTAGE_TREND_THRESHOLD = 0.05f;
    static constexpr float CURRENT_TREND_THRESHOLD = 0.05f;
    static constexpr float SOC_TREND_THRESHOLD = 0.05f;

    static constexpr const char* NVS_NAMESPACE = "battery";
    static constexpr const char* NVS_KEY_PANEL = "panel";

};
