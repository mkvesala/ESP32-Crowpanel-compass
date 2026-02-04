#include "ESPNowReceiver.h"

// Static member initialization
portMUX_TYPE ESPNowReceiver::_mux = portMUX_INITIALIZER_UNLOCKED;
HeadingData ESPNowReceiver::_latest_data;
volatile bool ESPNowReceiver::_has_new_data = false;
volatile uint32_t ESPNowReceiver::_last_rx_millis = 0;
volatile uint32_t ESPNowReceiver::_packet_count = 0;
ESPNowReceiver* ESPNowReceiver::_instance = nullptr;

// Constructor
ESPNowReceiver::ESPNowReceiver()
    : _channel(1)
    , _packets_per_second(0.0f)
    , _last_stats_millis(0)
    , _last_packet_count(0)
    , _initialized(false)
{
    _instance = this;
}

bool ESPNowReceiver::begin(uint8_t channel) {
    if (_initialized) {
        return true;
    }

    _channel = channel;

    // WiFi STA mode but disconnected
    WiFi.mode(WIFI_STA);
    WiFi.disconnect();

    // Set channel manually
    esp_wifi_set_channel(channel, WIFI_SECOND_CHAN_NONE);

    // Init ESP-NOW
    if (esp_now_init() != ESP_OK) {
        return false;
    }

    // Register callback
    esp_now_register_recv_cb(onDataRecvStatic);

    _initialized = true;
    _last_stats_millis = millis();

    Serial.printf("MAC: %s  CH: %d\n", WiFi.macAddress().c_str(), _channel);

    return true;
}

bool ESPNowReceiver::begin(const char* ssid, const char* password, uint32_t timeout_ms) {
    if (_initialized) {
        return true;
    }

    // WiFi STA mode and connect
    WiFi.mode(WIFI_STA);
    WiFi.begin(ssid, password);

    uint32_t start = millis();
    while (WiFi.status() != WL_CONNECTED) {
        if (millis() - start > timeout_ms) {
            return false;
        }
        delay(100);
    }

    _channel = WiFi.channel();

    // Init ESP-NOW
    if (esp_now_init() != ESP_OK) {
        return false;
    }

    // Register callback
    esp_now_register_recv_cb(onDataRecvStatic);

    _initialized = true;
    _last_stats_millis = millis();

    Serial.printf("MAC: %s  CH: %d\n", WiFi.macAddress().c_str(), _channel);

    return true;
}

void ESPNowReceiver::onDataRecvStatic(const uint8_t* mac_addr, const uint8_t* data, int data_len) {
    if (_instance) {
        _instance->onDataRecv(mac_addr, data, data_len);
    }
}

void ESPNowReceiver::onDataRecv(const uint8_t* mac_addr, const uint8_t* data, int data_len) {
    // 
    if (data_len != sizeof(HeadingDelta)) {
        return;
    }

    // Copy and translate data
    HeadingDelta delta;
    memcpy(&delta, data, sizeof(HeadingDelta));

    HeadingData converted = convertDeltaToData(delta);

    // Save thread-safe
    portENTER_CRITICAL(&_mux);
    _latest_data = converted;
    _has_new_data = true;
    _last_rx_millis = millis();
    _packet_count++;
    portEXIT_CRITICAL(&_mux);
}

bool ESPNowReceiver::hasNewData() {
    bool result;
    portENTER_CRITICAL(&_mux);
    result = _has_new_data;
    portEXIT_CRITICAL(&_mux);
    return result;
}

HeadingData ESPNowReceiver::getData() {
    HeadingData data;
    portENTER_CRITICAL(&_mux);
    data = _latest_data;
    _has_new_data = false;
    portEXIT_CRITICAL(&_mux);
    return data;
}

bool ESPNowReceiver::isConnected(uint32_t timeout_ms) const {
    uint32_t last_rx;
    portENTER_CRITICAL(&_mux);
    last_rx = _last_rx_millis;
    portEXIT_CRITICAL(&_mux);

    if (last_rx == 0) {
        return false; 
    }

    return (millis() - last_rx) < timeout_ms;
}

void ESPNowReceiver::updateStats() {
    uint32_t now = millis();
    uint32_t elapsed = now - _last_stats_millis;

    if (elapsed >= 1000) {
        uint32_t count;
        portENTER_CRITICAL(&_mux);
        count = _packet_count;
        portEXIT_CRITICAL(&_mux);

        uint32_t delta_count = count - _last_packet_count;
        _packets_per_second = (float)delta_count * 1000.0f / (float)elapsed;

        _last_packet_count = count;
        _last_stats_millis = now;
    }
}
