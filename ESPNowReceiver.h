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

    // Thread-safe static data storage for ESP-NOW
    static portMUX_TYPE _mux;
    static HeadingData _latest_data;
    static volatile bool _has_new_data;
    static volatile uint32_t _last_rx_millis;
    static volatile uint32_t _packet_count;

    // Instance data
    uint8_t _channel;
    float _packets_per_second;
    uint32_t _last_stats_millis;
    uint32_t _last_packet_count;
    bool _initialized;

    // Level response handling
    static volatile bool _level_response_received;
    static volatile bool _level_response_success;
    
};

