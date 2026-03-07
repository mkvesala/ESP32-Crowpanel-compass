#include "CrowPanelApplication.h"

// === S T A T I C ===

static uint32_t s_flush_total = 0;
static uint32_t s_flush_max = 0;
static uint32_t s_flush_calls = 0;

// Static callback function for lvgl
static void lvglFlushCb(lv_disp_drv_t* disp, const lv_area_t* area, lv_color_t* color_p) {

    auto* gfx = static_cast<Arduino_ST7701_RGBPanel*>(disp->user_data);
    if (!gfx) {
        lv_disp_flush_ready(disp);
        return;
    }
    uint32_t t0 = micros();

    uint32_t w = (area->x2 - area->x1 + 1);
    uint32_t h = (area->y2 - area->y1 + 1);

    // gfx->draw16bitBeRGBBitmap(area->x1, area->y1, (uint16_t *)&color_p->full, w, h);
    gfx->draw16bitRGBBitmap(area->x1, area->y1, (uint16_t *)&color_p->full, w, h);

    uint32_t ft = micros() - t0;
    s_flush_total += ft;
    s_flush_calls++;
    if (ft > s_flush_max) s_flush_max = ft;

    lv_disp_flush_ready(disp);
}

// === P U B L I C ===

// Constructor
CrowPanelApplication::CrowPanelApplication():
    _bus(16, 2, 1,
        40, 7, 15, 41,
        46, 3, 8, 18, 17,
        14, 13, 12, 11, 10, 9,
        5, 45, 48, 47, 21),
    _gfx(&_bus, GFX_NOT_DEFINED, 0,
        false, 480, 480,
        st7701_type5_init_operations, sizeof(st7701_type5_init_operations),
        true,
        10, 4, 20,
        10, 4, 20),
    _pcf8574(0x21),
    _receiver(),
    _compassUI(_receiver),
    _attitudeUI(_receiver),
    _weatherUI(_receiver),
    _batteryUI(_receiver),
    _brightnessUI(PWM_CHANNEL),
    _encoder(_pcf8574),
    _screenMgr() {}

// Initialize
void CrowPanelApplication::begin() {

    this->initPcfAndResetLines();

    this->initDisplay();

    this->initBacklight(PWM_DUTY);

    this->initLvgl();

    // SquareLine UI init
    ui_init();

    // UI adapter init
    _compassUI.begin();
    _attitudeUI.begin();
    _weatherUI.begin();
    _batteryUI.begin();
    _brightnessUI.begin();

    // Register screens with manager (carousel order: COMPASS → ATTITUDE → WEATHER → BATTERY → BRIGHTNESS)
    _screenMgr.addScreen(&_compassUI);
    _screenMgr.addScreen(&_attitudeUI);
    _screenMgr.addScreen(&_weatherUI);
    _screenMgr.addScreen(&_batteryUI);
    _screenMgr.addScreen(&_brightnessUI);

    // Screen manager init (loads first screen)
    _screenMgr.begin();

    // Rotary encoder init
    _encoder.begin();

    // ESP-NOW init
    _receiver.begin(ESP_NOW_CHANNEL);

}

// Loop
void CrowPanelApplication::loop() {

    const uint32_t now = millis();

    // LVGL tick
    this->handleLvglTick(now);

    // Update statistics
    _receiver.updateStats();

    // Knob rotation
    this->handleKnobRotation();

    // Button press
    this->handleKnobButtonPress();

    // UI update
    this->handleUIUpdate(now);

    // Diagnostics print
    this->handleDiagnostics(now);

}

// === P R I V A T E ===

// PCF init
void CrowPanelApplication::initPcfAndResetLines() {
    Wire.begin(I2C_SDA_PIN, I2C_SCL_PIN);

    _pcf8574.pinMode(PCF_TP_RST, OUTPUT);
    _pcf8574.pinMode(PCF_TP_INT, OUTPUT);
    _pcf8574.pinMode(PCF_LCD_PWR, OUTPUT);
    _pcf8574.pinMode(PCF_LCD_RST, OUTPUT);
    _pcf8574.pinMode(PCF_RE_BTN, INPUT_PULLUP);

    _pcf8574.begin();

    // LCD power on
    _pcf8574.digitalWrite(PCF_LCD_PWR, HIGH);
    delay(100);

    // LCD reset sequence
    _pcf8574.digitalWrite(PCF_LCD_RST, HIGH);
    delay(50);
    _pcf8574.digitalWrite(PCF_LCD_RST, LOW);
    delay(120);
    _pcf8574.digitalWrite(PCF_LCD_RST, HIGH);
    delay(120);

    // Touch reset lines
    _pcf8574.digitalWrite(PCF_TP_RST, HIGH);
    delay(50);
    _pcf8574.digitalWrite(PCF_TP_RST, LOW);
    delay(120);
    _pcf8574.digitalWrite(PCF_TP_RST, HIGH);
    delay(120);

    _pcf8574.digitalWrite(PCF_TP_INT, HIGH);
    delay(50);
}

// Display init
void CrowPanelApplication::initDisplay() {
    _gfx.begin();
    _gfx.fillScreen(BLACK);
}

// Screen backlight
void CrowPanelApplication::initBacklight(uint8_t duty) {
    ledcSetup(PWM_CHANNEL, PWM_FREQ, PWM_RESOLUTION);
    ledcAttachPin(SCREEN_BACKLIGHT_PIN, PWM_CHANNEL);
    ledcWrite(PWM_CHANNEL, duty);
}

// LVGL init
void CrowPanelApplication::initLvgl() {

    lv_init();

    _buf1 = (lv_color_t *)heap_caps_malloc(sizeof(lv_color_t) * BUF_PIXELS, MALLOC_CAP_INTERNAL);
    if (!_buf1) {
        while (1) delay(1000);  // Halt
    }
    lv_disp_draw_buf_init(&_draw_buf, _buf1, NULL, BUF_PIXELS);

    static lv_disp_drv_t disp_drv;
    lv_disp_drv_init(&disp_drv);
    disp_drv.hor_res = SCREEN_WIDTH;
    disp_drv.ver_res = SCREEN_HEIGHT;
    disp_drv.flush_cb = lvglFlushCb;
    disp_drv.draw_buf = &_draw_buf;
    disp_drv.user_data = &_gfx;
    lv_disp_drv_register(&disp_drv);

}

// Advance LVGL tick and run timer handler
void CrowPanelApplication::handleLvglTick(const uint32_t now) {

    if (now - _last_lvgl_tick < _next_lvgl_interval_ms) return;
    _last_lvgl_tick = now;

    uint32_t lvgl_start = micros();
    uint32_t next_ms = lv_timer_handler();
    uint32_t lvgl_elapsed = micros() - lvgl_start;

    if (next_ms < LVGL_TICK_MIN_MS) _next_lvgl_interval_ms = LVGL_TICK_MIN_MS;
    else if (next_ms > LVGL_TICK_MAX_MS) _next_lvgl_interval_ms = LVGL_TICK_MAX_MS;
    else _next_lvgl_interval_ms = next_ms;

    _diag_lvgl_time_total += lvgl_elapsed;
    if (lvgl_elapsed > _diag_lvgl_time_max) _diag_lvgl_time_max = lvgl_elapsed;
    _diag_lvgl_calls++;

}

// Handle knob rotation
void CrowPanelApplication::handleKnobRotation() {
    int8_t dir = _encoder.getDirection();
    if (dir == 0) return;

    IScreenUI* screen = _screenMgr.getCurrentScreen();
    if (screen && screen->interceptsRotation()) {
        screen->onRotation(dir);
    } else {
        if (dir > 0) _screenMgr.switchNext();
        else _screenMgr.switchPrevious();
    }
}

// Handle knob button press
void CrowPanelApplication::handleKnobButtonPress() {
    if (!_encoder.getButtonPressed()) return;
    IScreenUI* screen = _screenMgr.getCurrentScreen();
    if (screen) screen->onButtonPress();
}

// Handle UI update
void CrowPanelApplication::handleUIUpdate(const uint32_t now) {

    if (now - _last_ui_update < UI_UPDATE_INTERVAL_MS) return;
    _last_ui_update = now;

    IScreenUI* screen = _screenMgr.getCurrentScreen();
    if (!screen) return;

    uint32_t ui_start = micros();
    screen->update();
    uint32_t ui_elapsed = micros() - ui_start;

    _diag_ui_updates++;
    _diag_ui_update_time_total += ui_elapsed;
    if (ui_elapsed > _diag_ui_update_time_max) _diag_ui_update_time_max = ui_elapsed;
}

// Print diagnostics to Serial
void CrowPanelApplication::handleDiagnostics(const uint32_t now) {

    if (now - _diag_last_print < DIAG_PRINT_INTERVAL_MS) return;
    _diag_last_print = now;

    float pps = _receiver.getPacketsPerSecond();
    float avg_ui_time = (_diag_ui_updates > 0) ?
        (float)_diag_ui_update_time_total / _diag_ui_updates / 1000.0f : 0;

    float avg_lvgl_time = (_diag_lvgl_calls > 0) ?
        (float)_diag_lvgl_time_total / _diag_lvgl_calls / 1000.0f : 0;

    Serial.printf("[DIAG] PPS: %.1f | UI updates: %lu | UI avg: %.2f ms | UI max: %.2f ms\n",
        pps, _diag_ui_updates, avg_ui_time, _diag_ui_update_time_max / 1000.0f);

    float avg_flush_time = (s_flush_calls > 0) ?
        (float)s_flush_total / s_flush_calls / 1000.0f : 0;

    Serial.printf("[DIAG] LVGL calls: %lu | avg: %.2f ms | max: %.2f ms\n",
        (unsigned long)_diag_lvgl_calls, avg_lvgl_time, _diag_lvgl_time_max / 1000.0f);

    Serial.printf("[DIAG] Flush calls: %lu | avg: %.2f ms | max: %.2f ms\n",
        (unsigned long)s_flush_calls, avg_flush_time, s_flush_max / 1000.0f);

    // Memory and stack diagnostics
    Serial.printf("[DIAG] Heap free: %lu | min: %lu | Stack loop: %lu | enc: %lu | btn: %lu\n",
        (unsigned long)esp_get_free_heap_size(),
        (unsigned long)esp_get_minimum_free_heap_size(),
        (unsigned long)uxTaskGetStackHighWaterMark(NULL),
        (unsigned long)uxTaskGetStackHighWaterMark(_encoder.getEncoderTaskHandle()),
        (unsigned long)uxTaskGetStackHighWaterMark(_encoder.getButtonTaskHandle()));

    // Reset counters
    _diag_ui_updates = 0;
    _diag_ui_update_time_total = 0;
    _diag_ui_update_time_max = 0;
    _diag_lvgl_time_total = 0;
    _diag_lvgl_time_max = 0;
    _diag_lvgl_calls = 0;
    s_flush_total = 0;
    s_flush_max = 0;
    s_flush_calls = 0;
}
