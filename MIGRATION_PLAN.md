# Migration plan

## Context

Projekti käyttää LVGL 8.3.6 ja ESP32 Board Package 2.0.14 — molemmat merkittävästi jäljessä viimeisimmistä versioista (LVGL ~9.3, ESP32 Core ~3.3). Molemmat kirjastot voidaan päivittää toisistaan riippumatta; suositus on tehdä ESP32 Core ensin (vähemmän muutoksia), sitten LVGL. Muutokset ovat laajuudeltaan kohtalaisia: 2 kriittistä API- muutosta Core-puolella, ja LVGL:n puolella laajempi driver-API-uudelleenkirjoitus + SquareLine Studio -uudelleenexport.

## Vaihejako

| Vaihe	| Sisältö | Muutoksia | Riski |
|-------|---------|-----------|-------|
| A	| ESP32 Core 2.0.14 → 3.x | 4 paikassa | Matala|
| B	| LVGL 8.3.6 → 9.x | ~10 paikassa + SLS re-export | Keskisuuri |

### Vaihe A — ESP32 Core 2.0.14 → 3.x

#### A1. LEDC PWM — API muuttui täysin (kriittinen)

Kriittiset tiedostot:
- `CrowPanelApplication.cpp` — `initBacklight()`
- `BrightnessUI.cpp` — `applyBrightness()`

Vanha API (Core 2.x):

```cpp
ledcSetup(PWM_CHANNEL, PWM_FREQ, PWM_RESOLUTION);  // palauttaa int
ledcAttachPin(SCREEN_BACKLIGHT_PIN, PWM_CHANNEL);
ledcWrite(PWM_CHANNEL, duty);                        // kanava ensimmäisenä
```

Uusi API (Core 3.x):

```cpp
ledcAttach(SCREEN_BACKLIGHT_PIN, PWM_FREQ, PWM_RESOLUTION);  // yhdistetty
ledcWrite(SCREEN_BACKLIGHT_PIN, duty);                        // PIN ensimmäisenä
```

Muutokset:
- `CrowPanelApplication.cpp::initBacklight()`: 3 riviä → 2 riviä (poista `ledcSetup`, muuta `ledcAttachPin` → `ledcAttach`, muuta `ledcWrite(channel, ...)` → `ledcWrite(pin, ...)`)
- `BrightnessUI.cpp::applyBrightness()`: `ledcWrite(_pwm_channel, ...)` → `ledcWrite(SCREEN_BACKLIGHT_PIN, ...)`
  - Huom: `_pwm_channel` jäsenmuuttujaa ei enää tarvita Core 3.x:ssä — kirjoitetaan PIN-numerolla
  - `BrightnessUI.h`: `_pwm_channel` voidaan poistaa tai muuttaa vakioksi
  - Konstruktoriparametri `int pwm_channel` voidaan korvata `uint8_t backlight_pin`:llä selkeyden vuoksi

#### A2. ESP-NOW receive callback -signatuuri (kriittinen)

Kriittinen tiedosto: `ESPNowReceiver.cpp` + `ESPNowReceiver.h`

Vanha (Core 2.x):

```cpp
static void onDataRecv(const uint8_t* mac_addr, const uint8_t* data, int data_len);
```

Uusi (Core 3.x):

```cpp
static void onDataRecv(const esp_now_recv_info_t* recv_info, const uint8_t* data, int data_len);
```

Muutokset:
- `ESPNowReceiver.h`: päivitä callback-deklaraatio
- `ESPNowReceiver.cpp`: päivitä callback-määrittely (funktion signatuuri)
- Funktion runko ei tarvitse muutoksia — `mac_addr` ei ole käytössä `onDataRecv()` -rungossa

Huom: CMPS14-lähettäjä käyttää jo Core 3.x -signatuuria — CrowPanel jää jälkeen tässä.

#### A3. `Arduino_GFX_Library` — todennäköisesti yhteensopiva

`Arduino_ESP32RGBPanel` ja `Arduino_ST7701_RGBPanel` konstruktorit saattavat muuttua kirjaston uusissa versioissa. Jos ei käänny, katso Arduino_GFX esimerkkitiedostot uudella versiolla. Uusin versio on 1.6.5.

Seuraa: `CrowPanelApplication.cpp` konstruktori-init-lista (rivit ~37–47).

### Vaihe B — LVGL 8.3.6 → 9.x

#### B1. Display driver API — täysin uudelleenkirjoitettu (kriittinen)

Kriittinen tiedosto: `CrowPanelApplication.h` + `CrowPanelApplication.cpp::initLvgl()`

**B1a. Header-muutokset (`CrowPanelApplication.h`)**

Poista:

```cpp
lv_disp_draw_buf_t _draw_buf;   // rakenne poistunut LVGL 9:stä
lv_color_t *_buf1 = nullptr;    // tyyppi muuttuu
```

Tilalle:

```cpp
lv_display_t* _lvgl_disp = nullptr;
uint8_t* _buf1 = nullptr;       // draw buffer on nyt uint8_t* LVGL 9:ssä
```

**B1b. `initLvgl()` — LVGL 8 vs. LVGL 9**

Vanha (LVGL 8.x):

```cpp
lv_init();
_buf1 = (lv_color_t*)heap_caps_malloc(sizeof(lv_color_t) * BUF_PIXELS, MALLOC_CAP_INTERNAL);
lv_disp_draw_buf_init(&_draw_buf, _buf1, NULL, BUF_PIXELS);
static lv_disp_drv_t disp_drv;
lv_disp_drv_init(&disp_drv);
disp_drv.hor_res    = SCREEN_WIDTH;
disp_drv.ver_res    = SCREEN_HEIGHT;
disp_drv.flush_cb   = lvglFlushCb;
disp_drv.draw_buf   = &_draw_buf;
disp_drv.user_data  = &_gfx;
lv_disp_drv_register(&disp_drv);
```

Uusi (LVGL 9.x):

```cpp
lv_init();
_buf1 = (uint8_t*)heap_caps_malloc(sizeof(lv_color_t) * BUF_PIXELS, MALLOC_CAP_INTERNAL);
_lvgl_disp = lv_display_create(SCREEN_WIDTH, SCREEN_HEIGHT);
lv_display_set_flush_cb(_lvgl_disp, lvglFlushCb);
lv_display_set_buffers(_lvgl_disp, _buf1, NULL,
                       sizeof(lv_color_t) * BUF_PIXELS,
                       LV_DISPLAY_RENDER_MODE_PARTIAL);
lv_display_set_user_data(_lvgl_disp, &_gfx);
// Jos värit vaihtavat tavujärjestyksen (LV_COLOR_16_SWAP oli käytössä):
// lv_display_set_color_format(_lvgl_disp, LV_COLOR_FORMAT_RGB565_SWAPPED);
```

#### B2. Flush callback -signatuuri (kriittinen)

Kriittinen tiedosto: `CrowPanelApplication.cpp` (rivit ~10–31)

Vanha (LVGL 8.x):

```cpp
static void lvglFlushCb(lv_disp_drv_t* disp, const lv_area_t* area, lv_color_t* color_p) {
    auto* gfx = static_cast<Arduino_ST7701_RGBPanel*>(disp->user_data);
    // ...
    gfx->draw16bitRGBBitmap(area->x1, area->y1, (uint16_t*)&color_p->full, w, h);
    lv_disp_flush_ready(disp);
}
```

Uusi (LVGL 9.x):

```cpp
static void lvglFlushCb(lv_display_t* disp, const lv_area_t* area, uint8_t* px_map) {
    auto* gfx = static_cast<Arduino_ST7701_RGBPanel*>(lv_display_get_user_data(disp));
    // ...
    gfx->draw16bitRGBBitmap(area->x1, area->y1, (uint16_t*)px_map, w, h);
    lv_display_flush_ready(disp);
}
```

Muutokset:
- `lv_disp_drv_t*` → `lv_display_t*`
- `lv_color_t* color_p` → `uint8_t* px_map`
- `disp->user_data` → `lv_display_get_user_data(disp)`
- `color_p->full` → `px_map` (suora `uint16_t` -cast)
- `lv_disp_flush_ready()` → `lv_display_flush_ready()`

#### B3. Image widget API — nimetty uudelleen (`CompassUI` + `AttitudeUI`)

Kriittiset tiedostot: `CompassUI.cpp`, `AttitudeUI.cpp`

| LVGL 8.x | LVGL 9.x. |
|----------|-----------|
| `lv_img_set_zoom(img, 512)` | `lv_image_set_scale(img, 512)` |
| `lv_img_set_angle(img, angle)` | `lv_image_set_rotation(img, angle)` |
| `lv_img_set_antialias(img, false)` | `lv_image_set_antialias(img, false) (tarkista)` |
| `lv_img_set_pivot(img, x, y)` | `lv_image_set_pivot(img, x, y)` |

Huom: kulmaparametrin arvoasteikko (0.1° yksikköä) säilyy — laskentalogiikka ei muutu.

Kaikki kutsut:
- `CompassUI.cpp`: `lv_img_set_antialias (begin)`, `lv_img_set_angle (setCompassRotation)`
- `AttitudeUI.cpp`: `lv_img_set_pivot (begin)`, `lv_img_set_angle (updateHorizon, x2)`

#### B4. `lv_conf.h` — uudelleengenerointi tarpeen

LVGL 9:n `lv_conf.h` rakenne muuttui merkittävästi:
- `LV_COLOR_16_SWAP` poistettu — korvataan `lv_display_set_color_format()` -kutsulla
- Lukuisia optioita nimetty uudelleen tai siirretty

Käytännössä: kopio LVGL 9:n `lv_conf_template.h`, mukauta projektin tarpeet. Tärkeimmät säilytettävät asetukset:
- `LV_TICK_CUSTOM 1` (ja `LV_TICK_CUSTOM_SYS_TIME_EXPR (millis())`)
- Fontit ja muistiasetukset

#### B5. SquareLine Studio — UI-tiedostojen uudelleenexport (merkittävin työvaihe)

Kaikki SquareLine-generoidut tiedostot tuottavat LVGL 8.x -koodia (käyttävät `lv_img_*`, `LV_IMG_DECLARE` jne.). Ne täytyy uudelleengeneroidaan LVGL 9.x -kohteella.

Toimenpiteet:

1. Avaa UI/-projekti SquareLine Studio 1.6.0:ssa
2. Vaihda projektin LVGL-kohdeversio 9.x:ään (Project Settings → LVGL version)
3. Exportoi — SquareLine generoi `lv_image_*` -kutsut automaattisesti
4. Uudelleengeneroituvat tiedostot:
    * ui.h / ui.c
    * ui_CompassScreen.h / .c
    * ui_AttitudeScreen.h / .c
    * ui_BrightnessScreen.h / .c
    * ui_WeatherScreen.h / .c
    * ui_helpers.h / .c

VAROITUS: SquareLine Studio tyhjentää export-hakemiston kokonaan — git commit ennen exportia.

### Muutoskohdat yhteenvetona

| Tiedosto | Vaihe | Muutos |
|----------|-------|--------|
| `CrowPanelApplication.cpp` | A1 | `ledcSetup`/`AttachPin` → `ledcAttach`; `ledcWrite(channel,...)` → `ledcWrite(pin,...)` |
| `BrightnessUI.cpp` | A1 | `ledcWrite(_pwm_channel,...)` → `ledcWrite(BACKLIGHT_PIN,...)` |
| `BrightnessUI.h` | A1 | `_pwm_channel` → `_backlight_pin` (tai poisto) | 
| `ESPNowReceiver.h` | A2 | Callback-deklaraation signatuuri |
| `ESPNowReceiver.cpp` | A2 | Callback-funktion signatuuri |
| `CrowPanelApplication.h` | B1 | `lv_disp_draw_buf_t` + `lv_color_t*` → `lv_display_t*` + `uint8_t*` |
| `CrowPanelApplication.cpp` | B1+B2 | `initLvgl()` + `lvglFlushCb()` uudelleenkirjoitus |
| `CompassUI.cpp` | B3 | `lv_img_*` → `lv_image_*` (2 kohtaa) | 
| `AttitudeUI.cpp` | B3 | `lv_img_*` → `lv_image_*` (3 kohtaa) | 
| `lv_conf.h` | B4 | Uudelleengenerointi LVGL 9 -templatesta | 
| `ui*.h/.c` (6 kpl) | B5 | SquareLine Studio re-export LVGL 9 -kohteella | 

### Verifikaatio

#### Vaihe A jälkeen

1. Käännös ilman varoituksia
2. Taustavalo syttyy oikealla kirkkaudella
3. Kirkkaussäätö toimii (arc, NVS-tallennus)
4. ESP-NOW yhteys muodostuu, PPS > 0 diagnostiikassa

#### Vaihe B jälkeen

1. Käännös ilman varoituksia
2. Kaikki kolme näyttöä renderöityvät oikein
3. Kompassiruusu pyörii (`lv_image_set_rotation` toimii)
4. Keinohorisontti liikkuu pitch/roll datan mukaan
5. Näyttöjen välinen siirtymäanimaatio (`lv_scr_load_anim`) toimii
6. Diagnostiikka: LVGL max vertailukelpoinen aiempaan (~164 ms)
7. Suorituskykyvertailu: `lv_image_set_rotation()` vs. vanha `lv_img_set_angle()`

### Huomioita

- RotaryScreen_2_1/ (Elecrow demo) ei käänny uusilla versioilla — tämä tiedetään jo eikä vaikuta projektiin (Arduino IDE ei käännä sitä)
- Suoritustehoero: LVGL 9:n renderöijä on optimoitu — kompassiruusun kiertoaika saattaa muuttua, mitataan uudelleen
- `LV_COLOR_16_SWAP`: tarkista onko se aktiivinen `lv_conf.h` :ssa ennen B-vaiheen aloitusta; jos on, muista lisätä `lv_display_set_color_format(...RGB565_SWAPPED)` B1:n yhteydessä
- LVGL 9 `lv_conf.h`: `LV_TICK_CUSTOM=1` on projektin kannalta kriittinen asetus — varmista säilyminen uudessa konfissa

---

## Toteutunut migraatio — mitä oikeasti muutettiin

**Lopputulos:** LVGL 9.5.0 + ESP32 Core 3.3.7 + Arduino_GFX 1.6.5 — toimii ✅

Alla kuvataan kaikki todelliset muutokset suunniteltuun verrattuna. Useita kohtia poikkeaa alkuperäisestä suunnitelmasta.

---

### Vaihe A — ESP32 Core 2.0.14 → 3.3.7 ✅

Toteutui suunnitelman mukaisesti.

**A1. LEDC PWM** (`CrowPanelApplication.cpp::initBacklight`, `BrightnessUI.cpp`): toteutui kuten suunniteltu.

**A2. ESP-NOW callback** (`ESPNowReceiver.h/.cpp`): toteutui kuten suunniteltu.

**A3. Arduino_GFX 1.3.1 → 1.6.5** — poikkesi suunnitelmasta merkittävästi:

- `Arduino_ST7701_RGBPanel` **ei ole** Arduino Library Manager -versiossa 1.6.5 (vain GitHub-versiossa). Pakko siirtyä `Arduino_RGB_Display`-lähestymiseen.
- Uusi GFX-objektirakenne (`CrowPanelApplication.h`):
  ```cpp
  // Vanha (1.3.1):
  Arduino_ESP32RGBPanel _bus;
  Arduino_ST7701_RGBPanel _gfx;

  // Uusi (1.6.5):
  Arduino_SWSPI _init_bus;        // erillinen SPI-väylä init-komennoille
  Arduino_ESP32RGBPanel _bus;
  Arduino_RGB_Display _gfx;       // auto_flush=true
  ```
- `_gfx.begin()` palauttaa `bool` (1.6.5), ei `void` kuten `Arduino_ST7701_RGBPanel`:ssa.

---

### Vaihe B — LVGL 8.3.6 → 9.5.0 ✅

**B1+B2. Display driver + flush callback** — toteutui pääosin suunnitelman mukaan, mutta kolme kriittistä poikkeamaa:

**Poikkeama 1 — Buffer-tyyppi ja koko:**
```cpp
// Suunniteltu (virheellinen):
_buf1 = (uint8_t*)heap_caps_malloc(sizeof(lv_color_t) * BUF_PIXELS, ...);
//  sizeof(lv_color_t) == 3 LVGL 9:ssä (RGB888) → väärä koko!

// Toteutunut (oikea):
uint16_t* _buf1 = nullptr;  // tyyppi uint16_t*, ei uint8_t*
_buf1 = (uint16_t*)heap_caps_malloc(sizeof(uint16_t) * BUF_PIXELS, ...);
// sizeof(uint16_t) == 2 → oikea RGB565-koko, 480×120×2 = 115 200 B
```

**Poikkeama 2 — flush-callbackin cast-tyyppi:**
```cpp
// Suunniteltu (vanha tyyppi jäi):
auto* gfx = static_cast<Arduino_ST7701_RGBPanel*>(...);

// Toteutunut:
auto* gfx = static_cast<Arduino_RGB_Display*>(...);
```

**Poikkeama 3 — `lv_tick_set_cb` korvaa `LV_TICK_CUSTOM`:**
- `LV_TICK_CUSTOM` ei ole enää LVGL 9:n `lv_conf.h`:ssa
- Ticking hoidetaan koodissa: `lv_tick_set_cb(millis)` `initLvgl()`-funktiossa

**B3. Image widget API** (`CompassUI.cpp`, `AttitudeUI.cpp`): toteutui kuten suunniteltu.

**B4. `lv_conf.h`** — poikkesi suunnitelmasta:
- Kaikki asetukset (`LV_COLOR_DEPTH 16`, `LV_DRAW_BUF_STRIDE_ALIGN 1`, `LV_DRAW_SW_SUPPORT_RGB565_SWAPPED 1`, `LV_MEM_SIZE 64KB`) vastaavat LVGL 9:n oletusarvoja — lv_conf.h:ssa ei ole projektikohtaisia muutoksia
- Tiedosto siirretty projektin juureen (`lv_conf.h` repossa) — löytyy `__has_include`-mekanismilla automaattisesti ennen libraries/-kansion versiota

**B5. SquareLine Studio re-export**: toteutui kuten suunniteltu.

---

### Kriittinen GFX-ongelma ja ratkaisu — pystyviivakuvio

Suurin työ oli satunnaisen pystyviivakuvion poistaminen näytöltä. Juurisyy ja ratkaisu:

**Juurisyy: `_bus`-konstruktorin hsync/vsync polariteetti oli väärin**

```cpp
// Vanha (aiheutti satunnaisen pystyviivakuvion):
0, 10, 4, 20,   /* hsync: polarity, front, pulse, back */
0, 10, 4, 20),  /* vsync: polarity, front, pulse, back */

// Uusi (korjattu):
1, 10, 4, 20,   /* hsync: polarity, front, pulse, back */
1, 10, 4, 20),  /* vsync: polarity, front, pulse, back */
```

Väärä polariteetti aiheutti sen, että RGB-paneelin DMA luki framebufferia väärästä kohdasta suhteessa sync-signaaleihin.

**Projektikohtainen init-taulukko (`Crowpanel_ST7701_Init.h/.cpp`):**

GFX-kirjaston sisäinen `st7701_type5`-init-taulukko aiheutti väriongelman (värit BGR-järjestyksessä). Ratkaisu: oma projektikohtainen init-taulukko jossa kriittiset muutokset:

```cpp
WRITE_C8_D8, 0x36, 0x00, // BGR=0 → RGB (kirjastossa oli 0x08 = BGR)
WRITE_C8_D8, 0x3A, 0x60, // RGB666 (toistaiseksi — toimii vaikka käytetään RGB565)
```

**`begin()` -järjestys:** `initBacklight()` ennen `initDisplay()` (taustavalo päälle ennen paneeli-inittiä).

---

### Muutoskohdat — toteutunut

| Tiedosto | Vaihe | Muutos |
|----------|-------|--------|
| `CrowPanelApplication.cpp` | A1 | `ledcSetup`/`AttachPin` → `ledcAttach`; `ledcWrite(channel,...)` → `ledcWrite(pin,...)` |
| `BrightnessUI.cpp` | A1 | `ledcWrite(_pwm_channel,...)` → `ledcWrite(BACKLIGHT_PIN,...)` |
| `ESPNowReceiver.h/.cpp` | A2 | Callback-signatuuri `mac_addr*` → `esp_now_recv_info_t*` |
| `CrowPanelApplication.h` | A3+B1 | `Arduino_ST7701_RGBPanel` → `Arduino_SWSPI` + `Arduino_RGB_Display`; `uint16_t* _buf1` |
| `CrowPanelApplication.cpp` | A3+B1 | Konstruktori: uusi GFX-objektirakenne + `_bus` polariteetti → 1 |
| `CrowPanelApplication.cpp` | B1+B2 | `initLvgl()`: `sizeof(uint16_t)*BUF_PIXELS`, `lv_tick_set_cb(millis)` |
| `CrowPanelApplication.cpp` | B2 | `lvglFlushCb()`: `Arduino_RGB_Display*` cast, `lv_display_flush_ready()` |
| `Crowpanel_ST7701_Init.h/.cpp` | A3 | **Uusi tiedosto** — projektikohtainen ST7701 init-taulukko (`0x36=0x00` RGB) |
| `CompassUI.cpp` | B3 | `lv_img_*` → `lv_image_*` |
| `AttitudeUI.cpp` | B3 | `lv_img_*` → `lv_image_*` |
| `lv_conf.h` | B4 | Siirretty projektin juureen repoon; kaikki arvot = LVGL 9 oletuksia |
| `ui*.h/.c` (6 kpl) | B5 | SquareLine Studio re-export LVGL 9 -kohteella |


