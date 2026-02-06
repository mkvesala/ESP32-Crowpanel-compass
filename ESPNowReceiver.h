#pragma once

#include <Arduino.h>
#include <WiFi.h>
#include <esp_wifi.h>
#include <esp_now.h>
#include "HeadingData.h"

// === C L A S S  E S P N O W R E C E I V E R ===
//
// Class ESPNowReceiver - receives heading data and sends level commands via ESP-NOW
//
// Init: receiver.begin(channel)
// Loop: if (receiver.hasNewData()) HeadingData data = receiver.getData();
// Level: receiver.sendLevelCommand(); if (receiver.hasLevelResponse()) bool ok = receiver.getLevelResult();

class ESPNowReceiver {
public:
    ESPNowReceiver();

    // Init (ESP-NOW only, no WiFi connection)
    bool begin(uint8_t channel = 1);

    // Check for newly received data
    bool hasNewData();

    // Get the latest data - thread safe
    HeadingData getData();

    // Check the connected state
    bool isConnected(uint32_t timeout_ms = 500) const;

    // Get received packets per second
    float getPacketsPerSecond() const { return _packets_per_second; }

    // Get wifi channel
    uint8_t getChannel() const { return _channel; }

    //Get MAC address
    String getMacAddress() const { return WiFi.macAddress(); }

    // Update statistics - called from main loop
    void updateStats();

    // Level command (bidirectional ESP-NOW)
    bool sendLevelCommand();
    bool hasLevelResponse();
    bool getLevelResult();

private:

    // Singleton callback - ESP-NOW requires static callback
    static void onDataRecvStatic(const uint8_t* mac_addr, const uint8_t* data, int data_len);
    void onDataRecv(const uint8_t* mac_addr, const uint8_t* data, int data_len);

    // Thread-safe data storage
    static portMUX_TYPE _mux;
    static HeadingData _latest_data;
    static volatile bool _has_new_data;
    static volatile uint32_t _last_rx_millis;
    static volatile uint32_t _packet_count;

    // Singleton instance pointer required by ESP-NOW
    static ESPNowReceiver* _instance;

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

