# Context
Projekti laajenee yksittäisestä kompassinäytöstä monifunktionäytöksi, jossa useat ESP-NOW lähettäjät (kompassi, akku, moottori, sää…) syöttävät dataa eri näytöille. Nykyinen arkkitehtuuri on kovakoodattu kolmelle näytölle ja yhdelle lähettäjälle. Refaktorointi tehdään kahdessa vaiheessa.

## Vaihe 1 — `IScreenUI` + `ScreenManager` luokan-skaalautuvuus (toteutetaan nyt)

### Uusi tiedosto: `IScreenUI.h`

```cpp
class IScreenUI {
public:
    virtual ~IScreenUI() = default;
    virtual void begin()                   = 0;
    virtual lv_obj_t* getLvglScreen() const = 0;
    virtual void onEnter()  {}   // kutsutaan kun näyttö tulee aktiiviseksi
    virtual void onLeave()  {}   // cleanup (peruuta dialogi, tallenna asetus jne.)
    virtual void update()   {}   // kutsutaan UI_UPDATE_INTERVAL_MS-syklissä
    virtual void onButtonPress() {}
    virtual void onRotation(int8_t dir) {}
    virtual bool interceptsRotation() const { return false; }
};
``` 

Periaate: pull-malli — jokainen näyttö hakee itse datansa omalta vastaanottimeltaan `update()`-kutsun sisällä. CrowPanelApplication ei enää tiedä eri näytöistä mitään.

### `CompassUI` — muutokset
`CompassUI.h/.cpp`
- Lisää `ESPNowReceiver&` konstruktoriparametriksi (tallennetaan `_receiver` -viitteenä)
- Toteuta `IScreenUI`
- `getLvglScreen()` → `return ui_CompassScreen`
- `onButtonPress()` → kutsuu `toggleHeadingMode()`
- Uusi `update()` (ei parametreja): hakee `_receiver.isConnected()` + `_receiver.hasNewData()` + `_receiver.getData()` sisäisesti, renderöi
- `showDisconnected()` pysyy privaten `update()` sisällä (ei enää julkinen kutsu)


```cpp
// Uusi julkinen rajapinta (muutos nykyisestä)
class CompassUI : public IScreenUI {
public:
    explicit CompassUI(ESPNowReceiver& receiver);
    void begin()                    override;
    lv_obj_t* getLvglScreen() const override { return ui_CompassScreen; }
    void update()                   override;   // pull-malli
    void onButtonPress()            override;   // toggleHeadingMode()
    // toggleHeadingMode() pysyy julkisena jos tarvitaan (voidaan tehdä privaatiksi)
};
```

### `AttitudeUI` — muutokset
`AttitudeUI.h/.cpp`
- `ESPNowReceiver&` on jo konstruktorissa ✓
- Toteuta `IScreenUI`
- `getLvglScreen()` → `return ui_AttitudeScreen`
- `onLeave()` → kutsuu `cancelLevelOperation()`
- `onButtonPress()` → `handleButtonPress` (nykyinen logiikka)
- `update()`: yhdistää nykyiset `update(data, connected)` + `updateLevelState()` yhteen kutsumatta enää erikseen


```cpp
class AttitudeUI : public IScreenUI {
public:
    explicit AttitudeUI(ESPNowReceiver& receiver);
    void begin()                    override;
    lv_obj_t* getLvglScreen() const override { return ui_AttitudeScreen; }
    void update()                   override;   // pull + level state machine
    void onButtonPress()            override;
    void onLeave()                  override;   // cancelLevelOperation()
};
```

Poistuu julkisesta rajapinnasta: `updateLevelState()`, `cancelLevelOperation()` (→ private), `showDisconnected()` (→ private/internal).

### `BrightnessUI` — muutokset
`BrightnessUI.h/.cpp`
- Toteuta `IScreenUI`
- `getLvglScreen()` → return `ui_BrightnessScreen`
- `onButtonPress()` → `handleButtonPress()` (nykyinen)
- `interceptsRotation()` → return `isAdjusting()`
- `onRotation(int8_t dir)` → `handleRotation(dir)`
- `onLeave()` → `cancelAdjustment()` (nykyinen, jo olemassa)
- `update()` → `updateState()` (nykyinen)

```cpp
class BrightnessUI : public IScreenUI {
public:
    BrightnessUI();
    void begin(int pwm_channel)      /* ei IScreenUI-signatuuria, pysyy overloaded */;
    void begin()                     override { /* delegoi begin(0) tai lataa NVS-kanava */ }
    lv_obj_t* getLvglScreen()  const override { return ui_BrightnessScreen; }
    void update()                    override;   // updateState()
    void onButtonPress()             override;
    void onRotation(int8_t dir)      override;
    bool interceptsRotation() const  override;
    void onLeave()                   override;   // cancelAdjustment()
};
```

Huom: `begin(int pwm_channel)` on edelleen kutsuttava erikseen, tai PWM-kanava voidaan tallentaa konstruktoriparametriksi ennen `begin()` kutsua.

### `ScreenManager` — suuri muutos
Poistetaan: kaikki tyypitetyt viitteet (`CompassUI&, AttitudeUI&, BrightnessUI&`), `enum class Screen`, `switch` -lausekkeet karousellissa, `isCompassActive()` jne.
Tilalle:


```cpp
// ScreenManager.h
class ScreenManager {
public:
    static constexpr uint8_t MAX_SCREENS = 8;

    ScreenManager() = default;

    void addScreen(IScreenUI* screen);      // kutsutaan ennen begin()
    void begin();
    void switchNext();
    void switchPrevious();
    IScreenUI* getCurrentScreen() const;

private:
    enum class Direction { CW, CCW };
    void switchTo(uint8_t index, Direction dir);
    void onLeavingCurrentScreen();

    IScreenUI*  _screens[MAX_SCREENS] = {};
    uint8_t     _screen_count  = 0;
    uint8_t     _current       = 0;
    bool        _initialized   = false;

    static constexpr uint32_t ANIM_DURATION_MS = 300;
};
```

Karuselli — triviaali indeksilogiikka:


```cpp
uint8_t nextIdx()     const { return (_current + 1)                    % _screen_count; }
uint8_t previousIdx() const { return (_current + _screen_count - 1)   % _screen_count; }
```


```cpp
void ScreenManager::switchTo(uint8_t index, Direction dir) {
    if (!_initialized || index == _current) return;
    onLeavingCurrentScreen();               // onLeave() aktiivisella näytöllä
    _current = index;
    lv_obj_t* target = _screens[index]->getLvglScreen();
    auto anim = (dir == Direction::CW)
        ? LV_SCR_LOAD_ANIM_MOVE_LEFT
        : LV_SCR_LOAD_ANIM_MOVE_RIGHT;
    lv_scr_load_anim(target, anim, ANIM_DURATION_MS, 0, false);
    _screens[index]->onEnter();
}
```

### `CrowPanelApplication` — yksinkertaistuu merkittävästi
`CrowPanelApplication.h` — muutokset jäsenmuuttujiin:
- `CompassUI _compassUI` → `CompassUI _compassUI{_receiver}` (receiver konstruktorissa)
- `ScreenManager _screenMgr` → ei enää parametreja konstruktorissa
- `CrowPanelApplication.cpp` — `begin()` lisää rekisteröinnin:


```cpp
void CrowPanelApplication::begin() {
    // ... olemassa olevat init-kutsut ...
    _screenMgr.addScreen(&_compassUI);
    _screenMgr.addScreen(&_attitudeUI);
    _screenMgr.addScreen(&_brightnessUI);
    _screenMgr.begin();
}
```

`handleKnobRotation()` — ennen: 10 riviä, jälkeen:


```cpp
void CrowPanelApplication::handleKnobRotation() {
    int8_t dir = _encoder.getDirection();
    if (dir == 0) return;
    IScreenUI* screen = _screenMgr.getCurrentScreen();
    if (screen->interceptsRotation()) screen->onRotation(dir);
    else dir > 0 ? _screenMgr.switchNext() : _screenMgr.switchPrevious();
}
```

`handleKnobButtonPress()` — ennen: 10 riviä, jälkeen:


```cpp
void CrowPanelApplication::handleKnobButtonPress() {
    if (_encoder.getButtonPressed())
        _screenMgr.getCurrentScreen()->onButtonPress();
}
```

`handleUIUpdate()` — ennen: hakee dataa + reitittelee, jälkeen:


```cpp
void CrowPanelApplication::handleUIUpdate(const uint32_t now) {
    if (now - _last_ui_update < UI_UPDATE_INTERVAL_MS) return;
    _last_ui_update = now;
    uint32_t t0 = micros();
    _screenMgr.getCurrentScreen()->update();
    uint32_t dt = micros() - t0;
    // ... diag-laskurit kuten ennenkin ...
}
```

Poistetaan: `isCompassActive(), isAttitudeActive(), isBrightnessActive()` kaikki käyttökohdat.

### Uuden näytön lisääminen Vaiheen 1 jälkeen
Lisäys vaatii vain nämä muutokset:
1. Luo `BatteryUI.h/.cpp` : `public IScreenUI` (toteuta `begin, getLvglScreen, update`, …)
2. Lisää `BatteryUI _batteryUI;` jäsenmuuttuja `CrowPanelApplication.h` :iin
3. Lisää `_screenMgr.addScreen(&_batteryUI);` `begin()` :iin
4. SquareLine Studio: lisää `ui_BatteryScreen` -näyttöobjekti

Karuselliin ei kosketa.

## Vaihe 2 — Multi-Sender ESP-NOW (toteutetaan myöhemmin)

Toteutetaan multi-sender käärimällä varsinainen ESP-NOW paketti kehykseen, jonka header kertoo paketin tyypin ja jonka varsinainen payload sisältää itse datan (esim `HeadingDelta`).

Suuntaa antava sample code (`espnow_protocol.h`):

```cpp
#pragma once
#include <Arduino.h>
#include <cmath>
#include <cstring>

// =======================
// Protocol header
// =======================

// Optional vakiot, turhaa overheadia tässä vaiheessa?
// static constexpr uint32_t ESPNOW_MAGIC = 0x45534E57; // 'E''S''N''W'
// static constexpr uint8_t  ESPNOW_PROTO_VERSION = 1;

// Viestityypit (lisäätään tätä listaa kun tulee uusia antureita)
enum class EspNowMsgType : uint8_t {
    HEADING_DELTA   = 1,
    BATTERY_DELTA   = 2,
    WEATHER_DELTA   = 3,
    LEVEL_COMMAND   = 10,
    LEVEL_RESPONSE  = 11,
};

// Yleinen header kaikkien viestien eteen
// Pidetään pienenä ja kiinteänä
struct EspNowHeader {
    // uint32_t magic;      // ESPNOW_MAGIC - OPTIONAL
    // uint8_t  version;    // ESPNOW_PROTO_VERSION - OPTIONAL
    uint8_t  msg_type;      // EspNowMsgType
    uint16_t payload_len;   // payloadin pituus tavuina
    // uint16_t seq;        // lähettäjän oma juokseva laskuri - OPTIONAL, turhaa overheadia tässä vaiheessa?
} __attribute__((packed));

// =======================
// Payloads
// =======================

// --- Compass / attitude delta (float radians) ---
struct HeadingDelta {
    float heading_rad;       // Magnetic heading (radians)
    float heading_true_rad;  // True heading (radians)
    float pitch_rad;         // Pitch (radians)
    float roll_rad;          // Roll (radians)
};

// --- Battery delta (float) ---
struct BatteryDelta {
    float house_voltage;   // volts
    float house_current;   // amps, voi olla negatiivinen = lataus / purku
    float house_power;     // watts
    float house_soc;       // percent
    float start_voltage;   // volts, starttiakkku
};

// --- Weather delta (float) ---
struct WeatherDelta {
    float temperature_c;    // °C
    float humidity_p;       // percent
    float pressure_hpa;     // hPa
};

// --- Level command ---
struct LevelCommand {
    uint8_t magic[4];     // "LVLC"
    uint8_t reserved[4];
};

// --- Level response ---
struct LevelResponse {
    uint8_t magic[4];     // "LVLR"
    uint8_t success;
    uint8_t reserved[3];
};

// =======================
// Packet wrappers (header + payload)
// =======================

template <typename TPayload>
struct EspNowPacket {
    EspNowHeader hdr;
    TPayload     payload;
} __attribute__((packed));

// =======================
// Helpers
// =======================

inline void initHeader(EspNowHeader& h, EspNowMsgType type, uint16_t payload_len) {
    // h.magic = ESPNOW_MAGIC;
    // h.version = ESPNOW_PROTO_VERSION;
    h.msg_type = static_cast<uint8_t>(type);
    h.payload_len = payload_len;
    // h.seq = seq;
}

// =======================
// Existing HeadingData converter (unchanged)
// =======================

struct HeadingData {
    uint16_t heading_mag_x10;
    uint16_t heading_true_x10;
    int16_t  pitch_x10;
    int16_t  roll_x10;

    HeadingData()
        : heading_mag_x10(0), heading_true_x10(0), pitch_x10(0), roll_x10(0) {}

    uint16_t getHeadingMagDeg() const { return heading_mag_x10 / 10; }
    uint16_t getHeadingTrueDeg() const { return heading_true_x10 / 10; }
    int16_t  getPitchDeg() const { return pitch_x10 / 10; }
    int16_t  getRollDeg() const { return roll_x10 / 10; }
};

inline HeadingData convertDeltaToData(const HeadingDelta& delta) {
    HeadingData data;
    constexpr float RAD_TO_DEG_X10 = 180.0f * 10.0f / M_PI;

    data.heading_mag_x10  = (uint16_t)(delta.heading_rad      * RAD_TO_DEG_X10);
    data.heading_true_x10 = (uint16_t)(delta.heading_true_rad * RAD_TO_DEG_X10);
    data.pitch_x10        = (int16_t)( delta.pitch_rad        * RAD_TO_DEG_X10);
    data.roll_x10         = (int16_t)( delta.roll_rad         * RAD_TO_DEG_X10);

    return data;
}
```

Suuntaa antava sample code lähetyspäähän (tässä: kompassi):

```cpp
#include "espnow_protocol.h"

void sendHeadingDelta(const HeadingDelta& d) {
    EspNowPacket<HeadingDelta> pkt;
    initHeader(pkt.hdr, EspNowMsgType::HEADING_DELTA, sizeof(HeadingDelta));
    pkt.payload = d;

    esp_now_send(BROADCAST_ADDR, (uint8_t*)&pkt, sizeof(pkt));
}
```

Suuntaa antava sample code vastaanottopäähän (CrowPanel):

```cpp
#include "espnow_protocol.h"

void onEspNowRecv(const uint8_t* mac, const uint8_t* data, int len) {
    if (len < (int)sizeof(EspNowHeader)) return;

    const EspNowHeader* h = (const EspNowHeader*)data;
    // if (h->magic != ESPNOW_MAGIC) return;
    // if (h->version != ESPNOW_PROTO_VERSION) return;

    const uint8_t* payload = data + sizeof(EspNowHeader);
    int payload_len = len - (int)sizeof(EspNowHeader);

    if (h->payload_len != payload_len) {
        // voi olla myös: if (payload_len < h->payload_len) jne
        return;
    }

    switch ((EspNowMsgType)h->msg_type) {

        case EspNowMsgType::HEADING_DELTA: {
            if (h->payload_len != sizeof(HeadingDelta)) return;
            const HeadingDelta* d = (const HeadingDelta*)payload;

            HeadingData hd = convertDeltaToData(*d);
            // do whatever you do
        } break;

        case EspNowMsgType::BATTERIES_DELTA: {
            if (h->payload_len != sizeof(BatteriesDelta)) return;
            const BatteriesDelta* b = (const BatteriesDelta*)payload;

            // do whatever you do
        } break;

        case EspNowMsgType::LEVEL_COMMAND: {
            if (h->payload_len != sizeof(LevelCommand)) return;
            const LevelCommand* cmd = (const LevelCommand*)payload;
            // käsittele komento
        } break;

        case EspNowMsgType::LEVEL_RESPONSE: {
            if (h->payload_len != sizeof(LevelResponse)) return;
            const LevelResponse* resp = (const LevelResponse*)payload;
            // käsittele vastaus
        } break;

        default:
            break;
    }
}
```

## Yhteenveto muutettavista tiedostoista

### Vaihe 1

| Tiedosto | Muutos |
|----------|--------|
| `IScreenUI.h`	| UUSI — abstrakti kantaluokka |
| `ScreenManager.h/.cpp` | Suuri refaktorointi — array + index, poistaa switch + typedref |
| `CompassUI.h/.cpp` | Lisää `ESPNowReceiver&`, toteuttaa `IScreenUI`, pull-malli |
| `AttitudeUI.h/.cpp` |	Toteuttaa `IScreenUI`, yhdistää `update` + `levelState`, `onLeave` |
| `BrightnessUI.h/.cpp` |	Toteuttaa `IScreenUI`, `interceptsRotation, onLeave` |
| `CrowPanelApplication.h/.cpp` |	`addScreen` -rekisteröinti, yksinkertaistetut handle*-metodit |
| `ESPNowReceiver.h/.cpp` |	Ei muutoksia Vaiheessa 1 | 
| `RotaryEncoder.h/.cpp` | Ei muutoksia |

### Vaihe 2

## Verifikaatio
### Vaihe 1 — toiminnallisuuden tarkistus
1. Käännös — kääntyykö kaikki ilman varoituksia
2. Perustoiminta — kompassiruusu pyörii, pitch/roll päivittyy, kirkkaus toimii
3. Karouselli — CW/CCW navigointi kaikissa suunnissa
4. Napin toiminnot — T/M-vaihto, tasausdialogi, kirkkaussäätö
5. Diagnostiikka — [DIAG]-rivit tulostuvat 5s välein (UI updates, LVGL avg/max)
6. Yhteyden katkeaminen — disconnected-tila näkyy kun lähettäjä sammutetaan
7. Uuden näytön lisäys — lisää tyhjä `TestUI : public IScreenUI`, rekisteröi, varmista että karouselli toimii 4 näytöllä ilman muita muutoksia

### Vaihe 2 — multi-sender tarkistus

