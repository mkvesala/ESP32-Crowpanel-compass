// Host unit tests for the Celestial screen's pure logic.
// Build and run: tests/celestial/run_tests.sh

#include <cmath>
#include <cstdio>
#include <cstring>
#include <string>

#include "../../CelestialCalculator.h"
#include "../../CelestialSelector.h"
#include "../../CelestialFormat.h"
#include "../../NavStars.h"
#include "horizons_fixture.h"
#include "simbad_fixture.h"

// --- minimal test harness ---------------------------------------------------

static int s_checks = 0;
static int s_failures = 0;

#define CHECK(cond) do { s_checks++; if (!(cond)) { s_failures++; \
    printf("  FAIL %s:%d: %s\n", __FILE__, __LINE__, #cond); } } while (0)

#define CHECK_NEAR(a, b, tol) do { s_checks++; double _a = (a), _b = (b); \
    if (!(fabs(_a - _b) <= (tol))) { s_failures++; \
    printf("  FAIL %s:%d: %s = %.6f, expected %.6f ± %.6f\n", __FILE__, __LINE__, #a, _a, _b, (double)(tol)); } } while (0)

#define CHECK_STR(a, b) do { s_checks++; std::string _a = (a), _b = (b); \
    if (_a != _b) { s_failures++; \
    printf("  FAIL %s:%d: \"%s\" != \"%s\"\n", __FILE__, __LINE__, _a.c_str(), _b.c_str()); } } while (0)

static constexpr double D2R = M_PI / 180.0;
static constexpr double ARCMIN = 1.0 / 60.0;

// Wrapped difference of two azimuths, degrees
static double azDiff(double a, double b) {
    double d = fmod(a - b + 540.0, 360.0) - 180.0;
    return fabs(d);
}

// Textbook sight reduction: Hc/Zn from an apparent place of date and local apparent sidereal time
static void sightReduction(double lat, double last_hours, double ra_deg, double dec_deg, double& hc, double& zn) {
    double lha = (last_hours * 15.0 - ra_deg) * D2R;
    double phi = lat * D2R, dec = dec_deg * D2R;
    hc = asin(sin(phi) * sin(dec) + cos(phi) * cos(dec) * cos(lha)) / D2R;
    zn = atan2(-cos(dec) * sin(lha), sin(dec) * cos(phi) - cos(dec) * sin(phi) * cos(lha)) / D2R;
    if (zn < 0.0) zn += 360.0;
}

static CelestialCalculator::Position bodyPosition(CelestialCalculator& calc, const char* body) {
    using P = CelestialCalculator::Planet;
    if (!strcmp(body, "SUN"))     return calc.sun();
    if (!strcmp(body, "MOON"))    return calc.moon();
    if (!strcmp(body, "VENUS"))   return calc.planet(P::VENUS);
    if (!strcmp(body, "MARS"))    return calc.planet(P::MARS);
    if (!strcmp(body, "JUPITER")) return calc.planet(P::JUPITER);
    return calc.planet(P::SATURN);
}

// --- ephemeris vs JPL Horizons ----------------------------------------------

// Geocentric Hc/Zn of Sun, Moon and planets against Hc/Zn reduced from Horizons' geocentric
// apparent RA/Dec and local apparent sidereal time. Requirement: Hc 1', Zn 1° — test 0.5', 0.1°.
static void testEphemerisVsHorizons() {
    printf("ephemeris vs JPL Horizons\n");
    double worst_hc = 0.0, worst_zn = 0.0;
    for (const HorizonsSite& site : HZ_SITES) {
        CelestialCalculator calc;
        calc.set(HZ_TIMES_UNIX[site.time_idx], site.lat, site.lon);
        for (const HorizonsGeo& g : HZ_GEO) {
            if (g.time_idx != site.time_idx) continue;
            double hc_ref, zn_ref;
            sightReduction(site.lat, site.last_hours, g.ra_deg, g.dec_deg, hc_ref, zn_ref);
            CelestialCalculator::Position pos = bodyPosition(calc, g.body);
            CHECK(pos.valid);
            CHECK_NEAR(pos.hc_deg, hc_ref, 0.5 * ARCMIN);
            CHECK(azDiff(pos.zn_deg, zn_ref) <= 0.1);
            worst_hc = fmax(worst_hc, fabs(pos.hc_deg - hc_ref) * 60.0);
            worst_zn = fmax(worst_zn, azDiff(pos.zn_deg, zn_ref));
        }
    }
    printf("  worst |dHc| = %.3f', worst |dZn| = %.4f deg\n", worst_hc, worst_zn);
}

// The Sun's geocentric Hc differs from Horizons' topocentric airless elevation only by solar
// parallax (≤ 0.15'), which checks the horizon transform against an independent observer model
static void testSunVsHorizonsTopocentric() {
    printf("sun vs Horizons topocentric az/el\n");
    for (const HorizonsSite& site : HZ_SITES) {
        CelestialCalculator calc;
        calc.set(HZ_TIMES_UNIX[site.time_idx], site.lat, site.lon);
        CelestialCalculator::Position sun = calc.sun();
        CHECK_NEAR(sun.hc_deg, site.sun_el, 0.3 * ARCMIN);
        CHECK(azDiff(sun.zn_deg, site.sun_az) <= 0.1);
    }
}

// --- stars ------------------------------------------------------------------

// Table entries match SIMBAD looked up by NAME — catches name ↔ position mix-ups
static void testStarTableVsSimbad() {
    printf("star table vs SIMBAD\n");
    CHECK(sizeof(SIMBAD_STARS) / sizeof(SIMBAD_STARS[0]) == NavStars::COUNT);
    for (uint8_t i = 0; i < NavStars::COUNT; i++) {
        const NavStar& s = NavStars::TABLE[i];
        const SimbadStar& ref = SIMBAD_STARS[i];
        CHECK_STR(s.name, ref.name);
        CHECK(s.hip == ref.hip);
        double d = hypot((s.ra_deg - ref.ra_deg) * cos(ref.dec_deg * D2R), s.dec_deg - ref.dec_deg);
        CHECK(d <= 0.01 * ARCMIN);
        CHECK(s.vmag > -1.6f && s.vmag < 3.0f);
    }
}

// Proper motion: Arcturus (-1093.39, -2000.06 mas/yr) moves ~1.0' from J2000.0 to 2026.0
static void testProperMotionArcturus() {
    printf("proper motion (Arcturus)\n");
    uint8_t idx = 255;
    for (uint8_t i = 0; i < NavStars::COUNT; i++) if (!strcmp(NavStars::TABLE[i].name, "Arcturus")) idx = i;
    CHECK(idx != 255);
    if (idx == 255) return;

    const double years = 26.0;
    CelestialCalculator calc;
    calc.set(CelestialCalculator::UNIX_J2000 + years * 365.25 * 86400.0, 60.0, 25.0);
    double ra_h, dec;
    calc.starPositionAtEpoch(idx, ra_h, dec);

    const NavStar& s = NavStars::TABLE[idx];
    double exp_dec = s.dec_deg + (-2000.06 * years) / 3.6e6;                                // -0.8667'
    double exp_ra  = s.ra_deg + (-1093.39 / cos(s.dec_deg * D2R) * years) / 3.6e6;
    CHECK_NEAR(dec, exp_dec, 1e-7);
    CHECK_NEAR(ra_h * 15.0, exp_ra, 1e-7);
    double moved = hypot((ra_h * 15.0 - s.ra_deg) * cos(s.dec_deg * D2R), dec - s.dec_deg) / ARCMIN;
    CHECK_NEAR(moved, 0.99, 0.02);
}

// Rigorous IAU 1976 precession (Meeus 21.2–21.4), no nutation, no aberration
static void precessFromJ2000(double ra0, double dec0, double T, double& ra, double& dec) {
    double zeta  = (2306.2181 * T + 0.30188 * T * T + 0.017998 * T * T * T) / 3600.0 * D2R;
    double z     = (2306.2181 * T + 1.09468 * T * T + 0.018203 * T * T * T) / 3600.0 * D2R;
    double theta = (2004.3109 * T - 0.42665 * T * T - 0.041833 * T * T * T) / 3600.0 * D2R;
    double a0 = ra0 * D2R, d0 = dec0 * D2R;
    double A = cos(d0) * sin(a0 + zeta);
    double B = cos(theta) * cos(d0) * cos(a0 + zeta) - sin(theta) * sin(d0);
    double C = sin(theta) * cos(d0) * cos(a0 + zeta) + cos(theta) * sin(d0);
    ra = (atan2(A, B) + z) / D2R;
    dec = asin(C) / D2R;
}

// Star Hc/Zn against an independent reduction: catalogue place + proper motion, Meeus precession,
// Horizons' sidereal time. Nutation in RA is in LAST; the remainder (nutation in Dec ≤ 9",
// aberration ≤ 20.5") bounds the difference at ~0.5'.
static void testStarsVsIndependentReduction() {
    printf("stars vs independent reduction\n");
    double worst = 0.0;
    for (const HorizonsSite& site : HZ_SITES) {
        CelestialCalculator calc;
        double unix = HZ_TIMES_UNIX[site.time_idx];
        calc.set(unix, site.lat, site.lon);
        double years = (unix - CelestialCalculator::UNIX_J2000) / 86400.0 / 365.25;
        for (uint8_t i = 0; i < NavStars::COUNT; i++) {
            double ra_h, dec_e;
            calc.starPositionAtEpoch(i, ra_h, dec_e);
            double ra_d, dec_d;
            precessFromJ2000(ra_h * 15.0, dec_e, years / 100.0, ra_d, dec_d);
            double hc_ref, zn_ref;
            sightReduction(site.lat, site.last_hours, ra_d, dec_d, hc_ref, zn_ref);

            CelestialCalculator::Position pos = calc.star(i);
            CHECK(pos.valid);
            CHECK_NEAR(pos.hc_deg, hc_ref, 0.6 * ARCMIN);
            if (fabs(hc_ref) < 80.0) CHECK(azDiff(pos.zn_deg, zn_ref) <= 0.1);
            worst = fmax(worst, fabs(pos.hc_deg - hc_ref) * 60.0);
        }
    }
    printf("  worst |dHc| = %.3f'\n", worst);
}

// --- twilight ---------------------------------------------------------------

static void testTwilight() {
    printf("twilight window\n");
    CHECK(CelestialCalculator::isTwilight(-3.0));
    CHECK(CelestialCalculator::isTwilight(-9.0));
    CHECK(CelestialCalculator::isTwilight(-6.0));
    CHECK(!CelestialCalculator::isTwilight(-2.99));
    CHECK(!CelestialCalculator::isTwilight(-9.01));
    CHECK(!CelestialCalculator::isTwilight(-12.0));
    CHECK(!CelestialCalculator::isTwilight(10.0));

    // Fixture time 2 is evening twilight in Helsinki (Sun ≈ -8.3°)
    CelestialCalculator calc;
    calc.set(HZ_TIMES_UNIX[2], 60.15, 24.95);
    CHECK(CelestialCalculator::isTwilight(calc.sun().hc_deg));
}

// --- selector ---------------------------------------------------------------

using Cand = CelestialSelector::Candidate;
using Sel  = CelestialSelector::Selection;

static Cand cand(float hc, float zn, float vmag = 1.0f) {
    Cand c;
    c.hc_deg = hc; c.zn_deg = zn; c.vmag = vmag; c.valid = true;
    return c;
}

static void testSelectorPicksSpreadTriad() {
    printf("selector: triad spread ~120 deg\n");
    Cand planets[4] = { cand(30, 0), cand(5, 100), Cand{}, Cand{} };     // only planet 0 usable
    Cand stars[6] = {
        cand(40, 10),    // 0: bunched with planet
        cand(35, 120),   // 1: ideal
        cand(45, 240),   // 2: ideal
        cand(80, 200),   // 3: above hard window
        cand(30, 130),   // 4: near ideal
        cand(8,  250),   // 5: below hard window
    };
    CelestialSelector sel;
    const Sel& s = sel.update(planets, 4, stars, 6, nullptr);
    CHECK(s.planet == 0);
    CHECK((s.star1 == 1 && s.star2 == 2) || (s.star1 == 2 && s.star2 == 1));
}

static void testSelectorPrefersPreferredAltitude() {
    printf("selector: preferred altitude window\n");
    Cand planets[4] = { cand(30, 0), Cand{}, Cand{}, Cand{} };
    Cand stars[3] = { cand(12, 120), cand(40, 125), cand(40, 240) };      // 0 low, 1 good
    CelestialSelector sel;
    const Sel& s = sel.update(planets, 4, stars, 3, nullptr);
    CHECK(s.planet == 0);
    CHECK((s.star1 == 1 && s.star2 == 2) || (s.star1 == 2 && s.star2 == 1));
}

static void testSelectorNoPlanetGivesTwoStars() {
    printf("selector: no planet -> two stars ~90 deg apart\n");
    Cand planets[4] = { cand(5, 0), Cand{}, Cand{}, Cand{} };            // below hard window
    Cand stars[4] = { cand(40, 0), cand(40, 30), cand(40, 90), cand(40, 180) };
    CelestialSelector sel;
    const Sel& s = sel.update(planets, 4, stars, 4, nullptr);
    CHECK(s.planet == CelestialSelector::NONE);
    CHECK(s.star1 != CelestialSelector::NONE && s.star2 != CelestialSelector::NONE);
    float sep = fabsf(stars[s.star1].zn_deg - stars[s.star2].zn_deg);
    if (sep > 180.0f) sep = 360.0f - sep;
    CHECK_NEAR(sep, 90.0, 0.1);
}

static void testSelectorEmptyWhenNothingUsable() {
    printf("selector: nothing usable\n");
    Cand planets[4] = { Cand{}, Cand{}, Cand{}, Cand{} };
    Cand stars[2] = { cand(5, 0), cand(85, 90) };
    CelestialSelector sel;
    CHECK(sel.update(planets, 4, stars, 2, nullptr).isEmpty());
}

static void testSelectorMoonPenalty() {
    printf("selector: avoids bodies next to the Moon\n");
    Cand planets[4] = { cand(30, 0), Cand{}, Cand{}, Cand{} };
    Cand stars[3] = { cand(40, 120), cand(40, 230), cand(40, 255) };    // star 1 spreads better...
    Cand moon = cand(40, 228);                                           // ...but sits next to the Moon
    CelestialSelector sel;
    const Sel& s = sel.update(planets, 4, stars, 3, &moon);
    CHECK(s.star1 != 1 && s.star2 != 1);
    CHECK(s.star1 == 2 || s.star2 == 2);
}

static void testSelectorHysteresis() {
    printf("selector: hysteresis\n");
    Cand planets[4] = { cand(30, 0), Cand{}, Cand{}, Cand{} };
    Cand stars[3] = { cand(40, 115), cand(40, 240), cand(40, 125) };
    CelestialSelector sel;
    Sel first = sel.update(planets, 4, stars, 3, nullptr);
    CHECK(first.planet == 0);
    CHECK(first.star1 == 1 || first.star2 == 1);
    int8_t picked = (first.star1 == 1) ? first.star2 : first.star1;     // 0 or 2, equally good

    // Slightly better alternative appears: must NOT switch
    int8_t other = (picked == 0) ? 2 : 0;
    stars[other].zn_deg = 120.0f;
    stars[picked].zn_deg = (picked == 0) ? 113.0f : 127.0f;
    CHECK(sel.update(planets, 4, stars, 3, nullptr) == first);

    // Current member drops below the hard window: invalid, must switch at once
    stars[picked].hc_deg = 9.0f;
    CHECK(!sel.isStillValid(planets, 4, stars, 3));
    const Sel& s = sel.update(planets, 4, stars, 3, nullptr);
    CHECK(s.star1 == other || s.star2 == other);
    CHECK(s.star1 != picked && s.star2 != picked);
    CHECK(sel.isStillValid(planets, 4, stars, 3));

    // Clearly better set: switch
    CelestialSelector sel2;
    Cand stars2[3] = { cand(40, 20), cand(40, 240), cand(40, 120) };
    Cand planets2[4] = { cand(30, 0), Cand{}, Cand{}, Cand{} };
    Cand stars2b[3] = { cand(40, 20), cand(40, 240), Cand{} };          // star 2 not yet risen
    Sel a = sel2.update(planets2, 4, stars2b, 3, nullptr);
    CHECK((a.star1 == 0 && a.star2 == 1) || (a.star1 == 1 && a.star2 == 0));
    Sel b = sel2.update(planets2, 4, stars2, 3, nullptr);
    CHECK(b.star1 == 2 || b.star2 == 2);

    sel2.reset();
    CHECK(sel2.current().isEmpty());
}

// --- format -----------------------------------------------------------------

static std::string fmtAlt(double v) { char b[24]; CelestialFormat::formatAltitude(b, sizeof(b), v); return b; }
static std::string fmtAz(double v)  { char b[24]; CelestialFormat::formatAzimuth(b, sizeof(b), v); return b; }
static std::string fmtLat(double v) { char b[24]; CelestialFormat::formatLatitude(b, sizeof(b), v); return b; }
static std::string fmtLon(double v) { char b[24]; CelestialFormat::formatLongitude(b, sizeof(b), v); return b; }
static std::string abbr(const char* n) { char b[24]; CelestialFormat::abbreviateName(b, sizeof(b), n); return b; }

static void testFormat() {
    printf("format\n");
    CHECK_STR(fmtAlt(45.0 + 7.3 / 60.0), "45°07,3'");
    CHECK_STR(fmtAlt(0.0), "00°00,0'");
    CHECK_STR(fmtAlt(59.0 + 59.96 / 60.0), "60°00,0'");     // carry into the next degree
    CHECK_STR(fmtAlt(7.0 + 0.04 / 60.0), "07°00,0'");
    CHECK_STR(fmtAlt(-(2.0 + 30.0 / 60.0)), "-02°30,0'");
    CHECK_STR(fmtAlt(-0.0001 / 60.0), "00°00,0'");          // no "-0"

    CHECK_STR(fmtAz(92.0), "092°");
    CHECK_STR(fmtAz(359.6), "000°");
    CHECK_STR(fmtAz(0.4), "000°");
    CHECK_STR(fmtAz(359.4), "359°");
    CHECK_STR(fmtAz(-1.0), "359°");

    CHECK_STR(fmtLat(60.0 + 12.2 / 60.0), "60°12,2'N");
    CHECK_STR(fmtLat(-(33.0 + 54.0 / 60.0)), "33°54,0'S");
    CHECK_STR(fmtLat(5.5), "05°30,0'N");
    CHECK_STR(fmtLon(25.0 + 23.3 / 60.0), "025°23,3'E");
    CHECK_STR(fmtLon(-(70.0 + 36.0 / 60.0)), "070°36,0'W");
    CHECK_STR(fmtLon(-(179.0 + 59.99 / 60.0)), "180°00,0'W");

    CHECK_STR(abbr("Betelgeuse"), "Betelg.");
    CHECK_STR(abbr("Rigil Kentaurus"), "Rigil.");
    CHECK_STR(abbr("Kaus Australis"), "Kaus A.");
    CHECK_STR(abbr("Sirius"), "Sirius");
    CHECK_STR(abbr("Jupiter"), "Jupite.");
    CHECK_STR(abbr("Sun"), "Sun");
    CHECK_STR(abbr("Al Na'ir"), "Al Na'.");
}

// --- chart placement --------------------------------------------------------

static void testChartPlacement() {
    printf("chart placement\n");
    using CelestialFormat::chartPosition;
    const double R = CelestialFormat::HORIZON_RADIUS_PX;

    // Zenith at the centre, horizon on the 190 px circle
    CelestialFormat::ChartPoint p = chartPosition(90.0, 123.0, 0.0, false);
    CHECK(p.x == 0 && p.y == 0);
    p = chartPosition(0.0, 0.0, 0.0, false);
    CHECK(p.x == 0 && p.y == -190);
    p = chartPosition(45.0, 90.0, 0.0, false);
    CHECK(p.x == 95 && p.y == 0);

    // Spec example: Zn 092°, heading 010° → drawn at relative 082°
    p = chartPosition(30.0, 92.0, 10.0, false);
    double r = 60.0 / 90.0 * R;
    CHECK(p.x == (int16_t)lround(r * sin(82.0 * D2R)));
    CHECK(p.y == (int16_t)lround(-r * cos(82.0 * D2R)));

    // Default view: bow up, starboard right, astern down
    p = chartPosition(0.0, 10.0, 10.0, false);  CHECK(p.x == 0 && p.y == -190);
    p = chartPosition(0.0, 100.0, 10.0, false); CHECK(p.x == 190 && p.y == 0);
    p = chartPosition(0.0, 190.0, 10.0, false); CHECK(p.x == 0 && p.y == 190);

    // Sky-up view: bow down, relative 135° (starboard quarter) top right, where 045° normally is
    p = chartPosition(0.0, 10.0, 10.0, true);   CHECK(p.x == 0 && p.y == 190);
    p = chartPosition(0.0, 145.0, 10.0, true);  CHECK(p.x > 0 && p.y < 0);
    CelestialFormat::ChartPoint q = chartPosition(0.0, 55.0, 10.0, false);
    CHECK(p.x == q.x && p.y == q.y);
    p = chartPosition(0.0, 100.0, 10.0, true);  CHECK(p.x == 190 && p.y == 0);   // starboard stays right

    // Below-horizon values are clamped onto the horizon
    p = chartPosition(-5.0, 0.0, 0.0, false);
    CHECK(p.x == 0 && p.y == -190);
}

// --- freshness --------------------------------------------------------------

static void testFreshness() {
    printf("freshness\n");
    using CelestialFormat::isFresh;
    CHECK(!isFresh(1000, 0, 5000));               // never received
    CHECK(isFresh(5000, 1000, 5000));
    CHECK(!isFresh(6000, 1000, 5000));
    CHECK(isFresh(100, 0xFFFFFF00u, 5000));       // across millis() wraparound
    CHECK(!isFresh(10000, 0xFFFFFF00u, 5000));
}

// Two-stage heading loss: STALE after the timeout, LOST after twice the timeout
static void testHeadingState() {
    printf("heading state\n");
    using CelestialFormat::headingState;
    using HS = CelestialFormat::HeadingState;
    const uint32_t T = 2000;
    CHECK(headingState(true, 10000, 9000, T) == HS::FRESH);
    CHECK(headingState(true, 10999, 9000, T) == HS::FRESH);
    CHECK(headingState(true, 11000, 9000, T) == HS::STALE);     // at the timeout
    CHECK(headingState(true, 12999, 9000, T) == HS::STALE);
    CHECK(headingState(true, 13000, 9000, T) == HS::LOST);      // at 2 × timeout
    CHECK(headingState(true, 60000, 9000, T) == HS::LOST);
    CHECK(headingState(false, 10000, 9000, T) == HS::LOST);     // no valid heading yet
    CHECK(headingState(true, 10000, 0, T) == HS::LOST);         // never received
    CHECK(headingState(true, 1000, 0xFFFFFF00u, T) == HS::FRESH);  // millis() wraparound
    CHECK(headingState(true, 3000, 0xFFFFFF00u, T) == HS::STALE);
    CHECK(headingState(true, 5000, 0xFFFFFF00u, T) == HS::LOST);
}

int main() {
    testEphemerisVsHorizons();
    testSunVsHorizonsTopocentric();
    testStarTableVsSimbad();
    testProperMotionArcturus();
    testStarsVsIndependentReduction();
    testTwilight();
    testSelectorPicksSpreadTriad();
    testSelectorPrefersPreferredAltitude();
    testSelectorNoPlanetGivesTwoStars();
    testSelectorEmptyWhenNothingUsable();
    testSelectorMoonPenalty();
    testSelectorHysteresis();
    testFormat();
    testChartPlacement();
    testFreshness();
    testHeadingState();

    printf("\n%d checks, %d failures\n", s_checks, s_failures);
    return s_failures == 0 ? 0 : 1;
}
