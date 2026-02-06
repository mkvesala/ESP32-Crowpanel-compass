# Attitude Level - Implementation Plan

**Date:** 2026-02-04
**Updated:** 2026-02-06
**Status:** CrowPanel complete, Compass pending

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

**STATUS: SKIPPED** - Existing falling edge detection is sufficient. Long press does not cause repeated triggers.

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
| CONFIRM_WAIT | visible | "Level attitude?\nPress knob again to confirm." | Yellow (0xFFFF00) |
| SENDING | visible | "Leveling..." | White (0xFFFFFF) |
| SUCCESS | visible | "Success!" | Green (0x00FF00) |
| FAILED | visible | "Failed!" | Red (0xFF0000) |

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

**STATUS: COMPLETED**

CrowPanel now uses ESP-NOW only (no WiFi connection).

- Removed `begin(ssid, password)` overload from ESPNowReceiver
- Channel must match compass WiFi AP channel
- Configuration: `#define ESP_NOW_CHANNEL 6`

---

## 5. ESP-NOW Bidirectional Communication

**STATUS: COMPLETED** (CrowPanel side)

### ESPNowReceiver Methods

```cpp
bool sendLevelCommand();      // Send LevelCommand broadcast
bool hasLevelResponse();      // Check if response received
bool getLevelResult();        // Get result and clear flag
```

---

## 6. Compass Side Changes (ESPNowBroker) - PENDING

### Required Changes to ESPNowBroker.h

```cpp
// Add to class ESPNowBroker:

private:
    // Level command handling
    uint8_t _lastSenderMac[6];
    volatile bool _levelCommandReceived;

    // Receive callback
    static void onDataRecvStatic(const uint8_t* mac, const uint8_t* data, int len);
    void onDataRecv(const uint8_t* mac, const uint8_t* data, int len);

public:
    // Call from main loop to process pending level commands
    void processLevelCommand();
```

### Required Changes to ESPNowBroker.cpp

**1. Add static instance pointer and callback registration in begin():**

```cpp
// At file level
ESPNowBroker* ESPNowBroker::_instance = nullptr;

// In constructor
ESPNowBroker::ESPNowBroker() : _levelCommandReceived(false) {
    _instance = this;
    memset(_lastSenderMac, 0, 6);
}

// In begin(), after esp_now_init():
esp_now_register_recv_cb(onDataRecvStatic);
```

**2. Add receive callback:**

```cpp
void ESPNowBroker::onDataRecvStatic(const uint8_t* mac, const uint8_t* data, int len) {
    if (_instance) {
        _instance->onDataRecv(mac, data, len);
    }
}

void ESPNowBroker::onDataRecv(const uint8_t* mac, const uint8_t* data, int len) {
    // Check for LevelCommand (8 bytes)
    if (len == 8) {
        // Check magic "LVLC"
        if (data[0] == 'L' && data[1] == 'V' && data[2] == 'L' && data[3] == 'C') {
            Serial.println("[ESPNowBroker] Level command received");
            memcpy(_lastSenderMac, mac, 6);
            _levelCommandReceived = true;
        }
    }
}
```

**3. Add processLevelCommand():**

```cpp
void ESPNowBroker::processLevelCommand() {
    if (!_levelCommandReceived) return;
    _levelCommandReceived = false;

    // Call compass level function
    // Assumes _compass pointer is available (passed in constructor or set method)
    bool success = _compass->level();

    // Build response
    uint8_t resp[8];
    resp[0] = 'L';
    resp[1] = 'V';
    resp[2] = 'L';
    resp[3] = 'R';
    resp[4] = success ? 1 : 0;
    resp[5] = 0;
    resp[6] = 0;
    resp[7] = 0;

    // Add peer for unicast response
    esp_now_peer_info_t peerInfo = {};
    memcpy(peerInfo.peer_addr, _lastSenderMac, 6);
    peerInfo.channel = 0;  // Use current channel
    peerInfo.encrypt = false;

    if (!esp_now_is_peer_exist(_lastSenderMac)) {
        esp_now_add_peer(&peerInfo);
    }

    esp_err_t result = esp_now_send(_lastSenderMac, resp, sizeof(resp));

    Serial.printf("[ESPNowBroker] Level %s, response %s\n",
        success ? "OK" : "FAILED",
        result == ESP_OK ? "sent" : "send failed");
}
```

**4. Call from main loop:**

```cpp
// In main.cpp loop():
espnow.processLevelCommand();
```

---

## 7. SquareLine Studio Changes

**STATUS: COMPLETED**

Leveling dialog structure added to AttitudeScreen:

```
ui_AttitudeScreen
├── ... (existing elements)
│
└── ui_ContainerLevelingDialog (hidden)
    └── ui_PanelLevelingDialog
        └── ui_LabelLevelingDialog
```

---

## 8. Files Modified

### CrowPanel (ESP32-Crowpanel-compass) - COMPLETED

| File | Changes | Status |
|------|---------|--------|
| `HeadingData.h` | Add LevelCommand and LevelResponse structs | ✅ Done |
| `ESPNowReceiver.h` | Add sendLevelCommand(), hasLevelResponse(), getLevelResult(), remove WiFi overload | ✅ Done |
| `ESPNowReceiver.cpp` | Implement bidirectional ESP-NOW, handle LevelResponse, remove WiFi code | ✅ Done |
| `AttitudeUI.h` | Add LevelState enum, state machine variables, handleButtonPress(), updateLevelState(), cancelLevelOperation() | ✅ Done |
| `AttitudeUI.cpp` | Implement state machine, update dialog visibility/text/color | ✅ Done |
| `ScreenManager.h` | Add AttitudeUI pointer, onLeavingAttitudeScreen() | ✅ Done |
| `ScreenManager.cpp` | Call cancelLevelOperation() on screen switch | ✅ Done |
| `ESP32-Crowpanel-compass.ino` | Pass &attitudeUI to screenMgr.begin(), handle button press on AttitudeScreen, call updateLevelState() | ✅ Done |

### Compass (CMPS14-ESP32-SignalK-gateway) - PENDING

| File | Changes | Status |
|------|---------|--------|
| `ESPNowBroker.h` | Add _lastSenderMac, _levelCommandReceived, onDataRecv callback, processLevelCommand() | ⏳ Pending |
| `ESPNowBroker.cpp` | Register recv callback, implement onDataRecv(), implement processLevelCommand() | ⏳ Pending |
| `main.cpp` | Call espnow.processLevelCommand() in loop | ⏳ Pending |

---

## 9. Implementation Order

1. ~~**SquareLine Studio:** Add leveling dialog elements to AttitudeScreen, export~~ ✅ DONE
2. ~~**Push-release validation**~~ SKIPPED (not needed)
3. ~~**HeadingData.h:** Add LevelCommand and LevelResponse structs~~ ✅ DONE
4. ~~**ESPNowReceiver:** Remove WiFi, implement ESP-NOW only mode~~ ✅ DONE
5. ~~**ESPNowReceiver:** Add sendLevelCommand() and response handling~~ ✅ DONE
6. ~~**AttitudeUI:** Implement state machine and label updates~~ ✅ DONE
7. ~~**ScreenManager:** Add cancel callback on screen switch~~ ✅ DONE
8. ~~**ESP32-Crowpanel-compass.ino:** Update setup/loop for level handling~~ ✅ DONE
9. **Compass ESPNowBroker:** Add receive callback and processLevelCommand() ⏳ PENDING
10. **End-to-end testing** ⏳ PENDING

---

## 10. Testing Checklist

- [x] First knob press shows "Level attitude? Press knob again to confirm."
- [x] Timeout after 3s returns to IDLE (dialog hidden)
- [x] Screen switch cancels and returns to IDLE
- [ ] Second press within 3s sends command (shows "Leveling...")
- [ ] "Failed!" shown on timeout (when compass not responding)
- [ ] Compass receives command and calls level()
- [ ] Response sent back to CrowPanel
- [ ] "Success!" shown on success response
- [ ] "Failed!" shown on failure response
- [ ] Pitch/roll values update after successful level
