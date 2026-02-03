#pragma once

#include <Arduino.h>
#include "PCF8574.h"

/**
 * @brief Rotary encoder -käsittelijä CrowPanel 2.1" HMI:lle
 *
 * Lukee rotary encoderin tilaa ja painiketta FreeRTOS-taskissa ja tarjoaa
 * suuntatiedon sekä painallukset pollattavaksi pääsilmukasta.
 *
 * Pinout (CrowPanel 2.1"):
 * - Encoder A: GPIO 42
 * - Encoder B: GPIO 4
 * - Switch: PCF8574 P5 (I2C osoite 0x21)
 */
class RotaryEncoder {
public:
    RotaryEncoder();

    /**
     * @brief Alusta encoder ja käynnistä lukutaski
     *
     * @param pcf Viittaus PCF8574-instanssiin (main.ino:sta)
     */
    void begin(PCF8574& pcf);

    /**
     * @brief Hae ja nollaa suuntatieto
     *
     * @return int8_t -1 = CCW (vastapäivään), 0 = ei muutosta, +1 = CW (myötäpäivään)
     */
    int8_t getDirection();

    /**
     * @brief Hae kumulatiivinen laskuri (debuggausta varten)
     */
    int32_t getCounter() const { return _counter; }

    /**
     * @brief Tarkista onko painiketta painettu
     *
     * Palauttaa true kerran per painallus (kuluttaa eventin).
     * Sisältää debounce-logiikan.
     *
     * @return true jos painallus havaittu
     */
    bool getButtonPressed();

private:
    static void encoderTask(void* param);
    static void buttonTask(void* param);
    void processEncoder();
    void processButton();

    // Pinnit
    static constexpr uint8_t ENCODER_A_PIN = 42;
    static constexpr uint8_t ENCODER_B_PIN = 4;

    // Debounce
    static constexpr uint32_t DEBOUNCE_MS = 50;

    // Encoder tila
    volatile int8_t _direction;      // -1, 0, +1
    volatile int32_t _counter;       // Kumulatiivinen laskuri
    volatile uint8_t _lastStateCLK;  // Edellinen CLK-tila

    // Painikkeen tila
    volatile bool _buttonPressed;    // Painallus havaittu (kulutettava)
    volatile bool _lastButtonState;  // Edellinen tila (debounce)
    volatile uint32_t _lastButtonTime; // Viimeinen muutos (debounce)

    // PCF8574 viittaus (ei omista)
    PCF8574* _pcf8574;

    // FreeRTOS
    TaskHandle_t _encoderTaskHandle;
    TaskHandle_t _buttonTaskHandle;
    portMUX_TYPE _spinlock;

    bool _initialized;
};
