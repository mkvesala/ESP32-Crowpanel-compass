#pragma once

#include <Arduino.h>
#include "PCF8574.h"

// === C L A S S  R O T A R Y E N C O D E R ===
//
// - Class RotaryEncoder - responsible for managing CrowPanel 2.1" HMI rotary encoder
// - Init: encoder.begin();
// - FreeRTOS Tasks:
//  - encoderTask, "RotaryEnc", core 0 - reads knob rotation
//  - buttonTask, "RotaryBtn", core 0 - reads knob press
//  - (main loop runs on core 1)
// - Provides public API to:
//  - Get the knob rotation event data
//  - Get the knob button press event
//  - Get the task handles to core 0 tasks
// - Uses: PCF8574
// - Owned by: CrowPanelApplication
// - Hardware:
//  - Encoder A: GPIO42
//  - Encoder B: GPIO4
//  - Switch: PCF8574 P5 (I2C address 0x21)

class RotaryEncoder {

public:

    RotaryEncoder(PCF8574 &pcf);

    void begin();
    int8_t getDirection();
    bool getButtonPressed();

    TaskHandle_t getEncoderTaskHandle() const { return _encoderTaskHandle; }
    TaskHandle_t getButtonTaskHandle() const { return _buttonTaskHandle; }

private:

    static void encoderTask(void* param);
    static void buttonTask(void* param);
    void processEncoder();
    void processButton();

    // Encoder pins A and B
    static constexpr uint8_t ENCODER_A_PIN = 42;
    static constexpr uint8_t ENCODER_B_PIN = 4;

    // Debounce
    static constexpr uint32_t DEBOUNCE_MS = 50;

    // Rotary state
    volatile int8_t _direction;      // -1, 0, +1
    volatile int32_t _counter;       // Kumulatiivinen laskuri
    volatile uint8_t _lastStateCLK;  // Edellinen CLK-tila

    // Button state
    volatile bool _buttonPressed;    // Painallus havaittu (kulutettava)
    volatile bool _lastButtonState;  // Edellinen tila (debounce)
    volatile uint32_t _lastButtonTime; // Viimeinen muutos (debounce)

    // PCF8574 instance
    PCF8574 &_pcf8574;

    // FreeRTOS
    TaskHandle_t _encoderTaskHandle;
    TaskHandle_t _buttonTaskHandle;
    portMUX_TYPE _spinlock;

    bool _initialized;
    
};
