#pragma once

#include <Arduino.h>
#include <WiFi.h>
#include <esp_wifi.h>
#include <esp_now.h>
#include "espnow_protocol.h"

using namespace ESPNow;

// === C L A S S  E S P N O W R E C E I V E R ===
//
// - Class ESPNowReceiver - responsible for ESP-NOW inbound/outbound communication
//
// - Provides public API to manage incoming instrument data and outbound attitude leveling command
// - Receives: HEADING_DELTA (compass/attitude), WEATHER_DELTA (weather sensor), LEVEL_RESPONSE
// - Sends: LEVEL_COMMAND (broadcast)
// - Init: _receiver.begin(channel)
// - Owned by: CrowPanelApplication

class ESPNowReceiver {

public:
    
    ESPNowReceiver();

    bool begin(uint8_t channel = 1);
    bool hasNewData() const;
    HeadingData getData();
    bool isConnected(uint32_t timeout_ms = 500) const;
    void updateStats();
    bool sendLevelCommand();
    bool hasLevelResponse() const;
    bool getLevelResult();
    bool hasNewWeatherData() const;
    bool hasNewBatteryData() const;
    WeatherDelta getWeatherData();
    BatteryDelta getBatteryData();

    float getPacketsPerSecond() const { return _packets_per_second; }

private:

    // Static callback for ESP-NOW
    static void onDataRecv(const uint8_t* mac_addr, const uint8_t* data, int data_len);

    // Static data storage for ESP-NOW
    inline static portMUX_TYPE s_spinlock = portMUX_INITIALIZER_UNLOCKED;
    inline static HeadingData s_latest_data = {};
    inline static volatile bool s_has_new_data = false;
    inline static volatile uint32_t s_last_rx_millis = 0;
    inline static volatile uint32_t s_packet_count = 0;

    // Static variables for level response handling
    inline static volatile bool s_level_response_received = false;
    inline static volatile bool s_level_response_success = false;

    // Static variables for weather data handling
    inline static WeatherDelta s_latest_weather = {};
    inline static volatile bool s_has_new_weather = false;

    // Static variables for battery data handling
    inline static BatteryDelta s_latest_battery = {};
    inline static volatile bool s_has_new_battery = false;
    
    // ESP-NOW mac address for broadcasting
    inline static constexpr uint8_t BROADCAST_ADDR[6] = {0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF};

    // Instance data
    uint8_t _channel = 1;
    float _packets_per_second = 0.0f;
    uint32_t _last_stats_millis = 0;
    uint32_t _last_packet_count = 0;
    bool _initialized = false;
    
};

