#pragma once

#include <Arduino.h>
#include <math.h>

// === T R E N D   I N D I C A T O R ===
//
// - Header-only, no LVGL dependency
// - Estimates the direction a value is developing in, for the ↑/↓ trend arrows
// - Two time-constant based EMAs (fast, slow): smoothing does not depend on the sender's rate
// - For a value changing linearly at rate r, (fast - slow) settles to r * (tau_slow - tau_fast),
//   so rate per minute = (fast - slow) / (tau_slow - tau_fast) * 60
// - Hysteresis: arrow appears at rate_on, disappears below rate_on / 2. UP <-> DOWN always passes NONE
// - A gap longer than tau_slow between samples restarts from the new value

class TrendIndicator {

public:
    enum class Trend : int8_t { DOWN = -1, NONE = 0, UP = 1 };

    constexpr TrendIndicator(float tau_fast_s, float tau_slow_s, float rate_on_per_min)
        : _tau_fast_s(tau_fast_s), _tau_slow_s(tau_slow_s), _rate_on_per_min(rate_on_per_min) {}

    // Feed a new sample. Returns true if the trend changed.
    bool update(float value, uint32_t now_ms) {
        if (isnan(value)) return false;

        if (!_started || (now_ms - _last_ms) > static_cast<uint32_t>(_tau_slow_s * 1000.0f)) {
            Trend previous = _trend;
            _fast = value;
            _slow = value;
            _last_ms = now_ms;
            _started = true;
            _trend = Trend::NONE;
            return previous != _trend;
        }

        float dt_s = (now_ms - _last_ms) / 1000.0f;
        _last_ms = now_ms;
        _fast += (1.0f - expf(-dt_s / _tau_fast_s)) * (value - _fast);
        _slow += (1.0f - expf(-dt_s / _tau_slow_s)) * (value - _slow);

        float rate = (_fast - _slow) / (_tau_slow_s - _tau_fast_s) * 60.0f;
        float rate_off = 0.5f * _rate_on_per_min;

        Trend next = _trend;
        switch (_trend) {
            case Trend::NONE:
                if (rate >= _rate_on_per_min)       next = Trend::UP;
                else if (rate <= -_rate_on_per_min) next = Trend::DOWN;
                break;
            case Trend::UP:
                if (rate < rate_off) next = Trend::NONE;
                break;
            case Trend::DOWN:
                if (rate > -rate_off) next = Trend::NONE;
                break;
        }

        bool changed = (next != _trend);
        _trend = next;
        return changed;
    }

    Trend trend() const { return _trend; }

    // Forget history (e.g. on connection loss) — next sample starts from scratch
    void reset() {
        _started = false;
        _trend = Trend::NONE;
    }

private:
    float _tau_fast_s;
    float _tau_slow_s;
    float _rate_on_per_min;

    float _fast = 0.0f;
    float _slow = 0.0f;
    uint32_t _last_ms = 0;
    bool _started = false;
    Trend _trend = Trend::NONE;

};
