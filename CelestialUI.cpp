#include "CelestialUI.h"
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
    this->showViewBanner();
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

    // View name banner expired: data back at once, not on the next second
    if (_banner_active && (uint32_t)(now - _banner_start_ms) >= VIEW_BANNER_MS) {
        _banner_active = false;
        _refresh_key = INT64_MIN;
    }

    // Redraw when the displayed UTC second changes, so the clock ticks in step with GNSS time
    int64_t key = time_ok ? (int64_t)floor(this->utcAt(now))
                          : -1 - (int64_t)(now / REFRESH_INTERVAL_MS);
    if (key != _refresh_key) {
        _refresh_key = key;
        this->refresh(now);
    }
}

// Realizes onButtonPress(): cycle chart view FROM_ZENITH → FROM_VESSEL → NORTH_UP
void CelestialUI::onButtonPress() {
    if (!_initialized) return;
    uint8_t next = (static_cast<uint8_t>(_view) + 1) % static_cast<uint8_t>(ChartView::COUNT);
    _view = static_cast<ChartView>(next);
    this->applyView();
    this->showViewBanner();
    this->refresh(millis());
}

// === P R I V A T E ===

// Take whatever is new from the receiver. Freshness is judged separately by RX timestamps.
void CelestialUI::consumeData() {
    if (_receiver.hasNewData()) {
        HeadingData hd = _receiver.getData();
        _heading_true_x10 = hd.heading_true_x10;
        _have_heading = (hd.heading_true_x10 < 3600);
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

    CelestialFormat::HeadingState heading = this->headingState(now);
    bool position_ok = this->isPositionFresh(now);
    bool time_ok = this->isTimeFresh(now);

    this->updateDataLabel(now, heading, position_ok, time_ok);
    this->updateChart(now, heading, position_ok, time_ok);
}

// Centre label: blank, HDG true, latitude, longitude, UTC, pressure, temperature —
// or the view name for VIEW_BANNER_MS after a view change
void CelestialUI::updateDataLabel(uint32_t now, CelestialFormat::HeadingState heading, bool position_ok, bool time_ok) {
    if (_banner_active) {
        lv_label_set_text(ui_LabelData, viewBannerText(_view));
        return;
    }

    char hdg[12], lat[20], lon[20], utc[12], pres[16], temp[12];

    if (heading == CelestialFormat::HeadingState::FRESH) snprintf(hdg, sizeof(hdg), "%03u°", (unsigned)(_heading_true_x10 / 10));
    else snprintf(hdg, sizeof(hdg), "---°");

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

// Place the bodies of the latest result; hide everything when its inputs are not fresh.
// Bow-relative views: a stale heading still places the bodies (with the last heading) but turns
// their texts red, a lost heading hides them. NORTH_UP does not use the heading at all.
void CelestialUI::updateChart(uint32_t now, CelestialFormat::HeadingState heading, bool position_ok, bool time_ok) {
    CelestialEngine::Result r = _engine.getResult();
    bool result_ok = r.seq != 0 && r.valid
                     && CelestialFormat::isFresh(now, r.computed_millis, RESULT_MAX_AGE_MS);
    bool north_up = (_view == ChartView::NORTH_UP);

    if (!position_ok || !time_ok || !result_ok
        || (!north_up && heading == CelestialFormat::HeadingState::LOST)) {
        this->hideBodies();
        return;
    }

    this->setLabelsStale(!north_up && heading == CelestialFormat::HeadingState::STALE);
    double ref_deg = north_up ? 0.0 : _heading_true_x10 / 10.0;

    this->placeBody(ui_ContainerSun, ui_LabelSun, "Sun", r.sun, ref_deg);
    this->placeBody(ui_ContainerMoon, ui_LabelMoon, "Moon", r.moon, ref_deg);

    // One planet container, recoloured for whichever planet is selected (full name, not shortened)
    CelestialEngine::Body planet_body;
    const char* planet_name = "";
    if (r.planet != CelestialSelector::NONE && r.planet < (int8_t)CelestialCalculator::PLANET_COUNT) {
        planet_body = r.planet_pos;
        planet_name = CelestialCalculator::planetName(static_cast<CelestialCalculator::Planet>(r.planet));
        this->setPlanetColor(r.planet);
    }
    this->placeBody(ui_ContainerPlanet, ui_LabelPlanet, planet_name, planet_body, ref_deg);

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
        body.hc_deg, body.zn_deg, ref_deg, _view == ChartView::FROM_VESSEL);
    lv_obj_set_pos(container, p.x, p.y);
    lv_obj_remove_flag(container, LV_OBJ_FLAG_HIDDEN);
}

void CelestialUI::hideBodies() {
    lv_obj_t* const containers[] = {
        ui_ContainerSun, ui_ContainerMoon, ui_ContainerPlanet,
        ui_ContainerStar1, ui_ContainerStar2 };
    for (lv_obj_t* c : containers) lv_obj_add_flag(c, LV_OBJ_FLAG_HIDDEN);
}

// Body label text colour: red while the heading is stale, white otherwise (only on change)
void CelestialUI::setLabelsStale(bool stale) {
    if (stale == _labels_stale) return;
    _labels_stale = stale;
    lv_color_t color = lv_color_hex(stale ? COLOR_TEXT_STALE : COLOR_TEXT);
    lv_obj_t* const labels[] = {
        ui_LabelSun, ui_LabelMoon, ui_LabelPlanet,
        ui_LabelStar1, ui_LabelStar2 };
    for (lv_obj_t* label : labels) lv_obj_set_style_text_color(label, color, 0);
}

// Planet dot colour (background, border, outline) for the selected planet (only on change)
void CelestialUI::setPlanetColor(int8_t planet) {
    if (planet == _planet_color || planet < 0 || planet >= (int8_t)CelestialCalculator::PLANET_COUNT) return;
    _planet_color = planet;
    lv_color_t color = lv_color_hex(PLANET_COLORS[planet]);
    lv_obj_set_style_bg_color(ui_PanelPlanet, color, 0);
    lv_obj_set_style_border_color(ui_PanelPlanet, color, 0);
    lv_obj_set_style_outline_color(ui_PanelPlanet, color, 0);
}

// Bow arrow: top and pointing up from the zenith, bottom and pointing down from the vessel.
// North-up the top of the chart is north, not the bow: arrow hidden, N/E/S/W shown instead.
void CelestialUI::applyView() {
    if (_view == ChartView::FROM_VESSEL) {
        lv_obj_set_align(ui_ImageBow, LV_ALIGN_BOTTOM_MID);
        lv_image_set_rotation(ui_ImageBow, BOW_ROTATION_FROM_VESSEL);
    } else {
        lv_obj_set_align(ui_ImageBow, LV_ALIGN_TOP_MID);
        lv_image_set_rotation(ui_ImageBow, 0);
    }
    if (_view == ChartView::NORTH_UP) {
        lv_obj_add_flag(ui_ImageBow, LV_OBJ_FLAG_HIDDEN);
        lv_obj_remove_flag(ui_LabelCardinals, LV_OBJ_FLAG_HIDDEN);
    } else {
        lv_obj_remove_flag(ui_ImageBow, LV_OBJ_FLAG_HIDDEN);
        lv_obj_add_flag(ui_LabelCardinals, LV_OBJ_FLAG_HIDDEN);
    }
}

// Show the active view's name in place of the centre data for VIEW_BANNER_MS
void CelestialUI::showViewBanner() {
    _banner_active = true;
    _banner_start_ms = millis();
}

const char* CelestialUI::viewBannerText(ChartView view) {
    switch (view) {
        case ChartView::FROM_VESSEL: return BANNER_FROM_VESSEL;
        case ChartView::NORTH_UP:    return BANNER_NORTH_UP;
        default:                     return BANNER_FROM_ZENITH;
    }
}

CelestialFormat::HeadingState CelestialUI::headingState(uint32_t now) const {
    return CelestialFormat::headingState(_have_heading, now, _receiver.lastHeadingRxMillis(), HEADING_TIMEOUT_MS);
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
