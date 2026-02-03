#pragma once

#include <Arduino.h>
#include <cmath>

// Internal struct for compass data
//
// Values stored as uint16_t/int16_t, scaled x10
// for instance 234.5° = 2345
//
// This is to enable
// - Rotation of UI elements 0-3599
// - Integer operations (fast)
// - Easy convert to human readable values (234°)

struct HeadingData {
    uint16_t heading_mag_x10;   // Magnetic heading 0-3599 (0.0° - 359.9°)
    uint16_t heading_true_x10;  // True heading 0-3599 (0.0° - 359.9°)
    int16_t  pitch_x10;         // Pitch -900 to +900 (-90.0° to +90.0°)
    int16_t  roll_x10;          // Roll -1800 to +1800 (-180.0° to +180.0°)

    HeadingData()
        : heading_mag_x10(0)
        , heading_true_x10(0)
        , pitch_x10(0)
        , roll_x10(0)
    {}

    // Helpers for UI (full degrees)
    uint16_t getHeadingMagDeg() const { return heading_mag_x10 / 10; }
    uint16_t getHeadingTrueDeg() const { return heading_true_x10 / 10; }
    int16_t  getPitchDeg() const { return pitch_x10 / 10; }
    int16_t  getRollDeg() const { return roll_x10 / 10; }
};

// ESP-NOW package
// Equal to CMPS14-ESP32-SignalK-gateway compass HeadingDelta
// Values in radians
// Always contains valid four values (compass validates before sending)

struct HeadingDelta {
    float heading_rad;       // Magnetic heading (radians)
    float heading_true_rad;  // True heading (radians)
    float pitch_rad;         // Pitch (radians)
    float roll_rad;          // Roll (radians)
};

// Convert HeadingDelta to HeadingData
inline HeadingData convertDeltaToData(const HeadingDelta& delta) {
    HeadingData data;

    // Rad --> Deg --> x10
    constexpr float RAD_TO_DEG_X10 = 180.0f * 10.0f / M_PI;

    // Heading (magnetic) - normalize to 0-3599
    float hdg_mag_x10 = delta.heading_rad * RAD_TO_DEG_X10;
    data.heading_mag_x10 = (uint16_t)hdg_mag_x10;

    // Heading (true) - normalize to 0-3599
    float hdg_true_x10 = delta.heading_true_rad * RAD_TO_DEG_X10;
    data.heading_true_x10 = (uint16_t)hdg_true_x10;

    // Pitch and Roll - keep sign
    data.pitch_x10 = (int16_t)(delta.pitch_rad * RAD_TO_DEG_X10);
    data.roll_x10  = (int16_t)(delta.roll_rad * RAD_TO_DEG_X10);

    return data;
}
