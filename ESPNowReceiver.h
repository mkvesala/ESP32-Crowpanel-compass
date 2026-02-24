#pragma once

#include <Arduino.h>
#include <WiFi.h>
#include <esp_wifi.h>
#include <esp_now.h>
#include "espnow_protocol.h"

// === C L A S S  E S P N O W R E C E I V E R ===
//
// - Class ESPNowReceiver - responsible for ESP-NOW inbound/outbound communication
//
// - Provides public API to manage inbound compass data and outbound attitude leveling command
// - Init: receiver.begin(channel)
// - Loop: if (receiver.hasNewData()) HeadingData data = receiver.getData();
// - Level: receiver.sendLevelCommand(); if (receiver.hasLevelResponse()) bool ok = receiver.getLevelResult();
// - Owned by: CrowPanelApplication

class ESPNowReceiver {

public:
    
    ESPNowReceiver();

    bool begin(uint8_t channel = 1);
    bool hasNewData();
    HeadingData getData();
    bool isConnected(uint32_t timeout_ms = 500) const;
    void updateStats();
    bool sendLevelCommand();
    bool hasLevelResponse();
    bool getLevelResult();

    float getPacketsPerSecond() const { return _packets_per_second; }

private:

    // Static callback for ESP-NOW
    static void onDataRecv(const uint8_t* mac_addr, const uint8_t* data, int data_len);

    // Static data storage for ESP-NOW
    inline static portMUX_TYPE s_mux = portMUX_INITIALIZER_UNLOCKED;
    inline static HeadingData s_latest_data = {};
    inline static volatile bool s_has_new_data = false;
    inline static volatile uint32_t s_last_rx_millis = 0;
    inline static volatile uint32_t s_packet_count = 0;

    // Static variables for level response handling
    inline static volatile bool s_level_response_received = false;
    inline static volatile bool s_level_response_success = false;
    
    // ESP-NOW mac address for broadcasting
    inline static constexpr uint8_t BROADCAST_ADDR[6] = {0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF};

    // Instance data
    uint8_t _channel;
    float _packets_per_second;
    uint32_t _last_stats_millis;
    uint32_t _last_packet_count;
    bool _initialized;
    
};

