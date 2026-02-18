#include "RotaryEncoder.h"

// === S T A T I C ===

// Static pointer for FreeRTOS tasks
static RotaryEncoder* s_instance = nullptr;

// === P U B L I C ===

// Constructor
RotaryEncoder::RotaryEncoder(PCF8574 &pcf)
    : _pcf8574(pcf)
    , _direction(0)
    , _counter(0)
    , _lastStateCLK(0)
    , _buttonPressed(false)
    , _lastButtonState(true)
    , _lastButtonTime(0)
    , _encoderTaskHandle(nullptr)
    , _buttonTaskHandle(nullptr)
    , _spinlock(portMUX_INITIALIZER_UNLOCKED)
    , _initialized(false) {}

// Initialize
void RotaryEncoder::begin() {
    if (_initialized) return;

    // Static pointer to 'this' 
    s_instance = this;

    // P5 (knob button) init in main.ino before pcf8574.begin()
    // PCF8574 library does not allow pinMode() calls after begin()

    // Init encoder pins
    pinMode(ENCODER_A_PIN, INPUT);
    pinMode(ENCODER_B_PIN, INPUT);

    // Initial states
    _lastStateCLK = digitalRead(ENCODER_A_PIN);
    _lastButtonState = _pcf8574.digitalRead(P5, true);

    // FreeRTOS tasks on core 0 (core 1: main loop)
    xTaskCreatePinnedToCore(
        encoderTask,         // Callback
        "RotaryEnc",         // Name
        2048,                // Stack size
        this,                // Param (this)
        1,                   // Priority
        &_encoderTaskHandle, // Handle
        0                    // Core
    );

    xTaskCreatePinnedToCore(
        buttonTask,          // Callback
        "RotaryBtn",         // Name
        2048,                // Stack size
        this,                // Param (this)
        1,                   // Priority
        &_buttonTaskHandle,  // Handle
        0                    // Core
    );

    _initialized = true;
}

// Return rotation direction -1 = CCW, 0 = neutral, +1 = CW
int8_t RotaryEncoder::getDirection() {
    int8_t dir;
    portENTER_CRITICAL(&_spinlock);
    dir = _direction;
    _direction = 0;
    portEXIT_CRITICAL(&_spinlock);
    return dir;
}

// Return true when knob button pressed
bool RotaryEncoder::getButtonPressed() {
    bool pressed;
    portENTER_CRITICAL(&_spinlock);
    pressed = _buttonPressed;
    _buttonPressed = false;
    portEXIT_CRITICAL(&_spinlock);
    return pressed;
}

// Rotary knob task, 500 Hz
void RotaryEncoder::encoderTask(void* param) {
    RotaryEncoder* self = static_cast<RotaryEncoder*>(param);

    while (true) {
        self->processEncoder();
        vTaskDelay(pdMS_TO_TICKS(2));  // 2ms polling
    }
}

// Knob button task, 200 Hz
void RotaryEncoder::buttonTask(void* param) {
    RotaryEncoder* self = static_cast<RotaryEncoder*>(param);

    while (true) {
        self->processButton();
        vTaskDelay(pdMS_TO_TICKS(5));  // 5ms polling
    }
}

// Process the rotary knob rotation
void RotaryEncoder::processEncoder() {
    // Read current state of CLK
    uint8_t currentStateCLK = digitalRead(ENCODER_A_PIN);

    // Capture rising edge
    if (currentStateCLK != _lastStateCLK && currentStateCLK == 1) {
        // Read DT to determine direction
        uint8_t stateDT = digitalRead(ENCODER_B_PIN);

        portENTER_CRITICAL(&_spinlock);
        if (stateDT != currentStateCLK) {
            // CCW
            _direction = -1;
            _counter--;
        } else {
            // CW
            _direction = 1;
            _counter++;
        }
        portEXIT_CRITICAL(&_spinlock);
    }

    _lastStateCLK = currentStateCLK;
}

// Process the knob button press
void RotaryEncoder::processButton() {

    // Read knob button state (LOW = pressed, INPUT_PULLUP)
    bool currentState = _pcf8574.digitalRead(P5, true);
    uint32_t now = millis();

    // Debounce, react only if timer due
    if (currentState != _lastButtonState) {
        if (now - _lastButtonTime >= DEBOUNCE_MS) {
            if (currentState == LOW && _lastButtonState == HIGH) {
                // Captured falling edge
                portENTER_CRITICAL(&_spinlock);
                _buttonPressed = true;
                portEXIT_CRITICAL(&_spinlock);
            }
            _lastButtonState = currentState;
            _lastButtonTime = now;
        }
    }
}
