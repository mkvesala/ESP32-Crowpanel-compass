#include "CelestialUI.h"
#include "CelestialFormat.h"
#include "NavStars.h"
#include "ui.h"

// === P U B L I C ===

// Constructor
CelestialUI::CelestialUI(ESPNowReceiver &receiver)
    : _receiver(receiver) {}

// Realizes getLvglScreen(): return the LVGL screen object for this UI
lv_obj_t* CelestialUI::getLvglScreen() const {
    return ui_CelestialScreen;
}

// Realizes begin(): initialize
void CelestialUI::begin() {
    if (_initialized) return;

    _view = this->loadView();

    // Star labels must centre on their container like the other body labels
    lv_obj_set_size(ui_LabelStar1, BODY_LABEL_SIZE, BODY_LABEL_SIZE);
    lv_obj_set_size(ui_LabelStar2, BODY_LABEL_SIZE, BODY_LABEL_SIZE);

    this->hideBodies();
    this->applyView();

    _engine.begin();

    _initialized = true;
    this->refresh(millis());
}

// Realizes onEnter(): start computing, redraw at once
void CelestialUI::onEnter() {
    if (!_initialized) return;
    _engine.setActive(true);
    _refresh_key = INT64_MIN;
}

// Realizes onLeave(): stop computing, save chart view to NVS
void CelestialUI::onLeave() {
    _engine.setActive(false);
    this->saveView();
}

// Realizes update(): consume data every tick, redraw once per second
void CelestialUI::update() {
    if (!_initialized) return;

    this->consumeData();

    uint32_t now = millis();
    bool position_ok = this->isPositionFresh(now);
    bool time_ok = this->isTimeFresh(now);

    // Engine input: time anchor + position, flagged invalid when either is stale
    CelestialEngine::Inputs in;
    in.unix_utc      = _utc_anchor;
    in.anchor_millis = _anchor_millis;
    in.lat_deg       = _gnss.lat_deg;
    in.lon_deg       = _gnss.lon_deg;
    in.valid         = position_ok && time_ok;
    _engine.setInputs(in);

    // Redraw when the displayed UTC second changes, so the clock ticks in step with GNSS time
    int64_t key = time_ok ? (int64_t)floor(this->utcAt(now))
                          : -1 - (int64_t)(now / REFRESH_INTERVAL_MS);
    if (key != _refresh_key) {
        _refresh_key = key;
        this->refresh(now);
    }
}

// Realizes onButtonPress(): toggle chart view
void CelestialUI::onButtonPress() {
    if (!_initialized) return;
    _view = (_view == ChartView::FROM_ZENITH) ? ChartView::FROM_BOAT : ChartView::FROM_ZENITH;
    this->applyView();
    this->refresh(millis());
}

// === P R I V A T E ===

// Take whatever is new from the receiver. Freshness is judged separately by RX timestamps.
void CelestialUI::consumeData() {
    if (_receiver.hasNewData()) {
        HeadingData hd = _receiver.getData();
        _heading_true_x10 = hd.heading_true_x10;
        _have_heading = true;
    }

    if (_receiver.hasNewGnssData()) {
        _gnss = _receiver.getGnssData();
    }

    if (_receiver.hasNewDateTimeData()) {
        uint32_t rx_millis;
        DateTimeDelta dt = _receiver.getDateTimeData(rx_millis);
        _have_time = (dt.time_valid == 1);
        if (_have_time) {
            _utc_anchor = (double)dt.unix_utc + DATETIME_TRUNCATION_S;
            _anchor_millis = rx_millis;
        }
    }

    if (_receiver.hasNewWeatherData()) {
        WeatherDelta w = _receiver.getWeatherData();
        _pressure_hpa = w.pressure_hpa;
        _temperature_c = w.temperature_c;
    }
}

// Redraw labels and chart from the current state and the latest engine result
void CelestialUI::refresh(uint32_t now) {
    if (!_initialized) return;

    bool heading_ok = this->isHeadingFresh();
    bool position_ok = this->isPositionFresh(now);
    bool time_ok = this->isTimeFresh(now);

    this->updateDataLabel(now, heading_ok, position_ok, time_ok);
    this->updateChart(now, heading_ok, position_ok, time_ok);
}

// Centre label: blank, HDG true, latitude, longitude, UTC, pressure, temperature
void CelestialUI::updateDataLabel(uint32_t now, bool heading_ok, bool position_ok, bool time_ok) {
    char hdg[12], lat[20], lon[20], utc[12], pres[16], temp[12];

    if (heading_ok) snprintf(hdg, sizeof(hdg), "%03u°", (unsigned)(_heading_true_x10 / 10));
    else            snprintf(hdg, sizeof(hdg), "---°");

    if (position_ok) {
        CelestialFormat::formatLatitude(lat, sizeof(lat), _gnss.lat_deg);
        CelestialFormat::formatLongitude(lon, sizeof(lon), _gnss.lon_deg);
    } else {
        snprintf(lat, sizeof(lat), "--°--,-'");
        snprintf(lon, sizeof(lon), "---°--,-'");
    }

    if (time_ok) {
        int64_t secs = (int64_t)floor(this->utcAt(now));
        uint32_t sod = (uint32_t)(secs % 86400);
        snprintf(utc, sizeof(utc), "%02lu:%02lu:%02lu",
                 (unsigned long)(sod / 3600), (unsigned long)((sod / 60) % 60), (unsigned long)(sod % 60));
    } else {
        snprintf(utc, sizeof(utc), "--:--:--");
    }

    bool weather_ok = this->isWeatherFresh(now);
    if (weather_ok && !isnan(_pressure_hpa)) snprintf(pres, sizeof(pres), "%ld hPa", lround(_pressure_hpa));
    else                                     snprintf(pres, sizeof(pres), "---- hPa");
    if (weather_ok && !isnan(_temperature_c)) snprintf(temp, sizeof(temp), "%ld°C", lround(_temperature_c));
    else                                      snprintf(temp, sizeof(temp), "--°C");

    char buf[96];
    snprintf(buf, sizeof(buf), "\n%s\n%s\n%s\n%s\n%s\n%s", hdg, lat, lon, utc, pres, temp);
    lv_label_set_text(ui_LabelData, buf);
}

// Place the bodies of the latest result; hide everything when its inputs are not fresh
void CelestialUI::updateChart(uint32_t now, bool heading_ok, bool position_ok, bool time_ok) {
    if (heading_ok) lv_obj_remove_flag(ui_ImageBow, LV_OBJ_FLAG_HIDDEN);
    else            lv_obj_add_flag(ui_ImageBow, LV_OBJ_FLAG_HIDDEN);

    CelestialEngine::Result r = _engine.getResult();
    bool result_ok = r.seq != 0 && r.valid
                     && CelestialFormat::isFresh(now, r.computed_millis, RESULT_MAX_AGE_MS);

    if (!position_ok || !time_ok || !result_ok) {
        this->hideBodies();
        return;
    }

    // Bow-relative when the heading is fresh, otherwise north-up
    double ref_deg = heading_ok ? _heading_true_x10 / 10.0 : 0.0;

    this->placeBody(ui_ContainerSun, ui_LabelSun, "Sun", r.sun, ref_deg);
    this->placeBody(ui_ContainerMoon, ui_LabelMoon, "Moon", r.moon, ref_deg);

    // One container per planet; show only the selected one (full name, as designed in SLS)
    lv_obj_t* const planet_containers[CelestialCalculator::PLANET_COUNT] = {
        ui_ContainerVenus, ui_ContainerMars, ui_ContainerJupiter, ui_ContainerSaturn };
    lv_obj_t* const planet_labels[CelestialCalculator::PLANET_COUNT] = {
        ui_LabelVenus, ui_LabelMars, ui_LabelJupiter, ui_LabelSaturn };
    for (uint8_t i = 0; i < CelestialCalculator::PLANET_COUNT; i++) {
        CelestialEngine::Body body;
        if (r.planet == (int8_t)i) body = r.planet_pos;
        this->placeBody(planet_containers[i], planet_labels[i],
                        CelestialCalculator::planetName(static_cast<CelestialCalculator::Planet>(i)), body, ref_deg);
    }

    // Two star slots; names longer than 6 characters are shortened
    lv_obj_t* const star_containers[2] = { ui_ContainerStar1, ui_ContainerStar2 };
    lv_obj_t* const star_labels[2] = { ui_LabelStar1, ui_LabelStar2 };
    for (uint8_t i = 0; i < 2; i++) {
        CelestialEngine::Body body;
        char name[12] = "";
        if (r.star[i] != CelestialSelector::NONE && r.star[i] < NavStars::COUNT) {
            body = r.star_pos[i];
            CelestialFormat::abbreviateName(name, sizeof(name), NavStars::TABLE[r.star[i]].name);
        }
        this->placeBody(star_containers[i], star_labels[i], name, body, ref_deg);
    }
}

// Move a body's container to its chart position and fill in its label, or hide it
void CelestialUI::placeBody(lv_obj_t* container, lv_obj_t* label, const char* name,
                            const CelestialEngine::Body& body, double ref_deg) {
    if (!body.visible) {
        lv_obj_add_flag(container, LV_OBJ_FLAG_HIDDEN);
        return;
    }

    char hc[16], zn[8], buf[48];
    CelestialFormat::formatAltitude(hc, sizeof(hc), body.hc_deg);
    CelestialFormat::formatAzimuth(zn, sizeof(zn), body.zn_deg);
    snprintf(buf, sizeof(buf), "\n%s\n\n\n%s\n%s", name, hc, zn);
    lv_label_set_text(label, buf);

    CelestialFormat::ChartPoint p = CelestialFormat::chartPosition(
        body.hc_deg, body.zn_deg, ref_deg, _view == ChartView::FROM_BOAT);
    lv_obj_set_pos(container, p.x, p.y);
    lv_obj_remove_flag(container, LV_OBJ_FLAG_HIDDEN);
}

void CelestialUI::hideBodies() {
    lv_obj_t* const containers[] = {
        ui_ContainerSun, ui_ContainerMoon,
        ui_ContainerVenus, ui_ContainerMars, ui_ContainerJupiter, ui_ContainerSaturn,
        ui_ContainerStar1, ui_ContainerStar2 };
    for (lv_obj_t* c : containers) lv_obj_add_flag(c, LV_OBJ_FLAG_HIDDEN);
}

// Bow arrow: top and pointing up from the zenith, bottom and pointing down from the boat
void CelestialUI::applyView() {
    if (_view == ChartView::FROM_BOAT) {
        lv_obj_set_align(ui_ImageBow, LV_ALIGN_BOTTOM_MID);
        lv_image_set_rotation(ui_ImageBow, BOW_ROTATION_FROM_BOAT);
    } else {
        lv_obj_set_align(ui_ImageBow, LV_ALIGN_TOP_MID);
        lv_image_set_rotation(ui_ImageBow, 0);
    }
}

bool CelestialUI::isHeadingFresh() const {
    return _have_heading && _heading_true_x10 < 3600 && _receiver.isConnected(HEADING_TIMEOUT_MS);
}

bool CelestialUI::isPositionFresh(uint32_t now) const {
    return _gnss.hasFix() && !isnan(_gnss.lat_deg) && !isnan(_gnss.lon_deg)
           && CelestialFormat::isFresh(now, _receiver.lastGnssRxMillis(), GNSS_TIMEOUT_MS);
}

bool CelestialUI::isTimeFresh(uint32_t now) const {
    return _have_time && CelestialFormat::isFresh(now, _anchor_millis, DATETIME_TIMEOUT_MS);
}

bool CelestialUI::isWeatherFresh(uint32_t now) const {
    return CelestialFormat::isFresh(now, _receiver.lastWeatherRxMillis(), WEATHER_TIMEOUT_MS);
}

// UTC extrapolated from the last GNSS time packet
double CelestialUI::utcAt(uint32_t now) const {
    return _utc_anchor + (double)(int32_t)(now - _anchor_millis) / 1000.0;
}

// Save chart view to NVS
void CelestialUI::saveView() {
    Preferences prefs;
    prefs.begin(NVS_NAMESPACE, false);
    prefs.putUChar(NVS_KEY_VIEW, static_cast<uint8_t>(_view));
    prefs.end();
}

// Load chart view from NVS
CelestialUI::ChartView CelestialUI::loadView() {
    Preferences prefs;
    prefs.begin(NVS_NAMESPACE, true);
    uint8_t val = prefs.getUChar(NVS_KEY_VIEW, 0);  // default: FROM_ZENITH
    prefs.end();

    if (val >= static_cast<uint8_t>(ChartView::COUNT)) val = 0;
    return static_cast<ChartView>(val);
}
