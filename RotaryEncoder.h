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

    explicit RotaryEncoder(PCF8574 &pcf);

    void begin();
    int8_t getDirection();
    bool getButtonPressed();

    TaskHandle_t getEncoderTaskHandle() const { return _encoder_task_handle; }
    TaskHandle_t getButtonTaskHandle() const { return _button_task_handle; }

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
    volatile int8_t _direction = 0;
    volatile int32_t _counter = 0;
    volatile uint8_t _last_state_clk = 0;

    // Button state
    volatile bool _button_pressed = false;
    volatile bool _last_button_state;
    volatile uint32_t _last_button_time = 0;

    // PCF8574 instance
    PCF8574 &_pcf8574;

    // Static pointer for FreeRTOS tasks
    inline static RotaryEncoder *s_instance = nullptr;

    // FreeRTOS
    TaskHandle_t _encoder_task_handle = nullptr;
    TaskHandle_t _button_task_handle = nullptr;
    portMUX_TYPE _spinlock;

    bool _initialized = false;
    
};
