# Context
Projekti laajenee yksittäisestä kompassinäytöstä monifunktionäytöksi, jossa useat ESP-NOW lähettäjät (kompassi, akku, moottori, sää…) syöttävät dataa eri näytöille. Nykyinen arkkitehtuuri on kovakoodattu kolmelle näytölle ja yhdelle lähettäjälle. Refaktorointi tehdään kahdessa vaiheessa.

## Vaihe 1 — `IScreenUI` + `ScreenManager` luokan-skaalautuvuus (toteutetaan nyt)

### Uusi tiedosto: `IScreenUI.h`

```cpp
class IScreenUI {
public:
    virtual ~IScreenUI() = default;
    virtual void begin()                    = 0;
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

---

### `CompassUI` — muutokset
`CompassUI.h/.cpp`
- Lisää `ESPNowReceiver&` konstruktoriparametriksi (tallennetaan `_receiver`-viitteenä)
- Toteuta `IScreenUI`
- `getLvglScreen()` → `return ui_CompassScreen`
- `onButtonPress()` → kutsuu `toggleHeadingMode()`
- Uusi `update()` (ei parametreja): hakee `_receiver.isConnected()` + `_receiver.hasNewData()` + `_receiver.getData()` sisäisesti, renderöi
- `showDisconnected()` pysyy privaten `update()`-sisällä (ei enää julkinen kutsu)

```cpp
// Uusi julkinen rajapinta (muutos nykyisestä)
class CompassUI : public IScreenUI {
public:
    explicit CompassUI(ESPNowReceiver& receiver);
    void begin()                    override;
    lv_obj_t* getLvglScreen() const override { return ui_CompassScreen; }
    void update()                   override;   // pull-malli
    void onButtonPress()            override;   // toggleHeadingMode()
};
```

---

### `AttitudeUI` — muutokset
`AttitudeUI.h/.cpp`
- `ESPNowReceiver&` on jo konstruktorissa ✓
- Toteuta `IScreenUI`
- `getLvglScreen()` → `return ui_AttitudeScreen`
- `onLeave()` → kutsuu `cancelLevelOperation()`
- `onButtonPress()` → `handleButtonPress()` (nykyinen logiikka)
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

---

### `BrightnessUI` — muutokset
`BrightnessUI.h/.cpp`
- Siirrä `pwm_channel` konstruktoriparametriksi (tallennetaan `_pwm_channel`-jäsenmuuttujaan)
  — yhdenmukainen `AttitudeUI(ESPNowReceiver&)` -mallin kanssa, välttää kaksoissignatuuriongelma
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
    explicit BrightnessUI(int pwm_channel);
    void begin()                     override;   // käyttää _pwm_channel-jäsenmuuttujaa
    lv_obj_t* getLvglScreen()  const override { return ui_BrightnessScreen; }
    void update()                    override;   // updateState()
    void onButtonPress()             override;
    void onRotation(int8_t dir)      override;
    bool interceptsRotation()  const override;
    void onLeave()                   override;   // cancelAdjustment()
};
```

`CrowPanelApplication.h`:ssa jäsenmuuttuja: `BrightnessUI _brightnessUI{PWM_CHANNEL};`

---

### `ScreenManager` — suuri muutos
Poistetaan: kaikki tyypitetyt viitteet (`CompassUI&, AttitudeUI&, BrightnessUI&`), `enum class Screen`, `switch`-lausekkeet karousellissa, `isCompassActive()` jne.
Tilalle:

```cpp
// ScreenManager.h
class ScreenManager {
public:
    static constexpr uint8_t MAX_SCREENS = 8;

    ScreenManager() = default;

    void addScreen(IScreenUI* screen);      // kutsutaan ennen begin()
    void begin();                           // lataa ensimmäisen näytön, asettaa _initialized
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

**Karouselli** — triviaali indeksilogiikka:

```cpp
uint8_t nextIdx()     const { return (_current + 1)                  % _screen_count; }
uint8_t previousIdx() const { return (_current + _screen_count - 1) % _screen_count; }
```

**Guard tyhjää listaa varten** — `switchNext` ja `switchPrevious` palauttavat heti jos näyttöjä on 0 tai 1:

```cpp
void ScreenManager::switchNext() {
    if (_screen_count <= 1) return;
    switchTo(nextIdx(), Direction::CW);
}
void ScreenManager::switchPrevious() {
    if (_screen_count <= 1) return;
    switchTo(previousIdx(), Direction::CCW);
}
```

**`switchTo()`:**

```cpp
void ScreenManager::switchTo(uint8_t index, Direction dir) {
    if (!_initialized || index == _current) return;
    onLeavingCurrentScreen();               // kutsuu _screens[_current]->onLeave()
    _current = index;
    lv_obj_t* target = _screens[index]->getLvglScreen();
    auto anim = (dir == Direction::CW)
        ? LV_SCR_LOAD_ANIM_MOVE_LEFT
        : LV_SCR_LOAD_ANIM_MOVE_RIGHT;
    lv_scr_load_anim(target, anim, ANIM_DURATION_MS, 0, false);
    _screens[index]->onEnter();
}
```

**`begin()` — vain ScreenManagerin oma alustus:**

```cpp
void ScreenManager::begin() {
    if (_screen_count == 0) return;
    _initialized = true;
    lv_scr_load(_screens[0]->getLvglScreen());  // lataa ensimmäinen näyttö animaatiotta
    _screens[0]->onEnter();
}
```

ScreenManager::begin() EI kutsu yksittäisten näyttöjen `begin()`-metodia. CrowPanelApplication
vastaa jokaisen UI-luokan `begin()`-kutsusta erikseen ennen `addScreen()`-rekisteröintiä.

---

### `CrowPanelApplication` — yksinkertaistuu merkittävästi
`CrowPanelApplication.h` — muutokset jäsenmuuttujiin:
- `CompassUI _compassUI` → `CompassUI _compassUI{_receiver}` (receiver konstruktorissa)
- `BrightnessUI _brightnessUI` → `BrightnessUI _brightnessUI{PWM_CHANNEL}`
- `ScreenManager _screenMgr` → ei enää parametreja konstruktorissa

`CrowPanelApplication.cpp` — `begin()` kutsuu jokaisen UI:n `begin()` ensin, sitten rekisteröi:

```cpp
void CrowPanelApplication::begin() {
    // ... olemassa olevat init-kutsut (pcf, backlight, display, lvgl, receiver) ...
    _compassUI.begin();
    _attitudeUI.begin();
    _brightnessUI.begin();              // käyttää _pwm_channel-jäsenmuuttujaa
    _screenMgr.addScreen(&_compassUI);
    _screenMgr.addScreen(&_attitudeUI);
    _screenMgr.addScreen(&_brightnessUI);
    _screenMgr.begin();                 // lataa ensimmäisen näytön
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

**Huom — `_receiver.updateStats()` (PPS-laskuri):** Nykyisessä koodissa kutsutaan
`handleUIUpdate()`-metodista. Pull-mallissa UI-luokat eivät kutsu `updateStats()`. Siirretään
kutsu `handleDiagnostics()`-metodiin — PPS päivitetään juuri ennen diagnostiikkatulosteen
kirjoittamista, mikä on riittävä tarkkuus.

Poistetaan: `isCompassActive(), isAttitudeActive(), isBrightnessActive()` kaikki käyttökohdat.

---

### Uuden näytön lisääminen Vaiheen 1 jälkeen
Lisäys vaatii vain nämä muutokset:
1. Luo `BatteryUI.h/.cpp : public IScreenUI` (toteuta `begin, getLvglScreen, update`, …)
2. Lisää `BatteryUI _batteryUI{...};` jäsenmuuttuja `CrowPanelApplication.h`:iin
3. Kutsu `_batteryUI.begin()` + `_screenMgr.addScreen(&_batteryUI)` `begin()`:iin
4. SquareLine Studio: lisää `ui_BatteryScreen`-näyttöobjekti

Karouselliin ei kosketa.

---

## Vaihe 2 — Multi-Sender ESP-NOW (toteutetaan myöhemmin)

Toteutetaan multi-sender käärimällä varsinainen ESP-NOW paketti kehykseen, jonka header
kertoo paketin tyypin ja jonka varsinainen payload sisältää itse datan (esim. `HeadingDelta`).

**HUOM — koordinoitu muutos:** Vaihe 2 vaatii samanaikaisen päivityksen sekä CrowPaneliin
että CMPS14-ESP32-SignalK-gateway -lähettimeen. Lähettimen on lähetettävä kaikki paketit
uudessa `EspNowPacket<T>`-muodossa, ja sen on osattava vastaanottaa `EspNowPacket<LevelCommand>`.

---

### Protokollarakenne

**EspNowHeader** — 8 tavua, kiinteä:

```
offset  0: magic       (uint32_t, 4 tavua) — tunnistaa omat paketit muiden ESP-NOW-laitteiden paketeilta
offset  4: msg_type    (uint8_t,  1 tavu)  — EspNowMsgType
offset  5: payload_len (uint8_t,  1 tavu)  — max 250 (ESP-NOW raja), uint8_t riittää
offset  6: reserved    (uint8_t[2])        — täyttö: payload alkaa offsetista 8, floatit 4-tavun rajalla ✓
```

Header on 8 tavua → `HeadingDelta`-floatit alkavat offsetista 8 (4-tavun rajalla).
Headerissa käytetään `uint8_t payload_len` (ei `uint16_t`) koska ESP-NOW-paketin maksimikoko
on 250 tavua — `uint8_t` riittää eikä aiheuta alignment-ongelmia.

**Pakettikoot (ESP-NOW max 250 tavua):**

| Paketti | Koko |
|---------|------|
| `EspNowPacket<HeadingDelta>` | 8 + 16 = 24 tavua |
| `EspNowPacket<BatteryDelta>` | 8 + 20 = 28 tavua |
| `EspNowPacket<WeatherDelta>` | 8 + 12 = 20 tavua |
| `EspNowPacket<LevelCommand>` | 8 + 8  = 16 tavua |
| `EspNowPacket<LevelResponse>`| 8 + 8  = 16 tavua |

---

### Päivitetty `espnow_protocol.h`

```cpp
#pragma once
#include <Arduino.h>
#include <cmath>
#include <cstring>

// =======================
// Protocol constants
// =======================

// Magic tunnistaa omat paketit muiden ESP-NOW-laitteiden paketeilta
// (merimittaristossa voi olla useita ESP-NOW-laitteita: Victron, autopilootti jne.)
static constexpr uint32_t ESPNOW_MAGIC = 0x45534E57; // 'E''S''N''W'

// Viestityypit (lisätään listaa kun tulee uusia antureita)
enum class EspNowMsgType : uint8_t {
    HEADING_DELTA   = 1,
    BATTERY_DELTA   = 2,
    WEATHER_DELTA   = 3,
    LEVEL_COMMAND   = 10,
    LEVEL_RESPONSE  = 11,
};

// =======================
// Protocol header
// =======================

// Kiinteä 8-tavun header kaikkien viestien eteen.
// uint8_t payload_len: ESP-NOW max payload 250 t, uint8_t riittää.
// reserved[2]: täyttö → header 8 tavua, payload alkaa 4-tavun rajalla (floatit tasattu).
struct EspNowHeader {
    uint32_t magic;           // ESPNOW_MAGIC
    uint8_t  msg_type;        // EspNowMsgType
    uint8_t  payload_len;     // payloadin pituus tavuina (max 250)
    uint8_t  reserved[2];     // täyttö, aseta nollaksi
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
    float house_current;   // amps, negatiivinen = lataus
    float house_power;     // watts
    float house_soc;       // percent
    float start_voltage;   // volts, starttiaakku
};

// --- Weather delta (float) ---
struct WeatherDelta {
    float temperature_c;    // °C
    float humidity_p;       // percent
    float pressure_hpa;     // hPa
};

// --- Level command ---
struct LevelCommand {
    uint8_t magic[4];     // "LVLC" — redundantti Vaiheessa 2 (msg_type hoitaa tunnistuksen)
    uint8_t reserved[4];
};

// --- Level response ---
struct LevelResponse {
    uint8_t magic[4];     // "LVLR" — redundantti Vaiheessa 2 (msg_type hoitaa tunnistuksen)
    uint8_t success;
    uint8_t reserved[3];
};

// =======================
// Packet wrapper (header + payload)
// =======================

template <typename TPayload>
struct EspNowPacket {
    EspNowHeader hdr;
    TPayload     payload;
} __attribute__((packed));

// =======================
// Helpers
// =======================

inline void initHeader(EspNowHeader& h, EspNowMsgType type, uint8_t payload_len) {
    h.magic       = ESPNOW_MAGIC;
    h.msg_type    = static_cast<uint8_t>(type);
    h.payload_len = payload_len;
    h.reserved[0] = 0;
    h.reserved[1] = 0;
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

    uint16_t getHeadingMagDeg()  const { return heading_mag_x10 / 10; }
    uint16_t getHeadingTrueDeg() const { return heading_true_x10 / 10; }
    int16_t  getPitchDeg()       const { return pitch_x10 / 10; }
    int16_t  getRollDeg()        const { return roll_x10 / 10; }
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

---

### Sample: lähetyspää (kompassi, CMPS14)

```cpp
#include "espnow_protocol.h"

void sendHeadingDelta(const HeadingDelta& d) {
    EspNowPacket<HeadingDelta> pkt;
    initHeader(pkt.hdr, EspNowMsgType::HEADING_DELTA, sizeof(HeadingDelta));
    pkt.payload = d;
    esp_now_send(BROADCAST_ADDR, (uint8_t*)&pkt, sizeof(pkt));
}
```

---

### Sample: vastaanottopää (CrowPanel, `ESPNowReceiver::onDataRecv`)

```cpp
void ESPNowReceiver::onDataRecv(const uint8_t* mac, const uint8_t* data, int len) {
    // 1. Riittävä pituus headerille
    if (len < (int)sizeof(EspNowHeader)) return;

    const EspNowHeader* h = (const EspNowHeader*)data;

    // 2. Magic-tarkistus — hylkää muiden ESP-NOW-laitteiden paketit
    if (h->magic != ESPNOW_MAGIC) return;

    // 3. Kehyksen eheystarkistus
    const uint8_t* payload     = data + sizeof(EspNowHeader);
    int            payload_len = len  - (int)sizeof(EspNowHeader);
    if (h->payload_len != (uint8_t)payload_len) return;

    // 4. Reititys viestityypin mukaan
    switch (static_cast<EspNowMsgType>(h->msg_type)) {

        case EspNowMsgType::HEADING_DELTA: {
            if (h->payload_len != sizeof(HeadingDelta)) return;
            const HeadingDelta* d = (const HeadingDelta*)payload;
            HeadingData hd = convertDeltaToData(*d);
            portENTER_CRITICAL(&s_spinlock);
            s_latest_data  = hd;
            s_has_new_data = true;
            s_last_rx_millis = millis();
            s_packet_count++;
            portEXIT_CRITICAL(&s_spinlock);
        } break;

        case EspNowMsgType::BATTERY_DELTA: {
            if (h->payload_len != sizeof(BatteryDelta)) return;
            // const BatteryDelta* b = (const BatteryDelta*)payload;
            // TODO: tallenna s_latest_battery_data (Vaihe 2)
        } break;

        case EspNowMsgType::LEVEL_RESPONSE: {
            if (h->payload_len != sizeof(LevelResponse)) return;
            const LevelResponse* resp = (const LevelResponse*)payload;
            portENTER_CRITICAL(&s_spinlock);
            s_level_response_received = true;
            s_level_response_success  = (resp->success == 1);
            portEXIT_CRITICAL(&s_spinlock);
        } break;

        default:
            break;
    }
}
```

---

## Yhteenveto muutettavista tiedostoista

### Vaihe 1

| Tiedosto | Muutos |
|----------|--------|
| `IScreenUI.h` | UUSI — abstrakti kantaluokka |
| `ScreenManager.h/.cpp` | Suuri refaktorointi — array + index, poistaa switch + tyypitetyt viitteet |
| `CompassUI.h/.cpp` | Lisää `ESPNowReceiver&` konstruktoriparametriksi, toteuttaa `IScreenUI`, pull-malli |
| `AttitudeUI.h/.cpp` | Toteuttaa `IScreenUI`, yhdistää `update` + `levelState`, lisää `onLeave` |
| `BrightnessUI.h/.cpp` | Siirrä `pwm_channel` konstruktoriparametriksi, toteuttaa `IScreenUI` |
| `CrowPanelApplication.h/.cpp` | `addScreen`-rekisteröinti, yksinkertaistetut `handle*`-metodit, PPS → `handleDiagnostics` |
| `ESPNowReceiver.h/.cpp` | Ei muutoksia Vaiheessa 1 |
| `RotaryEncoder.h/.cpp` | Ei muutoksia |

### Vaihe 2

| Tiedosto | Muutos |
|----------|--------|
| `espnow_protocol.h` | Suuri muutos: `EspNowHeader`, `ESPNOW_MAGIC`, `EspNowMsgType`, `EspNowPacket<T>`, `initHeader()` |
| `ESPNowReceiver.cpp` | `onDataRecv()`: header-pohjainen dispatch `switch(msg_type)` korvaa size-pohjaisen erottelun; `sendLevelCommand()`: lähettää `EspNowPacket<LevelCommand>` |
| `CrowPanelApplication.h/.cpp` | Ei muutoksia (ESPNowReceiver-rajapinta pysyy samana) |
| `CompassUI.h/.cpp` | Ei muutoksia (käyttää edelleen `HeadingData`) |
| `AttitudeUI.h/.cpp` | Ei muutoksia |
| `BrightnessUI.h/.cpp` | Ei muutoksia |
| CMPS14 sender | Lähettää `EspNowPacket<HeadingDelta>` ja `EspNowPacket<LevelResponse>`; parsii `EspNowPacket<LevelCommand>` |

---

## Verifikaatio

### Vaihe 1 — toiminnallisuuden tarkistus
1. **Käännös** — kääntyykö kaikki ilman varoituksia
2. **Perustoiminta** — kompassiruusu pyörii, pitch/roll päivittyy, kirkkaus toimii
3. **Karouselli** — CW/CCW navigointi kaikissa suunnissa
4. **Napin toiminnot** — T/M-vaihto, tasausdialogi, kirkkaussäätö
5. **Diagnostiikka** — `[DIAG]`-rivit tulostuvat 5 s välein (PPS, UI updates, LVGL avg/max)
6. **Yhteyden katkeaminen** — disconnected-tila näkyy kun lähettäjä sammutetaan
7. **Uuden näytön lisäys** — lisää tyhjä `TestUI : public IScreenUI`, rekisteröi, varmista että karouselli toimii 4 näytöllä ilman muita muutoksia

### Vaihe 2 — multi-sender tarkistus
1. **Magic-suodatus** — lähetä testi-paketti ilman ESPNOW_MAGICia → ei vaikutusta UI:hin
2. **Tuntematon msg_type** — lähetä paketti tuntemattomalla tyypillä → `default:`-haara, ei kaatu
3. **HeadingDelta** — kompassidata saapuu uudessa `EspNowPacket<HeadingDelta>`-muodossa, näyttö toimii
4. **LevelCommand/Response** — tasaustoiminto toimii end-to-end uudella kehysmuodolla
5. **Pakettikoot** — varmista `sizeof(EspNowPacket<HeadingDelta>) == 24` jne. `static_assert`:lla
