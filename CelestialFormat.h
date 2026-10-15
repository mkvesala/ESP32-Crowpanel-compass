#pragma once

#include <math.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>

// === C E L E S T I A L  F O R M A T ===
//
// - Pure helpers for CelestialUI: label formatting, star chart placement, data freshness
// - No Arduino or LVGL dependencies — unit tested on the host (tests/celestial)
// - Angles are rounded once to tenths of an arcminute, so 59.95' carries into the next degree

namespace CelestialFormat {

    static constexpr uint8_t NAME_MAX_CHARS = 6;     // longer names are shortened to 6 chars + "."
    static constexpr float   HORIZON_RADIUS_PX = 190.0f;  // Hc 0° at the edge of the 380 px sky panel

    struct ChartPoint {
        int16_t x;  // offset from the screen centre, px (right = +)
        int16_t y;  // offset from the screen centre, px (down = +)
    };

    // Split |deg| into whole degrees, whole minutes and tenths of a minute
    inline void splitDegMin(double deg, long& d, long& m, long& tenths) {
        long t = lround(fabs(deg) * 600.0);  // tenths of an arcminute
        d = t / 600;
        m = (t % 600) / 10;
        tenths = t % 10;
    }

    // Hc as "dd°mm,m'" (e.g. "45°07,3'"), negative values prefixed with "-"
    inline void formatAltitude(char* buf, size_t len, double hc_deg) {
        long d, m, tenths;
        splitDegMin(hc_deg, d, m, tenths);
        bool negative = hc_deg < 0.0 && (d | m | tenths) != 0;
        snprintf(buf, len, "%s%02ld°%02ld,%ld'", negative ? "-" : "", d, m, tenths);
    }

    // Zn as "ddd°" (e.g. "092°"), rounded to whole degrees, 359.5° → "000°"
    inline void formatAzimuth(char* buf, size_t len, double zn_deg) {
        long zn = lround(zn_deg) % 360;
        if (zn < 0) zn += 360;
        snprintf(buf, len, "%03ld°", zn);
    }

    // Latitude as "dd°mm,m'N" / "dd°mm,m'S"
    inline void formatLatitude(char* buf, size_t len, double lat_deg) {
        long d, m, tenths;
        splitDegMin(lat_deg, d, m, tenths);
        snprintf(buf, len, "%02ld°%02ld,%ld'%c", d, m, tenths, lat_deg < 0.0 ? 'S' : 'N');
    }

    // Longitude as "ddd°mm,m'E" / "ddd°mm,m'W"
    inline void formatLongitude(char* buf, size_t len, double lon_deg) {
        long d, m, tenths;
        splitDegMin(lon_deg, d, m, tenths);
        snprintf(buf, len, "%03ld°%02ld,%ld'%c", d, m, tenths, lon_deg < 0.0 ? 'W' : 'E');
    }

    // Body name: up to 6 chars as is, longer ones as 6 chars + "." without a trailing space
    // ("Betelgeuse" → "Betelg.", "Rigil Kentaurus" → "Rigil.")
    inline void abbreviateName(char* buf, size_t len, const char* name) {
        if (len == 0) return;
        size_t n = strlen(name);
        if (n <= NAME_MAX_CHARS) {
            snprintf(buf, len, "%s", name);
            return;
        }
        size_t keep = NAME_MAX_CHARS;
        while (keep > 0 && name[keep - 1] == ' ') keep--;
        snprintf(buf, len, "%.*s.", (int)keep, name);
    }

    // Star chart position of a body: altitude linear from zenith (centre) to horizon (radius),
    // direction relative to ref_deg (true heading, or 0 for north-up).
    // sky_up = false: looking down from the zenith, ref direction up, bearings clockwise.
    // sky_up = true:  looking up from the boat, ref direction down, bearings counter-clockwise.
    inline ChartPoint chartPosition(double hc_deg, double zn_deg, double ref_deg, bool sky_up,
                                    float horizon_px = HORIZON_RADIUS_PX) {
        double hc = hc_deg < 0.0 ? 0.0 : (hc_deg > 90.0 ? 90.0 : hc_deg);
        double r = (90.0 - hc) / 90.0 * horizon_px;
        double rel = (zn_deg - ref_deg) * M_PI / 180.0;
        double x = r * sin(rel);
        double y = r * cos(rel);
        ChartPoint p;
        p.x = (int16_t)lround(x);
        p.y = (int16_t)lround(sky_up ? y : -y);
        return p;
    }

    // True if a timestamp is set and younger than timeout (millis() wraparound safe)
    inline bool isFresh(uint32_t now_ms, uint32_t last_ms, uint32_t timeout_ms) {
        return last_ms != 0 && (uint32_t)(now_ms - last_ms) < timeout_ms;
    }

    // Heading loss is signalled in two steps, so a short dropout does not blank the chart:
    // FRESH → STALE after timeout (bodies still placed with the last heading, texts red)
    //       → LOST after 2 × timeout (bodies hidden). Never received / invalid → LOST.
    enum class HeadingState : uint8_t {
        FRESH,
        STALE,
        LOST
    };

    inline HeadingState headingState(bool have_valid_heading, uint32_t now_ms, uint32_t last_rx_ms, uint32_t timeout_ms) {
        if (!have_valid_heading) return HeadingState::LOST;
        if (isFresh(now_ms, last_rx_ms, timeout_ms)) return HeadingState::FRESH;
        if (isFresh(now_ms, last_rx_ms, 2 * timeout_ms)) return HeadingState::STALE;
        return HeadingState::LOST;
    }

} // namespace CelestialFormat
