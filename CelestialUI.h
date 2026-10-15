#pragma once

#include <Arduino.h>
#include <lvgl.h>
#include <Preferences.h>
#include "IScreenUI.h"
#include "ESPNowReceiver.h"
#include "CelestialEngine.h"
#include "CelestialFormat.h"

// === C L A S S  C E L E S T I A L U I ===
//
// - Class CelestialUI - responsible for managing SquareLine generated UI elements on CelestialScreen
// - Realizes: IScreenUI
// - Sextant practice aid: computed altitude Hc and azimuth Zn of up to five bodies, placed on a
//   380 px star chart relative to the boat's true heading
//   - Centre = zenith (Hc 90°), panel edge = horizon (Hc 0°, radius 190 px), altitude linear
//   - Sun and Moon whenever Hc > 0°; in twilight (Sun -3°..-15°) also one planet + two stars
//     picked by CelestialSelector
// - Hc is geocentric (Nautical Almanac / sight reduction convention), see CelestialCalculator
// - Astronomy runs on CelestialEngine's own task (core 0); this class only samples inputs,
//   formats labels and moves containers, once per UTC second
// - Sources: HEADING_DELTA (HDG true), GNSS_DELTA (position), DATETIME_DELTA (UTC), WEATHER_DELTA
//   (pressure, temperature). Each has its own freshness limit, judged by the receiver's RX
//   timestamps; stale data is shown as dashes, never as fresh:
//   - heading stale (2 s)  → "---°", body texts red, bodies still placed with the last heading
//   - heading lost (4 s)   → "---°", all bodies hidden; back to normal when the heading returns
//   - NORTH_UP needs no heading: bodies stay visible and white, only the HDG line shows "---°"
//   - position or time stale → all bodies hidden
// - Knob press: cycles the chart view; its name replaces the centre data for 3 s on every view
//   change and on screen entry
//   1. FROM_ZENITH — looking down on the boat from the zenith: bow up, bearings clockwise
//   2. FROM_VESSEL — looking up at the sky from the boat: bow down, bearings counter-clockwise
//   3. NORTH_UP    — looking down from the zenith with true north up: no heading needed,
//                    bow arrow hidden
// - Chart view: persisted to NVS on onLeave()
// - Owns: CelestialEngine
// - Owned by: CrowPanelApplication

class CelestialUI : public IScreenUI {

public:

    explicit CelestialUI(ESPNowReceiver &receiver);

    void begin() override;
    lv_obj_t* getLvglScreen() const override;
    void onEnter() override;
    void onLeave() override;
    void update() override;
    void onButtonPress() override;

    const CelestialEngine& getEngine() const { return _engine; }

private:

    enum class ChartView : uint8_t {
        FROM_ZENITH = 0,   // default
        FROM_VESSEL = 1,
        NORTH_UP    = 2,
        COUNT
    };

    ESPNowReceiver &_receiver;
    CelestialEngine _engine;
    bool _initialized = false;

    ChartView _view = ChartView::FROM_ZENITH;

    // Latest consumed data
    uint16_t _heading_true_x10 = 0;
    bool     _have_heading     = false;   // a valid (0..3599) heading has been received
    GnssData _gnss;
    double   _utc_anchor       = 0.0;    // UTC at _anchor_millis
    uint32_t _anchor_millis    = 0;
    bool     _have_time        = false;
    float    _pressure_hpa     = NAN;
    float    _temperature_c    = NAN;

    // Refresh once per displayed UTC second (or per second of millis() without time)
    int64_t  _refresh_key = INT64_MIN;

    // Body label text colour currently applied (red while the heading is stale)
    bool     _labels_stale = false;

    // View name banner in ui_LabelData after a view change / screen entry
    bool     _banner_active = false;
    uint32_t _banner_start_ms = 0;

    void consumeData();
    void refresh(uint32_t now);
    void updateDataLabel(uint32_t now, CelestialFormat::HeadingState heading, bool position_ok, bool time_ok);
    void updateChart(uint32_t now, CelestialFormat::HeadingState heading, bool position_ok, bool time_ok);
    void placeBody(lv_obj_t* container, lv_obj_t* label, const char* name,
                   const CelestialEngine::Body& body, double ref_deg);
    void hideBodies();
    void setLabelsStale(bool stale);
    void applyView();
    void showViewBanner();
    static const char* viewBannerText(ChartView view);

    CelestialFormat::HeadingState headingState(uint32_t now) const;
    bool isPositionFresh(uint32_t now) const;
    bool isTimeFresh(uint32_t now) const;
    bool isWeatherFresh(uint32_t now) const;
    double utcAt(uint32_t now) const;

    // NVS
    void saveView();
    ChartView loadView();

    // Freshness limits (sender cadence in brackets)
    static constexpr uint32_t HEADING_TIMEOUT_MS  = 2000;   // CMPS14 (53 ms); bodies hidden at 2×
    static constexpr uint32_t GNSS_TIMEOUT_MS     = 5000;   // UBLOX position heartbeat (499 ms)
    static constexpr uint32_t DATETIME_TIMEOUT_MS = 10000;  // UBLOX DATETIME_DELTA (2 s)
    static constexpr uint32_t WEATHER_TIMEOUT_MS  = 6000;   // BME280 (2 s), as WeatherUI
    static constexpr uint32_t RESULT_MAX_AGE_MS   = 3000;   // engine computes at 1 Hz

    // DATETIME_DELTA carries whole seconds truncated by the sender: +0.5 s centres the error
    static constexpr double DATETIME_TRUNCATION_S = 0.5;

    static constexpr uint32_t REFRESH_INTERVAL_MS = 1000;
    static constexpr int32_t  BOW_ROTATION_FROM_VESSEL = 1800;   // 0.1° units
    static constexpr uint32_t VIEW_BANNER_MS = 3000;

    // View names in ui_LabelData: 7 rows like the data, name on rows 4–5 (centred)
    static constexpr const char* BANNER_FROM_ZENITH = "\n\n\nFROM\nZENITH\n\n";
    static constexpr const char* BANNER_FROM_VESSEL = "\n\n\nFROM\nVESSEL\n\n";
    static constexpr const char* BANNER_NORTH_UP    = "\n\n\nNORTH\nUP\n\n";

    // Body labels: 6 rows × 14 px line height = 84 px of text, but SLS exports them 80 px tall
    // (ui_LabelStar1 even as LV_SIZE_CONTENT wide). LVGL 9.5 clips label text at the label's
    // bottom edge, which cut off the Zn row; content height fits the 90 px container.
    static constexpr int32_t BODY_LABEL_WIDTH = 80;

    static constexpr uint32_t COLOR_TEXT       = 0xFFFFFF;
    static constexpr uint32_t COLOR_TEXT_STALE = 0xFF0000;

    static constexpr const char* NVS_NAMESPACE = "celestial";
    static constexpr const char* NVS_KEY_VIEW  = "view";

};
