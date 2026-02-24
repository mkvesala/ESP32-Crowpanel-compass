#include "ESPNowReceiver.h"

// === P U B L I C ===

// Constructor
ESPNowReceiver::ESPNowReceiver() {}

// Initialization
bool ESPNowReceiver::begin(uint8_t channel) {
    if (_initialized) return true;

    _channel = channel;

    // Init and disconnect WiFi
    WiFi.mode(WIFI_STA);
    WiFi.disconnect();

    // Set channel manually
    esp_wifi_set_channel(channel, WIFI_SECOND_CHAN_NONE);

    // Init ESP-NOW
    if (esp_now_init() != ESP_OK) return false;

    // Register callback
    esp_now_register_recv_cb(onDataRecv);

    _initialized = true;
    _last_stats_millis = millis();

    return true;
}

// Returns true if new packet available, otherwise false
bool ESPNowReceiver::hasNewData() const {
    bool result;
    portENTER_CRITICAL(&s_mux);
    result = s_has_new_data;
    portEXIT_CRITICAL(&s_mux);
    return result;
}

// Returns the latest data packet received via ESP-NOW
HeadingData ESPNowReceiver::getData() {
    HeadingData data;
    portENTER_CRITICAL(&s_mux);
    data = s_latest_data;
    s_has_new_data = false;
    portEXIT_CRITICAL(&s_mux);
    return data;
}

// Returns true if timeout not reached, otherwise false
bool ESPNowReceiver::isConnected(uint32_t timeout_ms) const {
    uint32_t last_rx;
    portENTER_CRITICAL(&s_mux);
    last_rx = s_last_rx_millis;
    portEXIT_CRITICAL(&s_mux);

    if (last_rx == 0) return false;

    return (millis() - last_rx) < timeout_ms;
}

// Update the PPS statistics
void ESPNowReceiver::updateStats() {
    uint32_t now = millis();
    uint32_t elapsed = now - _last_stats_millis;

    if (elapsed >= 1000) {
        uint32_t count;
        portENTER_CRITICAL(&s_mux);
        count = s_packet_count;
        portEXIT_CRITICAL(&s_mux);

        uint32_t delta_count = count - _last_packet_count;
        _packets_per_second = (float)delta_count * 1000.0f / (float)elapsed;

        _last_packet_count = count;
        _last_stats_millis = now;
    }
}

// Send attitude leveling command to ESP-NOW as broadcast
bool ESPNowReceiver::sendLevelCommand() {
    // Add broadcast peer if not exists
    if (!esp_now_is_peer_exist(BROADCAST_ADDR)) {
        esp_now_peer_info_t peerInfo = {};
        memcpy(peerInfo.peer_addr, BROADCAST_ADDR, 6);
        peerInfo.channel = _channel;
        peerInfo.encrypt = false;

        if (esp_now_add_peer(&peerInfo) != ESP_OK) {
            return false;
        }
    }

    // Build and send command
    LevelCommand cmd;
    memcpy(cmd.magic, "LVLC", 4);
    memset(cmd.reserved, 0, 4);

    // Clear previous response
    portENTER_CRITICAL(&s_mux);
    s_level_response_received = false;
    s_level_response_success = false;
    portEXIT_CRITICAL(&s_mux);

    esp_err_t result = esp_now_send(BROADCAST_ADDR, (uint8_t*)&cmd, sizeof(cmd));

    return (result == ESP_OK);
}

// Check for attitude leveling command response
bool ESPNowReceiver::hasLevelResponse() const {
    bool result;
    portENTER_CRITICAL(&s_mux);
    result = s_level_response_received;
    portEXIT_CRITICAL(&s_mux);
    return result;
}

// Check for success of the attitude leveling command
bool ESPNowReceiver::getLevelResult() {
    bool success;
    portENTER_CRITICAL(&s_mux);
    success = s_level_response_success;
    s_level_response_received = false; 
    portEXIT_CRITICAL(&s_mux);
    return success;
}

// === P R I V A T E ===

// Callback for data receive of ESP-NOW
void ESPNowReceiver::onDataRecv(const uint8_t* mac_addr, const uint8_t* data, int data_len) {
    // Check for HeadingDelta (compass data)
    if (data_len == sizeof(HeadingDelta)) {
        HeadingDelta delta;
        memcpy(&delta, data, sizeof(HeadingDelta));

        HeadingData converted = convertDeltaToData(delta);

        portENTER_CRITICAL(&s_mux);
        s_latest_data = converted;
        s_has_new_data = true;
        s_last_rx_millis = millis();
        s_packet_count++;
        portEXIT_CRITICAL(&s_mux);
    }
    // Check for LevelResponse
    else if (data_len == sizeof(LevelResponse)) {
        LevelResponse resp;
        memcpy(&resp, data, sizeof(LevelResponse));

        if (memcmp(resp.magic, "LVLR", 4) == 0) {
            portENTER_CRITICAL(&s_mux);
            s_level_response_received = true;
            s_level_response_success = (resp.success == 1);
            portEXIT_CRITICAL(&s_mux);
        }
    }
}
