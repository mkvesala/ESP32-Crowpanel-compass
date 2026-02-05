# Attitude Level - Implementation Plan

**Date:** 2026-02-04
**Status:** Planning complete, ready for implementation

---

## Overview

Implement attitude leveling feature that allows the user to set current pitch/roll as zero reference by pressing the rotary knob on AttitudeScreen. The level command is sent via ESP-NOW to the compass, which performs the actual calibration.

---

## 1. Architecture

### Communication Flow

```
CrowPanel                              Compass (CMPS14-ESP32-SignalK-gateway)
    |                                      |
[Knob press on AttitudeScreen]             |
    |                                      |
[Show "Level? Press to confirm"]           |
    |                                      |
[Knob press within 3s]                     |
    |                                      |
[Show "Leveling..."]                       |
    |                                      |
    |--[ESP-NOW broadcast LevelCommand]--->|
    |                                      |
    |                         [Extract CrowPanel MAC from packet]
    |                         [Call compass.level()]
    |                                      |
    |<--[ESP-NOW unicast LevelResponse]----|
    |                                      |
[Show "Level OK" or "Level Failed"]        |
    |                                      |
[Return to IDLE after 1.5-2s]              |
```

### Message Protocol

```cpp
// CrowPanel → Compass (broadcast)
struct LevelCommand {
    uint8_t magic[4] = {'L','V','L','C'};  // "LVLC" = Level Command
    uint8_t reserved[4];                    // Future use, padding
};

// Compass → CrowPanel (unicast to sender MAC)
struct LevelResponse {
    uint8_t magic[4] = {'L','V','L','R'};  // "LVLR" = Level Response
    uint8_t success;                        // 1 = OK, 0 = failed
    uint8_t reserved[3];                    // Padding
};
```

---

## 2. Button Press Safety (Push-Release Validation)

To prevent accidental triggers (e.g., coffee mug on knob), require full push-release cycle within time window.

### RotaryEncoder Changes

**New member variables:**
```cpp
volatile uint32_t _pressStartTime;   // When button was pressed down
volatile bool _pendingPress;         // Waiting for release
```

**New constants:**
```cpp
static constexpr uint32_t PRESS_MIN_MS = 50;   // Minimum press duration
static constexpr uint32_t PRESS_MAX_MS = 500;  // Maximum press duration
```

**New processButton() logic:**
```cpp
void RotaryEncoder::processButton() {
    if (!_pcf8574) return;

    bool currentState = _pcf8574->digitalRead(P5, true);
    uint32_t now = millis();

    // Debounce
    if (now - _lastButtonTime < DEBOUNCE_MS) return;

    // Falling edge: button pressed down
    if (currentState == LOW && _lastButtonState == HIGH) {
        _pressStartTime = now;
        _pendingPress = true;
        _lastButtonState = currentState;
        _lastButtonTime = now;
    }
    // Rising edge: button released
    else if (currentState == HIGH && _lastButtonState == LOW) {
        if (_pendingPress) {
            uint32_t pressDuration = now - _pressStartTime;
            // Valid press: 50-500ms duration
            if (pressDuration >= PRESS_MIN_MS && pressDuration <= PRESS_MAX_MS) {
                portENTER_CRITICAL(&_spinlock);
                _buttonPressed = true;
                portEXIT_CRITICAL(&_spinlock);
                Serial.println("[RotaryEncoder] Valid button press");
            } else {
                Serial.printf("[RotaryEncoder] Invalid press duration: %lu ms\n", pressDuration);
            }
        }
        _pendingPress = false;
        _lastButtonState = currentState;
        _lastButtonTime = now;
    }
}
```

---

## 3. AttitudeUI State Machine

### States

```cpp
enum class LevelState {
    IDLE,           // Normal operation, label hidden
    CONFIRM_WAIT,   // "Level? Press to confirm" visible, waiting for 2nd press
    SENDING,        // "Leveling..." visible, waiting for response
    SUCCESS,        // "Level OK" visible briefly
    FAILED          // "Level Failed" visible briefly
};
```

### State Diagram

```
                    [Knob press]
        IDLE ─────────────────────> CONFIRM_WAIT
          ▲                              │
          │                              │
          │ [timeout 3s]                 │ [Knob press]
          │ [knob turn]                  │
          │ [screen switch]              │
          │                              ▼
          │                          SENDING
          │                              │
          │     ┌────────────────────────┴────────────────────────┐
          │     │                                                 │
          │     ▼ [LevelResponse success=1]                       ▼ [timeout 3s / success=0]
          │  SUCCESS                                           FAILED
          │     │                                                 │
          │     │ [1.5s]                                          │ [2s]
          └─────┴─────────────────────────────────────────────────┘
```

### Label Text and Colors

| State | ui_ContainerLevelingDialog | ui_LabelLevelingDialog Text | Color |
|-------|----------------------------|----------------------------|-------|
| IDLE | hidden | - | - |
| CONFIRM_WAIT | visible | "Level?\nPress to confirm" | Yellow (0xFFFF00) |
| SENDING | visible | "Leveling..." | White (0xFFFFFF) |
| SUCCESS | visible | "Level OK" | Green (0x00FF00) |
| FAILED | visible | "Level Failed" | Red (0xFF0000) |

### Timeouts

| State | Timeout | Action |
|-------|---------|--------|
| CONFIRM_WAIT | 3000 ms | Return to IDLE |
| SENDING | 3000 ms | Go to FAILED |
| SUCCESS | 1500 ms | Return to IDLE |
| FAILED | 2000 ms | Return to IDLE |

### Screen Switch Behavior

If user switches screen (knob turn) while in any state except IDLE:
- Cancel operation immediately
- Reset state to IDLE
- Hide status label

Implementation: `ScreenManager::switchNext/Previous()` calls `attitudeUI.cancelLevelOperation()`

---

## 4. WiFi Removal from CrowPanel

CrowPanel will use ESP-NOW only (no WiFi connection needed).

### Changes to ESPNowReceiver

**Remove:**
- `begin(const char* ssid, const char* password)` overload
- All WiFi.begin() and WiFi.status() code
- WiFi credential handling

**Keep/Modify:**
- `begin(uint8_t channel)` as only begin method
- `WiFi.mode(WIFI_STA)` required for ESP-NOW
- Manual channel setting

**New begin() implementation:**
```cpp
void ESPNowReceiver::begin(uint8_t channel) {
    WiFi.mode(WIFI_STA);
    WiFi.disconnect();

    // Set channel manually
    esp_wifi_set_channel(channel, WIFI_SECOND_CHAN_NONE);

    // ESP-NOW init
    if (esp_now_init() != ESP_OK) {
        Serial.println("[ESPNowReceiver] ESP-NOW init failed");
        return;
    }

    esp_now_register_recv_cb(onDataReceived);

    _channel = channel;
    _initialized = true;
    Serial.printf("[ESPNowReceiver] Initialized on channel %d (ESP-NOW only)\n", channel);
}
```

### Changes to ESP32-Crowpanel-compass.ino

```cpp
// Remove
#include "secrets.h"
#define USE_WIFI_CONNECTION  true

// Add/Modify
#define ESP_NOW_CHANNEL  1  // Must match compass channel

// In setup():
receiver.begin(ESP_NOW_CHANNEL);
```

### File Cleanup

- `secrets.h` can be deleted (or kept empty for future use)
- Remove `WIFI_SSID`, `WIFI_PASSWORD` references

---

## 5. ESP-NOW Bidirectional Communication

### ESPNowReceiver Additions

**New methods:**
```cpp
// Send level command (broadcast)
bool sendLevelCommand();

// Check if level response received
bool hasLevelResponse();

// Get level response result and clear flag
bool getLevelResult();

// Set callback for level response (optional)
void setLevelResponseCallback(void (*callback)(bool success));
```

**New member variables:**
```cpp
volatile bool _levelResponseReceived;
volatile bool _levelResponseSuccess;
void (*_levelResponseCallback)(bool success);
```

**Modified onDataReceived callback:**
```cpp
void ESPNowReceiver::onDataReceived(const uint8_t* mac, const uint8_t* data, int len) {
    // Check for HeadingDelta (existing)
    if (len == sizeof(HeadingDelta)) {
        // ... existing code ...
    }
    // Check for LevelResponse
    else if (len == sizeof(LevelResponse)) {
        LevelResponse* resp = (LevelResponse*)data;
        if (memcmp(resp->magic, "LVLR", 4) == 0) {
            s_instance->_levelResponseReceived = true;
            s_instance->_levelResponseSuccess = (resp->success == 1);
            if (s_instance->_levelResponseCallback) {
                s_instance->_levelResponseCallback(resp->success == 1);
            }
        }
    }
}
```

**sendLevelCommand implementation:**
```cpp
bool ESPNowReceiver::sendLevelCommand() {
    LevelCommand cmd;
    memcpy(cmd.magic, "LVLC", 4);
    memset(cmd.reserved, 0, 4);

    // Broadcast to all (FF:FF:FF:FF:FF:FF)
    uint8_t broadcastAddr[] = {0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF};

    // Add broadcast peer if not exists
    esp_now_peer_info_t peerInfo = {};
    memcpy(peerInfo.peer_addr, broadcastAddr, 6);
    peerInfo.channel = _channel;
    peerInfo.encrypt = false;

    if (!esp_now_is_peer_exist(broadcastAddr)) {
        esp_now_add_peer(&peerInfo);
    }

    esp_err_t result = esp_now_send(broadcastAddr, (uint8_t*)&cmd, sizeof(cmd));

    _levelResponseReceived = false;
    _levelResponseSuccess = false;

    return (result == ESP_OK);
}
```

---

## 6. Compass Side Changes (ESPNowBroker)

### New receive callback

```cpp
void ESPNowBroker::onDataReceived(const uint8_t* mac, const uint8_t* data, int len) {
    if (len == sizeof(LevelCommand)) {
        LevelCommand* cmd = (LevelCommand*)data;
        if (memcmp(cmd->magic, "LVLC", 4) == 0) {
            Serial.println("[ESPNowBroker] Level command received");

            // Store sender MAC for response
            memcpy(_lastSenderMac, mac, 6);
            _levelCommandReceived = true;
        }
    }
}
```

### Process level command in loop

```cpp
void ESPNowBroker::processLevelCommand() {
    if (!_levelCommandReceived) return;
    _levelCommandReceived = false;

    // Call compass level function
    bool success = _compass->level();

    // Send response back to sender
    LevelResponse resp;
    memcpy(resp.magic, "LVLR", 4);
    resp.success = success ? 1 : 0;
    memset(resp.reserved, 0, 3);

    // Add peer for unicast response
    esp_now_peer_info_t peerInfo = {};
    memcpy(peerInfo.peer_addr, _lastSenderMac, 6);
    peerInfo.channel = _channel;
    peerInfo.encrypt = false;

    if (!esp_now_is_peer_exist(_lastSenderMac)) {
        esp_now_add_peer(&peerInfo);
    }

    esp_now_send(_lastSenderMac, (uint8_t*)&resp, sizeof(resp));

    Serial.printf("[ESPNowBroker] Level %s, response sent\n", success ? "OK" : "FAILED");
}
```

---

## 7. SquareLine Studio Changes

### AttitudeScreen - New Elements (DONE)

Leveling dialog structure added to AttitudeScreen:

```
ui_AttitudeScreen
├── ... (existing elements)
│
└── ui_ContainerLevelingDialog (hidden)
    └── ui_PanelLevelingDialog
        └── ui_LabelLevelingDialog
```

| Element | Properties |
|---------|------------|
| `ui_ContainerLevelingDialog` | Hidden by default, contains dialog panel |
| `ui_PanelLevelingDialog` | Background panel for dialog (semi-transparent or solid) |
| `ui_LabelLevelingDialog` | Text label, font includes ASCII 0x20-0x7A |

### Screen Shape Note

SquareLine Studio screen shape setting (Rectangle vs Circle) does not affect generated code.
Rectangle works fine since the 480×480 content is centered and hardware clips the corners automatically.
Circle setting is only a visual aid in the editor.

---

## 8. Files to Modify

### CrowPanel (ESP32-Crowpanel-compass)

| File | Changes |
|------|---------|
| `RotaryEncoder.h` | Add _pressStartTime, _pendingPress, PRESS_MIN_MS, PRESS_MAX_MS |
| `RotaryEncoder.cpp` | Implement push-release validation in processButton() |
| `ESPNowReceiver.h` | Add LevelCommand/Response structs, sendLevelCommand(), response handling |
| `ESPNowReceiver.cpp` | Implement bidirectional ESP-NOW, remove WiFi |
| `AttitudeUI.h` | Add LevelState enum, state machine variables, new methods |
| `AttitudeUI.cpp` | Implement state machine, update ui_ContainerLevelingDialog visibility, ui_LabelLevelingDialog text/color, cancelLevelOperation() |
| `ScreenManager.h` | Add reference to AttitudeUI for cancel callback |
| `ScreenManager.cpp` | Call attitudeUI.cancelLevelOperation() on screen switch |
| `ESP32-Crowpanel-compass.ino` | Remove secrets.h, update receiver.begin(), add level handling in loop |
| `HeadingData.h` | Add LevelCommand and LevelResponse structs |

### Compass (CMPS14-ESP32-SignalK-gateway)

| File | Changes |
|------|---------|
| `ESPNowBroker.h` | Add receive callback, _lastSenderMac, _levelCommandReceived |
| `ESPNowBroker.cpp` | Implement onDataReceived, processLevelCommand() |

---

## 9. Implementation Order

1. ~~**SquareLine Studio:** Add leveling dialog elements to AttitudeScreen, export~~ **DONE**
   - Added: ui_ContainerLevelingDialog, ui_PanelLevelingDialog, ui_LabelLevelingDialog
2. **RotaryEncoder:** Implement push-release validation
3. **HeadingData.h:** Add LevelCommand and LevelResponse structs
4. **ESPNowReceiver:** Remove WiFi, implement ESP-NOW only mode
5. **ESPNowReceiver:** Add sendLevelCommand() and response handling
6. **AttitudeUI:** Implement state machine and label updates
7. **ScreenManager:** Add cancel callback on screen switch
8. **ESP32-Crowpanel-compass.ino:** Update setup/loop for level handling
9. **Compass ESPNowBroker:** Add receive callback and processLevelCommand()
10. **End-to-end testing**

---

## 10. Testing Checklist

- [ ] Push-release validation works (50-500ms)
- [ ] Long press (>500ms) is ignored
- [ ] Coffee mug on knob doesn't trigger
- [ ] First knob press shows "Level? Press to confirm"
- [ ] Second press within 3s sends command
- [ ] Timeout after 3s returns to IDLE
- [ ] Knob turn cancels and returns to IDLE
- [ ] Screen switch cancels and returns to IDLE
- [ ] "Leveling..." shown while waiting
- [ ] "Level OK" shown on success
- [ ] "Level Failed" shown on timeout/failure
- [ ] Compass receives command and calls level()
- [ ] Response sent back to CrowPanel
- [ ] Pitch/roll values update after successful level
- [ ] ESP-NOW works without WiFi connection
- [ ] Channel matches between compass and CrowPanel
