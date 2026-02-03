# Claude Session Context - ESP32 CrowPanel Compass

**Päivämäärä:** 2026-02-02 (päivitetty)
**Projekti:** ESP32-Crowpanel-compass
**Hardware:** Elecrow CrowPanel 2.1" HMI (ESP32-S3, 480x480 IPS, Rotary Knob)

---

## Projektin yleiskuvaus

MVP-toteutus merenkulkuinstrumentista, joka vastaanottaa CMPS14-kompassilta ESP-NOW broadcast-viestejä ja näyttää heading/pitch/roll arvot LVGL-käyttöliittymällä pyöreällä kosketusnäytöllä.

### Näytöt

1. **CompassScreen** - Kompassiruusu ja heading-arvo
2. **AttitudeScreen** - Keinohorisontti (valkoinen viiva) ja pitch/roll arvot

---

## Tässä sessiossa toteutettu ja testattu

### 1. AttitudeScreen - toimiva versio

**Lopullinen toteutus:**
- Horisonttiviiva: 680×4 px valkoinen PNG (`ui_img_horizonline_png.c` = 41 KB)
- ContainerHorizonGroup: 680×680 px
- Musta tausta, valkoinen viiva liikkuu pitch/roll mukaan
- Laivan siluetti pysyy paikallaan (ContainerAttitudeGroup)
- Kulkuvalot: PanelPortside (punainen), PanelStarboard (vihreä)

**Horisontin liike (AttitudeUI.cpp):**
```cpp
// PITCH: Siirrä viivaa Y-suunnassa
int16_t y_offset = (pitch_x10 * PITCH_SCALE) / 10;
lv_obj_set_y(ui_ImageHorizon, y_offset);

// ROLL: Rotaatio kuvan keskipisteen ympäri
lv_img_set_pivot(ui_ImageHorizon, 340, 2);  // 680×4 kuvan keskipiste
lv_img_set_angle(ui_ImageHorizon, -roll_x10);
```

### 2. Kompassiruusun optimointi

**Muutokset:**
- PNG pienennetty 480×480 → 360×360 px
- C-array koko: 3.3 MB → 1.9 MB
- SquareLinessa Scale = 341 (skaalaa 360→480)
- Pivot X/Y = 0 (LVGL käyttää automaattisesti kuvan keskipistettä)

### 3. Rotary Encoder -näytönvaihto

Toimii: knobin kääntö vaihtaa Compass ↔ Attitude näyttöjen välillä.

### 4. Ratkaistut ongelmat

| Ongelma | Ratkaisu |
|---------|----------|
| Iso horizon-kuva (13 MB) ei mahtunut flashiin | Korvattu 680×4 px viivalla (41 KB) |
| Panel-objektien rotaatio ei toiminut | Vaihdettu Image-objektiin + lv_img_set_angle() |
| Musta alue näkyi pitch-liikkeessä | Siirrytty pelkkään viivaan, ei tarvita sky/sea |
| Sketch liian suuri (4.5 MB > 3.1 MB max) | Kevennetty kuvia |

---

## Nykyinen tiedostokoko

| Tiedosto | Koko |
|----------|------|
| `ui_img_1779767769.c` (kompassiruusu) | 1.9 MB |
| `ui_img_arrow_png.c` | 61 KB |
| `ui_img_horizonline_png.c` | 41 KB |

---

## Seuraavat vaiheet / TODO

1. **Kompassiruusun lisäkevennys** - tavoite pudottaa vielä pienemmäksi
2. **Diagnostiikka nykimiselle** - lisätään pakettilaskuri ja aikaleimat
3. **Testaus** - selvitetään pullonkaula (ESP-NOW pakettihävikki vs LVGL piirto)

---

## Arkkitehtuuri

```
┌─────────────────────────────────────────────────────────┐
│                    Main Loop                            │
├─────────────────────────────────────────────────────────┤
│  ┌──────────────┐  ┌──────────────┐  ┌──────────────┐  │
│  │RotaryEncoder │  │ESPNowReceiver│  │ScreenManager │  │
│  └──────┬───────┘  └──────┬───────┘  └──────┬───────┘  │
│         │                 │                  │          │
│         ▼                 ▼                  ▼          │
│  ┌────────────────────────────────────────────────┐    │
│  │                 HeadingData                    │    │
│  └────────────────────────────────────────────────┘    │
│         │                                    │          │
│         ▼                                    ▼          │
│  ┌──────────────┐                    ┌──────────────┐  │
│  │  CompassUI   │                    │  AttitudeUI  │  │
│  └──────────────┘                    └──────────────┘  │
└─────────────────────────────────────────────────────────┘
```

---

## UI-rakenne (SquareLine Studio)

### CompassScreen
```
ui_CompassScreen
├── ui_PanelTop (valkoinen pohja)
│   ├── ui_PanelCompassRose
│   │   └── ui_ImageCompassRose (360×360, Scale=341)
│   ├── ui_PanelArrow + ui_ImageArrow
│   ├── ui_PanelHeading + ui_LabelHeading
│   ├── ui_PanelHeadingMode + ui_LabelHeadingMode
│   └── ui_PanelConnected
```

### AttitudeScreen
```
ui_AttitudeScreen (musta tausta)
├── ui_ContainerHorizonGroup (680×680)
│   └── ui_ImageHorizon (680×4 valkoinen viiva)
│
└── ui_ContainerAttitudeGroup (484×484, pysyy paikallaan)
    ├── ui_PanelPitch + ui_LabelPitchTitle + ui_LabelPitch
    ├── ui_PanelRoll + ui_LabelRollTitle + ui_LabelRoll
    ├── ui_PanelHull (laivan runko)
    ├── ui_PanelDeck
    ├── ui_PanelBridge
    ├── ui_PanelMast
    ├── ui_PanelPortside (punainen kulkuvalo, vasen)
    └── ui_PanelStarboard (vihreä kulkuvalo, oikea)
```

---

## Tärkeät huomiot

### SquareLine Studio Export
**VAROITUS:** SquareLine Studio tyhjentää export-hakemiston kokonaan! Tee aina git commit tai varmuuskopio ennen exportia.

### LVGL Scale-arvo
- 256 = 100% (alkuperäinen koko)
- Kaava: (haluttu_koko / alkuperäinen_koko) × 256
- Esim: 480/360 × 256 = 341

### LVGL Pivot
- Pivot X/Y = 0 → LVGL käyttää automaattisesti kuvan keskipistettä
- Tai aseta eksplisiittisesti: `lv_img_set_pivot(img, width/2, height/2)`

### Horisontin matematiikka
- Pitch < 0 (keula alas) → viiva siirtyy ylös (y negatiivinen)
- Roll < 0 (kallistus vasemmalle) → viiva kallistuu myötäpäivään

---

## Fontit käytössä

| Fontti | Käyttö |
|--------|--------|
| `ui_font_FontHeading96b` | CompassScreen: heading |
| `ui_font_FontHeading64b` | CompassScreen: T/M mode |
| `ui_font_FontAttitude84c` | AttitudeScreen: pitch/roll (sisältää +) |
| `ui_font_FontAttitudeTitle24` | AttitudeScreen: "Pitch", "Roll" |

---

## Tiedostoluettelo

```
ESP32-Crowpanel-compass/
├── ESP32-Crowpanel-compass.ino  # Pääohjelma
├── HeadingData.h                # Datarakenteet
├── ESPNowReceiver.h/.cpp        # ESP-NOW vastaanotin
├── CompassUI.h/.cpp             # Kompassi-näytön adapter
├── AttitudeUI.h/.cpp            # Keinohorisontti-näytön adapter
├── RotaryEncoder.h/.cpp         # Rotary encoder -käsittelijä
├── ScreenManager.h/.cpp         # Näyttöjen hallinta
├── secrets.h                    # WiFi credentials (ei gitissä)
├── .gitignore
├── ui.h/.c                      # SquareLine generoitu
├── ui_CompassScreen.h/.c        # SquareLine generoitu
├── ui_AttitudeScreen.h/.c       # SquareLine generoitu
├── ui_helpers.h/.c              # SquareLine generoitu
├── ui_font_*.c                  # Fontit
├── ui_img_*.c                   # Kuvat
├── UI/                          # SquareLine projekti
└── RotaryScreen_2_1/            # Elecrow demo (referenssi)
```
