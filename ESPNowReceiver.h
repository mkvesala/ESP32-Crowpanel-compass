#pragma once

#include <Arduino.h>
#include <WiFi.h>
#include <esp_wifi.h>
#include <esp_now.h>
#include "HeadingData.h"

/**
 * @brief ESP-NOW vastaanottoluokka
 *
 * Vastaanottaa HeadingDelta broadcast-viestejä ja muuntaa ne
 * sisäiseen HeadingData formaattiin.
 *
 * Suunniteltu laajennettavaksi kaksisuuntaiseen kommunikaatioon (Phase 2).
 *
 * Käyttö:
 *   ESPNowReceiver receiver;
 *   receiver.begin("your_ssid", "your_password");  // Tai begin() ilman WiFi:tä
 *
 *   // loop():ssa
 *   if (receiver.hasNewData()) {
 *       HeadingData data = receiver.getData();
 *       // käytä dataa
 *   }
 */
class ESPNowReceiver {
public:
    ESPNowReceiver();

    /**
     * @brief Alusta ESP-NOW ilman WiFi-yhteyttä
     *
     * HUOM: Vaatii että lähettäjä ja vastaanotin ovat samalla kanavalla.
     * Oletuskanava on 1, mutta voi vaihdella.
     *
     * @param channel WiFi-kanava (1-13), oletus 1
     * @return true jos alustus onnistui
     */
    bool begin(uint8_t channel = 1);

    /**
     * @brief Alusta ESP-NOW WiFi-yhteydellä (suositeltu)
     *
     * Yhdistää WiFi-verkkoon varmistaen saman kanavan lähettäjän kanssa.
     *
     * @param ssid WiFi-verkon nimi
     * @param password WiFi-salasana
     * @param timeout_ms Yhteyden muodostuksen timeout
     * @return true jos alustus onnistui
     */
    bool begin(const char* ssid, const char* password, uint32_t timeout_ms = 10000);

    /**
     * @brief Tarkista onko uutta dataa saatavilla
     */
    bool hasNewData();

    /**
     * @brief Hae viimeisin data ja nollaa uusi-data -lippu
     *
     * Thread-safe, käyttää kritistä sektiota.
     */
    HeadingData getData();

    /**
     * @brief Tarkista onko yhteys aktiivinen
     *
     * @param timeout_ms Timeout millisekunneissa (oletus 500ms)
     * @return true jos dataa vastaanotettu timeout-ajan sisällä
     */
    bool isConnected(uint32_t timeout_ms = 500) const;

    /**
     * @brief Hae vastaanotettujen pakettien määrä sekunnissa
     */
    float getPacketsPerSecond() const { return _packets_per_second; }

    /**
     * @brief Hae WiFi-kanava
     */
    uint8_t getChannel() const { return _channel; }

    /**
     * @brief Hae oma MAC-osoite
     */
    String getMacAddress() const { return WiFi.macAddress(); }

    /**
     * @brief Päivitä pakettilaskuri (kutsu loop():ssa)
     */
    void updateStats();

    // Phase 2: Kaksisuuntainen kommunikaatio
    // bool sendCommand(uint8_t cmd, int16_t param1 = 0, int16_t param2 = 0);
    // bool registerPeer(const uint8_t* mac_addr);

private:
    // Singleton callback - ESP-NOW vaatii static callbackin
    static void onDataRecvStatic(const uint8_t* mac_addr, const uint8_t* data, int data_len);
    void onDataRecv(const uint8_t* mac_addr, const uint8_t* data, int data_len);

    // Thread-safe data storage
    static portMUX_TYPE _mux;
    static HeadingData _latest_data;
    static volatile bool _has_new_data;
    static volatile uint32_t _last_rx_millis;
    static volatile uint32_t _packet_count;

    // Singleton instance pointer (ESP-NOW callback tarvitsee)
    static ESPNowReceiver* _instance;

    // Instance data
    uint8_t _channel;
    float _packets_per_second;
    uint32_t _last_stats_millis;
    uint32_t _last_packet_count;
    bool _initialized;
};

