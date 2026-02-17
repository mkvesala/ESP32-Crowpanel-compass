 // === M A I N  P R O G R A M ===
 //
 // Receives magnetic heading, true heading, pitch and roll
 // as radians from CMPS14-ESP32-SignalK-gateway compass
 // and shows the values on CrowPanel with lvgl. UI development
 // with SquareLine Studio.
 //
 // Turn knob to switch screens:
 // - CompassScreen: compass rose and heading
 // - AttitudeScreen: artificial horizon, pitch and roll
 // - BrightnessScreen: display backlight brightness adjustment
 //
 // Hardware: Elecrow CrowPanel 2.1" HMI (ESP32-S3, 480x480 IPS) Rotary Display Round Knob Screen
 // ESP32 Core: 2.0.14

#include <Arduino.h>
#include <Wire.h>

// Graphics libraries for Elecrow CrowPanel
#include <lvgl.h>
#include <Arduino_GFX_Library.h>
#include "PCF8574.h"

// SquareLine Studio UI export
#include "ui.h"

// Own class implementations
#include "HeadingData.h"
#include "ESPNowReceiver.h"
#include "CompassUI.h"
#include "AttitudeUI.h"
#include "BrightnessUI.h"
#include "RotaryEncoder.h"
#include "ScreenManager.h"

// ESP-NOW config

// Channel must match compass WiFi AP channel
static constexpr uint8_t ESP_NOW_CHANNEL = 6;

// Connection timeout if nothing received (ms)
static constexpr uint32_t CONNECTION_TIMEOUT_MS = 3000;

// I2C / PCF8574
static constexpr uint8_t I2C_SDA_PIN = 38;
static constexpr uint8_t I2C_SCL_PIN = 39;
PCF8574 pcf8574(0x21);

// PCF pins
static constexpr uint8_t PCF_TP_RST  = P0;  // 0
static constexpr uint8_t PCF_TP_INT  = P2;  // 2
static constexpr uint8_t PCF_LCD_PWR = P3;  // 3
static constexpr uint8_t PCF_LCD_RST = P4;  // 4
static constexpr uint8_t PCF_RE_BTN  = P5;  // 5

// Backlight PWM (ESP32 core 2.0.14 API)
static constexpr uint8_t SCREEN_BACKLIGHT_PIN = 6;
const int pwmFreq = 5000;
const int pwmChannel = 0;
const int pwmResolution = 8;

// Display
static const uint16_t screenWidth  = 480;
static const uint16_t screenHeight = 480;

// LVGL draw buffer (40 lines)
static lv_disp_draw_buf_t draw_buf;
static lv_color_t *buf1 = NULL;
static const uint32_t buf_pixels = screenWidth * 40;

// UI upddate frequency ~17 Hz (compass send rate is 53 ms)
static constexpr uint8_t UI_UPDATE_INTERVAL_MS = 59;

// Diagnostics and debug interval 5 secs
static constexpr uint8_t DIAG_PRINT_INTERVAL_MS = 5000; 

// Diagnostic counters
static uint32_t diag_ui_updates = 0;
static uint32_t diag_ui_update_time_total = 0;
static uint32_t diag_ui_update_time_max = 0;
static uint32_t diag_lvgl_time_total = 0;
static uint32_t diag_lvgl_time_max = 0;
static uint32_t diag_lvgl_calls = 0;
static uint32_t diag_last_print = 0;

// Arduino_GFX bus + panel
Arduino_ESP32RGBPanel *bus = new Arduino_ESP32RGBPanel(
    16 /* CS */, 2 /* SCK */, 1 /* SDA */,
    40 /* DE */, 7 /* VSYNC */, 15 /* HSYNC */, 41 /* PCLK */,
    46 /* R0 */, 3 /* R1 */, 8 /* R2 */, 18 /* R3 */, 17 /* R4 */,
    14 /* G0 */, 13 /* G1 */, 12 /* G2 */, 11 /* G3 */, 10 /* G4 */, 9 /* G5 */,
    5 /* B0 */, 45 /* B1 */, 48 /* B2 */, 47 /* B3 */, 21 /* B4 */
);

Arduino_ST7701_RGBPanel *gfx = new Arduino_ST7701_RGBPanel(
    bus, GFX_NOT_DEFINED /* RST */, 0 /* rotation */,
    false /* IPS */, 480 /* width */, 480 /* height */,
    st7701_type5_init_operations, sizeof(st7701_type5_init_operations),
    true /* BGR */,
    10 /* hsync_front_porch */, 4 /* hsync_pulse_width */, 20 /* hsync_back_porch */,
    10 /* vsync_front_porch */, 4 /* vsync_pulse_width */, 20 /* vsync_back_porch */
);

// Global instances
ESPNowReceiver receiver;
CompassUI compassUI;
AttitudeUI attitudeUI(receiver);
BrightnessUI brightnessUI;
RotaryEncoder encoder;
ScreenManager screenMgr(compassUI, attitudeUI, brightnessUI);

// LVGL flush callback
void my_disp_flush(lv_disp_drv_t *disp, const lv_area_t *area, lv_color_t *color_p) {
    uint32_t w = (area->x2 - area->x1 + 1);
    uint32_t h = (area->y2 - area->y1 + 1);

    // gfx->draw16bitBeRGBBitmap(area->x1, area->y1, (uint16_t *)&color_p->full, w, h);
    gfx->draw16bitRGBBitmap(area->x1, area->y1, (uint16_t *)&color_p->full, w, h);

    lv_disp_flush_ready(disp);
}

// Screen backlight
void initBacklight(uint8_t duty) {
    ledcSetup(pwmChannel, pwmFreq, pwmResolution);
    ledcAttachPin(SCREEN_BACKLIGHT_PIN, pwmChannel);
    ledcWrite(pwmChannel, duty);
}

// PCF init
void initPcfAndResetLines() {
    Wire.begin(I2C_SDA_PIN, I2C_SCL_PIN);

    pcf8574.pinMode(PCF_TP_RST, OUTPUT);
    pcf8574.pinMode(PCF_TP_INT, OUTPUT);
    pcf8574.pinMode(PCF_LCD_PWR, OUTPUT);
    pcf8574.pinMode(PCF_LCD_RST, OUTPUT);
    pcf8574.pinMode(PCF_RE_BTN, INPUT_PULLUP); 

    pcf8574.begin();

    // LCD power on
    pcf8574.digitalWrite(PCF_LCD_PWR, HIGH);
    delay(100);

    // LCD reset sequence
    pcf8574.digitalWrite(PCF_LCD_RST, HIGH);
    delay(50);
    pcf8574.digitalWrite(PCF_LCD_RST, LOW);
    delay(120);
    pcf8574.digitalWrite(PCF_LCD_RST, HIGH);
    delay(120);

    // Touch reset lines
    pcf8574.digitalWrite(PCF_TP_RST, HIGH);
    delay(50);
    pcf8574.digitalWrite(PCF_TP_RST, LOW);
    delay(120);
    pcf8574.digitalWrite(PCF_TP_RST, HIGH);
    delay(120);

    pcf8574.digitalWrite(PCF_TP_INT, HIGH);
    delay(50);
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

    Serial.begin(115200);

    // Hardware init
    initPcfAndResetLines();

    gfx->begin();
    gfx->fillScreen(BLACK);

    initBacklight(200);

    // LVGL init
    initLvgl();

    // SquareLine UI init 
    ui_init();

    // UI adapter init
    compassUI.begin();
    attitudeUI.begin();
    brightnessUI.begin(pwmChannel);

    // Screen manager init
    screenMgr.begin();

    // Rotary encoder init
    encoder.begin(pcf8574);

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
