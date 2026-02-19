#include "CrowPanelApplication.h"

// === S T A T I C ===

// Static pointer to _gfx for lvgl callback function
static Arduino_ST7701_RGBPanel* s_gfx = nullptr;

// Static callback function for lvgl
static void lvglFlushCb(lv_disp_drv_t* disp, const lv_area_t* area, lv_color_t* color_p) {

    uint32_t w = (area->x2 - area->x1 + 1);
    uint32_t h = (area->y2 - area->y1 + 1);

    // s_gfx->draw16bitBeRGBBitmap(area->x1, area->y1, (uint16_t *)&color_p->full, w, h);
    s_gfx->draw16bitRGBBitmap(area->x1, area->y1, (uint16_t *)&color_p->full, w, h);

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
    _compassUI(),
    _attitudeUI(_receiver),
    _brightnessUI(),
    _encoder(_pcf8574),
    _screenMgr(_compassUI, _attitudeUI, _brightnessUI) {}

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
    _brightnessUI.begin(PWM_CHANNEL);

    // Screen manager init
    _screenMgr.begin();

    // Rotary encoder init
    _encoder.begin();

    // ESP-NOW init
    _receiver.begin(ESP_NOW_CHANNEL);

}

// Loop
void CrowPanelApplication::loop() {

    this->handleLvglTick();

    // Update statistics
    _receiver.updateStats();

    this->handleKnobRotation();

    this->handleKnobPress();

    this->handleUIUpdate();

    this->handleDiagnostics();

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

    // Set static gfx used by the static CB function
    s_gfx = &_gfx;
    
    lv_init();

    buf1 = (lv_color_t *)heap_caps_malloc(sizeof(lv_color_t) * BUF_PIXELS, MALLOC_CAP_INTERNAL);
    if (!buf1) {
        while (1) delay(1000);  // Halt
    }
    lv_disp_draw_buf_init(&draw_buf, buf1, NULL, BUF_PIXELS);

    static lv_disp_drv_t disp_drv;
    lv_disp_drv_init(&disp_drv);
    disp_drv.hor_res = SCREEN_WIDTH;
    disp_drv.ver_res = SCREEN_HEIGHT;
    disp_drv.flush_cb = lvglFlushCb;
    disp_drv.draw_buf = &draw_buf;
    lv_disp_drv_register(&disp_drv);

}

// Diagnostics for lvgl draw
void CrowPanelApplication::handleLvglTick() {
    
    uint32_t lvgl_start = micros();
    lv_timer_handler();
    uint32_t lvgl_elapsed = micros() - lvgl_start;

    diag_lvgl_time_total += lvgl_elapsed;
    if (lvgl_elapsed > diag_lvgl_time_max) diag_lvgl_time_max = lvgl_elapsed;
    diag_lvgl_calls++;

}

// Handle knob rotation
void CrowPanelApplication::handleKnobRotation() {

    // Check rotary encoder
    // In BrightnessScreen ADJUSTING mode: rotation adjusts brightness
    // Otherwise: rotation switches screens (endless loop)
    int8_t dir = _encoder.getDirection();
    if (dir != 0) 
        if (_screenMgr.isBrightnessActive() && _brightnessUI.isAdjusting()) _brightnessUI.handleRotation(dir);
        else if (dir > 0) _screenMgr.switchNext(); 
        else _screenMgr.switchPrevious();
}

// Handle knob button press
void CrowPanelApplication::handleKnobButtonPress() {
 
    // Check knob button press
    if (_encoder.getButtonPressed()) {
        // CompassScreen: toggle heading mode (T/M)
        if (_screenMgr.isCompassActive()) _compassUI.toggleHeadingMode();
        // AttitudeScreen: handle level operation
        else if (_screenMgr.isAttitudeActive()) _attitudeUI.handleButtonPress();
        // BrightnessScreen: toggle adjustment mode
        else if (_screenMgr.isBrightnessActive()) _brightnessUI.handleButtonPress();
    }
}

// Handle UI update
void CrowPanelApplication::handleUIUpdate() {

    // Update state machines (check for timeouts)
    if (_screenMgr.isAttitudeActive()) _attitudeUI.updateLevelState();
    if (_screenMgr.isBrightnessActive()) _brightnessUI.updateState();

    // UI update
    static uint32_t last_ui_update = 0;
    uint32_t now = millis();

    if (now - last_ui_update >= UI_UPDATE_INTERVAL_MS) {
        last_ui_update = now;

        bool is_connected = _receiver.isConnected(CONNECTION_TIMEOUT_MS);

        // Get the latest data
        if (_receiver.hasNewData() || is_connected) {
            HeadingData data = _receiver.getData();
            float pps = _receiver.getPacketsPerSecond();

            // Measure the runtime of UI update
            uint32_t ui_start = micros();

            // Update screen (BrightnessScreen has no compass data to display)
            if (_screenMgr.isCompassActive()) _compassUI.update(data, is_connected, pps);
            else if (_screenMgr.isAttitudeActive()) _attitudeUI.update(data, is_connected, pps);

            // Calculate UI update runtime
            uint32_t ui_elapsed = micros() - ui_start;
            diag_ui_updates++;
            diag_ui_update_time_total += ui_elapsed;
            if (ui_elapsed > diag_ui_update_time_max) {
                diag_ui_update_time_max = ui_elapsed;
            }
        }
        else {
            // No data show disconnected indication of the screen
            static bool was_connected = false;

            if (was_connected && !is_connected) {
                if (_screenMgr.isCompassActive()) _compassUI.showDisconnected();
                else if (_screenMgr.isAttitudeActive()) _attitudeUI.showDisconnected();
            }
            was_connected = is_connected;
        }
    }
}

// Print diagnostics to Serial
void CrowPanelApplication::handleDiagnostics() {

    uint32_t now = millis();

    if (now - diag_last_print >= DIAG_PRINT_INTERVAL_MS) {
        diag_last_print = now;

        float pps = _receiver.getPacketsPerSecond();
        float avg_ui_time = (diag_ui_updates > 0) ?
            (float)diag_ui_update_time_total / diag_ui_updates / 1000.0f : 0;

        float avg_lvgl_time = (diag_lvgl_calls > 0) ?
            (float)diag_lvgl_time_total / diag_lvgl_calls / 1000.0f : 0;

        Serial.printf("[DIAG] PPS: %.1f | UI updates: %lu | UI avg: %.2f ms | UI max: %.2f ms\n",
            pps, diag_ui_updates, avg_ui_time, diag_ui_update_time_max / 1000.0f);

        Serial.printf("[DIAG] LVGL calls: %lu | avg: %.2f ms | max: %.2f ms\n",
            (unsigned long)diag_lvgl_calls, avg_lvgl_time, diag_lvgl_time_max / 1000.0f);

        // Memory and stack diagnostics
        Serial.printf("[DIAG] Heap free: %lu | min: %lu | Stack loop: %lu | enc: %lu | btn: %lu\n",
            (unsigned long)esp_get_free_heap_size(),
            (unsigned long)esp_get_minimum_free_heap_size(),
            (unsigned long)uxTaskGetStackHighWaterMark(NULL),
            (unsigned long)uxTaskGetStackHighWaterMark(_encoder.getEncoderTaskHandle()),
            (unsigned long)uxTaskGetStackHighWaterMark(_encoder.getButtonTaskHandle()));

        // Reset counters
        diag_ui_updates = 0;
        diag_ui_update_time_total = 0;
        diag_ui_update_time_max = 0;
        diag_lvgl_time_total = 0;
        diag_lvgl_time_max = 0;
        diag_lvgl_calls = 0;
    }
    
    delay(5);
}
