#pragma once

#include <Arduino.h>
#include <WiFi.h>
#include <esp_wifi.h>
#include <esp_now.h>
#include "HeadingData.h"

// === C L A S S  E S P N O W R E C E I V E R ===
//
// Class ESPNowReceiver - the "receiver", responsible for receiving data via ESP-NOW
// Owns:
// Uses:
// Init: receiver.begin(ssid, password) or without wifi receiver.begin()
// Loop: if (receiver.hasNewData()) HeadingData data = receiver.getData();

class ESPNowReceiver {
public:
    ESPNowReceiver();

    // Init
    bool begin(uint8_t channel = 1);
    bool begin(const char* ssid, const char* password, uint32_t timeout_ms = 10000);

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

    // Phase 2: two direction comms
    // bool sendCommand(uint8_t cmd, int16_t param1 = 0, int16_t param2 = 0);
    // bool registerPeer(const uint8_t* mac_addr);

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
};

