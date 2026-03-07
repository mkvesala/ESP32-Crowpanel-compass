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
    portENTER_CRITICAL(&s_spinlock);
    result = s_has_new_data;
    portEXIT_CRITICAL(&s_spinlock);
    return result;
}

// Returns the latest data packet received via ESP-NOW
HeadingData ESPNowReceiver::getData() {
    HeadingData data;
    portENTER_CRITICAL(&s_spinlock);
    data = s_latest_data;
    s_has_new_data = false;
    portEXIT_CRITICAL(&s_spinlock);
    return data;
}

// Returns true if timeout not reached, otherwise false
bool ESPNowReceiver::isConnected(uint32_t timeout_ms) const {
    uint32_t last_rx;
    portENTER_CRITICAL(&s_spinlock);
    last_rx = s_last_rx_millis;
    portEXIT_CRITICAL(&s_spinlock);

    if (last_rx == 0) return false;

    return (millis() - last_rx) < timeout_ms;
}

// Update the PPS statistics
void ESPNowReceiver::updateStats() {
    uint32_t now = millis();
    uint32_t elapsed = now - _last_stats_millis;
    if (elapsed < 1000) return;

    uint32_t count;
    portENTER_CRITICAL(&s_spinlock);
    count = s_packet_count;
    portEXIT_CRITICAL(&s_spinlock);

    uint32_t delta_count = count - _last_packet_count;
    _packets_per_second = (float)delta_count * 1000.0f / (float)elapsed;

    _last_packet_count = count;
    _last_stats_millis = now;
}

// Send attitude leveling command to ESP-NOW as broadcast
bool ESPNowReceiver::sendLevelCommand() {
    // Add broadcast peer if not exists
    if (!esp_now_is_peer_exist(BROADCAST_ADDR)) {
        esp_now_peer_info_t peerInfo = {};
        memcpy(peerInfo.peer_addr, BROADCAST_ADDR, 6);
        peerInfo.channel = _channel;
        peerInfo.encrypt = false;

        if (esp_now_add_peer(&peerInfo) != ESP_OK) return false;
    }

    // Build framed packet
    ESPNowPacket<LevelCommand> pkt;
    initHeader(pkt.hdr, ESPNowMsgType::LEVEL_COMMAND, sizeof(LevelCommand));
    memcpy(pkt.payload.magic, "LVLC", 4);
    memset(pkt.payload.reserved, 0, 4);

    // Clear previous response
    portENTER_CRITICAL(&s_spinlock);
    s_level_response_received = false;
    s_level_response_success  = false;
    portEXIT_CRITICAL(&s_spinlock);

    esp_err_t result = esp_now_send(BROADCAST_ADDR, (uint8_t*)&pkt, sizeof(pkt));

    return (result == ESP_OK);
}

// Check for attitude leveling command response
bool ESPNowReceiver::hasLevelResponse() const {
    bool result;
    portENTER_CRITICAL(&s_spinlock);
    result = s_level_response_received;
    portEXIT_CRITICAL(&s_spinlock);
    return result;
}

// Check for success of the attitude leveling command
bool ESPNowReceiver::getLevelResult() {
    bool success;
    portENTER_CRITICAL(&s_spinlock);
    success = s_level_response_success;
    s_level_response_received = false; 
    portEXIT_CRITICAL(&s_spinlock);
    return success;
}

// === P R I V A T E ===

// Callback for data receive of ESP-NOW
void ESPNowReceiver::onDataRecv(const uint8_t* mac_addr, const uint8_t* data, int data_len) {

    // Minimum frame size check
    if (data_len < (int)sizeof(ESPNowHeader)) return;

    // Extract and validate header
    ESPNowHeader hdr;
    memcpy(&hdr, data, sizeof(ESPNowHeader));

    if (hdr.magic != ESPNOW_MAGIC) return;

    // Frame integrity: buffer must hold header + declared payload
    if (data_len < (int)(sizeof(ESPNowHeader) + hdr.payload_len)) return;

    const uint8_t* payload = data + sizeof(ESPNowHeader);

    // Actual handling of the payload content, based on the message type
    switch (static_cast<ESPNowMsgType>(hdr.msg_type)) {

        case ESPNowMsgType::HEADING_DELTA: {
            if (hdr.payload_len != sizeof(HeadingDelta)) return;
            HeadingDelta delta;
            memcpy(&delta, payload, sizeof(HeadingDelta));
            HeadingData converted = convertDeltaToData(delta);
            portENTER_CRITICAL(&s_spinlock);
            s_latest_data    = converted;
            s_has_new_data   = true;
            s_last_rx_millis = millis();
            s_packet_count++;
            portEXIT_CRITICAL(&s_spinlock);
            break;
        }

        case ESPNowMsgType::LEVEL_RESPONSE: {
            if (hdr.payload_len != sizeof(LevelResponse)) return;
            LevelResponse resp;
            memcpy(&resp, payload, sizeof(LevelResponse));
            portENTER_CRITICAL(&s_spinlock);
            s_level_response_received = true;
            s_level_response_success  = (resp.success == 1);
            portEXIT_CRITICAL(&s_spinlock);
            break;
        }

        case ESPNowMsgType::WEATHER_DELTA: {
            if (hdr.payload_len != sizeof(WeatherDelta)) return;
            WeatherDelta weather;
            memcpy(&weather, payload, sizeof(WeatherDelta));
            portENTER_CRITICAL(&s_spinlock);
            s_latest_weather  = weather;
            s_has_new_weather = true;
            portEXIT_CRITICAL(&s_spinlock);
            break;
        }

        case ESPNowMsgType::BATTERY_DELTA: {
            if (hdr.payload_len != sizeof(BatteryDelta)) return;
            BatteryDelta battery;
            memcpy(&battery, payload, sizeof(BatteryDelta));
            portENTER_CRITICAL(&s_spinlock);
            s_latest_battery  = battery;
            s_has_new_battery = true;
            portEXIT_CRITICAL(&s_spinlock);
            break;
        }

        default:
            // Unknown msg_type — ignore
            break;
    }
}

// Returns true if new weather data packet available, otherwise false
bool ESPNowReceiver::hasNewWeatherData() const {
    bool result;
    portENTER_CRITICAL(&s_spinlock);
    result = s_has_new_weather;
    portEXIT_CRITICAL(&s_spinlock);
    return result;
}

// Returns the latest weather data packet received via ESP-NOW
WeatherDelta ESPNowReceiver::getWeatherData() {
    WeatherDelta data;
    portENTER_CRITICAL(&s_spinlock);
    data = s_latest_weather;
    s_has_new_weather = false;
    portEXIT_CRITICAL(&s_spinlock);
    return data;
}

// Returns true if new battery data packet available, otherwise false
bool ESPNowReceiver::hasNewBatteryData() const {
    bool result;
    portENTER_CRITICAL(&s_spinlock);
    result = s_has_new_battery;
    portEXIT_CRITICAL(&s_spinlock);
    return result;
}

// Returns the latest battery data packet received via ESP-NOW
BatteryDelta ESPNowReceiver::getBatteryData() {
    BatteryDelta data;
    portENTER_CRITICAL(&s_spinlock);
    data = s_latest_battery;
    s_has_new_battery = false;
    portEXIT_CRITICAL(&s_spinlock);
    return data;
}
