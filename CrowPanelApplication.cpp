#include "CrowPanelApplication.h"

// === S T A T I C ===

static Arduino_ST7701_RGBPanel* s_gfx = nullptr;

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
    _gfx(_bus, GFX_NOT_DEFINED, 0,
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
bool CrowPanelApplication::begin() {

    this->initPcfAndResetLines();

    _gfx->begin();
    _gfx->fillScreen(BLACK);

    this->initBacklight(200);

    this->initLvgl();


}

// Loop
void CrowPanelApplication::loop() {

}

// === P R I V A T E ===

// PCF init
void initPcfAndResetLines() {
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

// Screen backlight
void initBacklight(uint8_t duty) {
    ledcSetup(pwmChannel, pwmFreq, pwmResolution);
    ledcAttachPin(SCREEN_BACKLIGHT_PIN, pwmChannel);
    ledcWrite(pwmChannel, duty);
}

// LVGL init
void initLvgl() {
    lv_init();

    buf1 = (lv_color_t *)heap_caps_malloc(sizeof(lv_color_t) * buf_pixels, MALLOC_CAP_INTERNAL);
    if (!buf1) {
        while (1) delay(1000);  // Halt
    }
    lv_disp_draw_buf_init(&draw_buf, buf1, NULL, buf_pixels);

    static lv_disp_drv_t disp_drv;
    lv_disp_drv_init(&disp_drv);
    disp_drv.hor_res = screenWidth;
    disp_drv.ver_res = screenHeight;
    disp_drv.flush_cb = my_disp_flush;
    disp_drv.draw_buf = &draw_buf;
    lv_disp_drv_register(&disp_drv);
}

// === S E T U P ===

void setup() {

JATKA TÄSTÄ:
    // SquareLine UI init 
    ui_init();

    // UI adapter init
    compassUI.begin();
    attitudeUI.begin();
    brightnessUI.begin(pwmChannel);

    // Screen manager init
    screenMgr.begin();

    // Rotary encoder init
    encoder.begin();

    // ESP-NOW init
    receiver.begin(ESP_NOW_CHANNEL);
}

// === L O O P ===

void loop() {
    
    // LVGL tick
    uint32_t lvgl_start = micros();
    lv_timer_handler();
    uint32_t lvgl_elapsed = micros() - lvgl_start;

    diag_lvgl_time_total += lvgl_elapsed;
    if (lvgl_elapsed > diag_lvgl_time_max) {
        diag_lvgl_time_max = lvgl_elapsed;
    }
    diag_lvgl_calls++;

    // Update statistics
    receiver.updateStats();

    // Check rotary encoder
    // In BrightnessScreen ADJUSTING mode: rotation adjusts brightness
    // Otherwise: rotation switches screens (endless loop)
    int8_t dir = encoder.getDirection();
    if (dir != 0) {
        if (screenMgr.isBrightnessActive() && brightnessUI.isAdjusting()) {
            brightnessUI.handleRotation(dir);
        } else {
            if (dir > 0) {
                screenMgr.switchNext();     // clockwise
            } else {
                screenMgr.switchPrevious(); // counter-clockwise
            }
        }
    }

    // Check knob button press
    if (encoder.getButtonPressed()) {
        if (screenMgr.isCompassActive()) {
            // CompassScreen: toggle heading mode (T/M)
            compassUI.toggleHeadingMode();
        } else if (screenMgr.isAttitudeActive()) {
            // AttitudeScreen: handle level operation
            attitudeUI.handleButtonPress();
        } else if (screenMgr.isBrightnessActive()) {
            // BrightnessScreen: toggle adjustment mode
            brightnessUI.handleButtonPress();
        }
    }

    // Update state machines (check for timeouts)
    if (screenMgr.isAttitudeActive()) {
        attitudeUI.updateLevelState();
    }
    if (screenMgr.isBrightnessActive()) {
        brightnessUI.updateState();
    }

    // UI update
    static uint32_t last_ui_update = 0;
    uint32_t now = millis();

    if (now - last_ui_update >= UI_UPDATE_INTERVAL_MS) {
        last_ui_update = now;

        bool is_connected = receiver.isConnected(CONNECTION_TIMEOUT_MS);

        // Get the latest data
        if (receiver.hasNewData() || is_connected) {
            HeadingData data = receiver.getData();
            float pps = receiver.getPacketsPerSecond();

            // Measure the runtime of UI update
            uint32_t ui_start = micros();

            // Update screen (BrightnessScreen has no compass data to display)
            if (screenMgr.isCompassActive()) {
                compassUI.update(data, is_connected, pps);
            } else if (screenMgr.isAttitudeActive()) {
                attitudeUI.update(data, is_connected, pps);
            }

            // Calculate UI update runtime
            uint32_t ui_elapsed = micros() - ui_start;
            diag_ui_updates++;
            diag_ui_update_time_total += ui_elapsed;
            if (ui_elapsed > diag_ui_update_time_max) {
                diag_ui_update_time_max = ui_elapsed;
            }
        }
        else {
            // No data show disconnected
            static bool was_connected = false;

            if (was_connected && !is_connected) {
                if (screenMgr.isCompassActive()) {
                    compassUI.showDisconnected();
                } else if (screenMgr.isAttitudeActive()) {
                    attitudeUI.showDisconnected();
                }
                // BrightnessScreen: no connection indicator
            }
            was_connected = is_connected;
        }
    }

    // Print diagnostics to Serial
    if (now - diag_last_print >= DIAG_PRINT_INTERVAL_MS) {
        diag_last_print = now;

        float pps = receiver.getPacketsPerSecond();
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
            (unsigned long)uxTaskGetStackHighWaterMark(NULL),  // Current task (loop)
            (unsigned long)uxTaskGetStackHighWaterMark(encoder.getEncoderTaskHandle()),
            (unsigned long)uxTaskGetStackHighWaterMark(encoder.getButtonTaskHandle()));

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
