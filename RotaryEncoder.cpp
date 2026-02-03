#include "RotaryEncoder.h"

// Staattinen viittaus instanssiin taskia varten
static RotaryEncoder* s_instance = nullptr;

RotaryEncoder::RotaryEncoder()
    : _direction(0)
    , _counter(0)
    , _lastStateCLK(0)
    , _buttonPressed(false)
    , _lastButtonState(true)  // HIGH = ei painettu (INPUT_PULLUP)
    , _lastButtonTime(0)
    , _pcf8574(nullptr)
    , _encoderTaskHandle(nullptr)
    , _buttonTaskHandle(nullptr)
    , _spinlock(portMUX_INITIALIZER_UNLOCKED)
    , _initialized(false)
{
}

void RotaryEncoder::begin(PCF8574& pcf) {
    if (_initialized) return;

    // Tallenna instanssi staattiseen muuttujaan
    s_instance = this;

    // Tallenna PCF8574 viittaus
    _pcf8574 = &pcf;

    // Alusta P5 input pullup (painike)
    _pcf8574->pinMode(P5, INPUT_PULLUP);

    // Alusta encoder pinnit
    pinMode(ENCODER_A_PIN, INPUT);
    pinMode(ENCODER_B_PIN, INPUT);

    // Lue alkutilat
    _lastStateCLK = digitalRead(ENCODER_A_PIN);
    _lastButtonState = _pcf8574->digitalRead(P5, true);

    // Käynnistä FreeRTOS-taskit core 0:lla (jättää core 1 pääloopille)
    xTaskCreatePinnedToCore(
        encoderTask,         // Funktio
        "RotaryEnc",         // Nimi
        2048,                // Stack size
        this,                // Parametri (this-osoitin)
        1,                   // Prioriteetti
        &_encoderTaskHandle, // Handle
        0                    // Core 0
    );

    xTaskCreatePinnedToCore(
        buttonTask,          // Funktio
        "RotaryBtn",         // Nimi
        2048,                // Stack size
        this,                // Parametri (this-osoitin)
        1,                   // Prioriteetti
        &_buttonTaskHandle,  // Handle
        0                    // Core 0
    );

    _initialized = true;
    Serial.println("[RotaryEncoder] Initialized with button support");
}

int8_t RotaryEncoder::getDirection() {
    int8_t dir;
    portENTER_CRITICAL(&_spinlock);
    dir = _direction;
    _direction = 0;  // Nollaa luettu suunta
    portEXIT_CRITICAL(&_spinlock);
    return dir;
}

bool RotaryEncoder::getButtonPressed() {
    bool pressed;
    portENTER_CRITICAL(&_spinlock);
    pressed = _buttonPressed;
    _buttonPressed = false;  // Kuluta event
    portEXIT_CRITICAL(&_spinlock);
    return pressed;
}

void RotaryEncoder::encoderTask(void* param) {
    RotaryEncoder* self = static_cast<RotaryEncoder*>(param);

    while (true) {
        self->processEncoder();
        vTaskDelay(pdMS_TO_TICKS(2));  // 2ms polling
    }
}

void RotaryEncoder::buttonTask(void* param) {
    RotaryEncoder* self = static_cast<RotaryEncoder*>(param);

    while (true) {
        self->processButton();
        vTaskDelay(pdMS_TO_TICKS(5));  // 5ms polling
    }
}

void RotaryEncoder::processEncoder() {
    // Lue nykyinen CLK-tila
    uint8_t currentStateCLK = digitalRead(ENCODER_A_PIN);

    // Tunnista muutos: reagoi vain nousevaan reunaan
    if (currentStateCLK != _lastStateCLK && currentStateCLK == 1) {
        // Lue DT (B-pinni) määrittääksesi suunnan
        uint8_t stateDT = digitalRead(ENCODER_B_PIN);

        portENTER_CRITICAL(&_spinlock);
        if (stateDT != currentStateCLK) {
            // CCW (vastapäivään)
            _direction = -1;
            _counter--;
        } else {
            // CW (myötäpäivään)
            _direction = 1;
            _counter++;
        }
        portEXIT_CRITICAL(&_spinlock);
    }

    _lastStateCLK = currentStateCLK;
}

void RotaryEncoder::processButton() {
    if (!_pcf8574) return;

    // Lue painikkeen tila (LOW = painettu, INPUT_PULLUP)
    bool currentState = _pcf8574->digitalRead(P5, true);
    uint32_t now = millis();

    // Debounce: reagoi vain jos tarpeeksi aikaa kulunut
    if (currentState != _lastButtonState) {
        if (now - _lastButtonTime >= DEBOUNCE_MS) {
            // Tila muuttui ja debounce-aika kulunut
            if (currentState == LOW && _lastButtonState == HIGH) {
                // Painallus havaittu (falling edge)
                portENTER_CRITICAL(&_spinlock);
                _buttonPressed = true;
                portEXIT_CRITICAL(&_spinlock);
                Serial.println("[RotaryEncoder] Button pressed");
            }
            _lastButtonState = currentState;
            _lastButtonTime = now;
        }
    }
}
