#include "CrowPanelApplication.h"

// === S T A T I C ===

static uint32_t s_flush_total = 0;
static uint32_t s_flush_max = 0;
static uint32_t s_flush_calls = 0;

// Static callback function for LVGL (using now partial rendering mode)
// LVGL renders a region into the SRAM draw buffer, then we blit it to the display.
static void lvglFlushCb(lv_display_t* disp, const lv_area_t* area, uint8_t* px_map) {

    auto* gfx = static_cast<Arduino_RGB_Display*>(lv_display_get_user_data(disp));
    if (!gfx) {
        lv_display_flush_ready(disp);
        return;
    }
    uint32_t t0 = micros();

    int32_t w = area->x2 - area->x1 + 1;
    int32_t h = area->y2 - area->y1 + 1;
    // lv_draw_sw_rgb565_swap(px_map, w * h); // Not needed?
    // gfx->flush(true); // DIRECT rendering mode
    gfx->draw16bitRGBBitmap(area->x1, area->y1, (uint16_t*)px_map, w, h); // PARTIAL rendering mode
    // gfx->draw16bitBeRGBBitmap(area->x1, area->y1, (uint16_t*)px_map, w, h); // PARTIAL rendering mode, Big endian

    uint32_t ft = micros() - t0;
    s_flush_total += ft;
    s_flush_calls++;
    if (ft > s_flush_max) s_flush_max = ft;

    lv_display_flush_ready(disp);
}

// === P U B L I C ===

// Constructor
CrowPanelApplication::CrowPanelApplication():
    _init_bus(GFX_NOT_DEFINED, 16, 2, 1, GFX_NOT_DEFINED),
    _bus(40 /* DE */, 7 /* VSYNC */, 15 /* HSYNC */, 41 /* PCLK */,
        46 /* R0 */, 3 /* R1 */, 8 /* R2 */, 18 /* R3 */, 17 /* R4 */,
        14 /* G0 */, 13 /* G1 */, 12 /* G2 */, 11 /* G3 */, 10 /* G4 */, 9 /* G5 */,
        5 /* B0 */, 45 /* B1 */, 48 /* B2 */, 47 /* B3 */, 21 /* B4 */,
        1, 10, 4, 20,   /* hsync: polarity, front, pulse, back */
        1, 10, 4, 20),  /* vsync: polarity, front, pulse, back */
    _gfx(480 /* width */, 480 /* height */, &_bus, 0 /* rotation */, true /* auto_flush */,
        &_init_bus, GFX_NOT_DEFINED /* RST */,
        st7701_type5_init_operations, sizeof(st7701_type5_init_operations)),
    _pcf8574(0x21),
    _receiver(),
    _compassUI(_receiver),
    _attitudeUI(_receiver),
    _weatherUI(_receiver),
    _batteryUI(_receiver),
    _brightnessUI(SCREEN_BACKLIGHT_PIN),
    _encoder(_pcf8574),
    _screenMgr() {}

// Initialize
void CrowPanelApplication::begin() {

    this->initPcfAndResetLines();

    this->initBacklight(PWM_DUTY);

    this->initDisplay();

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
    _gfx.fillScreen(RGB565_BLACK);
    delay(2000);
    _gfx.fillScreen(RGB565_WHITE);
    delay(2000);
    _gfx.fillScreen(RGB565_RED);
    delay(2000);
    _gfx.fillScreen(RGB565_GREEN);
    delay(2000);
    _gfx.fillScreen(RGB565_BLUE);
    delay(2000);
    _gfx.fillScreen(RGB565_YELLOW);
    delay(2000);
    _gfx.fillScreen(RGB565_CYAN);
    delay(2000);
    _gfx.fillScreen(RGB565_MAGENTA);
    delay(2000);
    _gfx.fillScreen(RGB565_BLACK);
    delay(2000);
}

// Screen backlight
void CrowPanelApplication::initBacklight(uint8_t duty) {
    ledcAttach(SCREEN_BACKLIGHT_PIN, PWM_FREQ, PWM_RESOLUTION);
    ledcWrite(SCREEN_BACKLIGHT_PIN, duty);
}

// LVGL init
void CrowPanelApplication::initLvgl() {

    lv_init();
    lv_tick_set_cb(millis);

    // Partial rendering mode: LVGL renders into an SRAM draw buffer (120 lines),
    // then lvglFlushCb blits each region to the display via draw16bitRGBBitmap.
    // Buffer size uses sizeof(uint16_t) (RGB565, 2 bytes/pixel) — NOT sizeof(lv_color_t)
    // which is 3 bytes in LVGL 9 and would cause wrong flush-row calculations.
    _buf1 = (uint16_t*)heap_caps_malloc(sizeof(uint16_t) * BUF_PIXELS, MALLOC_CAP_INTERNAL);
    // _buf1 = _gfx.getFramebuffer(); // DIRECT rendering mode
    if (!_buf1) {
        while (1) delay(1000);  // Halt: out of SRAM
    }

    _lvgl_disp = lv_display_create(SCREEN_WIDTH, SCREEN_HEIGHT);
    lv_display_set_flush_cb(_lvgl_disp, lvglFlushCb);
    
    // lv_display_set_color_format(_lvgl_disp, LV_COLOR_FORMAT_RGB565); // Not needed?
    
    // DIRECT rendeering mode:
    // lv_display_set_buffers(_lvgl_disp,
    //                        (void*)_buf1,
    //                        nullptr,
    //                        SCREEN_WIDTH * SCREEN_HEIGHT * sizeof(uint16_t),
    //                        LV_DISPLAY_RENDER_MODE_DIRECT);

    // PARTIAL rendering mode
    lv_display_set_buffers(_lvgl_disp,
                           _buf1,
                           nullptr,
                           sizeof(uint16_t) * BUF_PIXELS,
                           LV_DISPLAY_RENDER_MODE_PARTIAL);
    
    lv_display_set_user_data(_lvgl_disp, &_gfx);

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
