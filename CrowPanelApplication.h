#pragma once

#include <Arduino.h>
#include <Wire.h>
#include <lvgl.h>
#include <Arduino_GFX_Library.h>
#include "PCF8574.h"
#include "ui.h"
#include "espnow_protocol.h"
#include "ESPNowReceiver.h"
#include "CompassUI.h"
#include "AttitudeUI.h"
#include "BrightnessUI.h"
#include "RotaryEncoder.h"
#include "ScreenManager.h"

// === C L A S S  C R O W P A N E L A P P L I C A T I O N ===
//
// - Class CrowPanelApplication - "the app", responsible for orchestrating everything
// - Init: app.begin();
// - Loop: app.loop()
// - Provides public API to:
//  - ..
// - Uses: 
// - Owns: PCF8574, ESPNowReceiver, CompassUI, AttitudeUI, BrightnessUI, RotaryEncoder, ScreenManager

class CrowPanelApplication {

public:

    CrowPanelApplication();

    void begin();
    void loop();

private:

    // Channel must match compass WiFi AP channel
    static constexpr uint8_t ESP_NOW_CHANNEL = 6;

    // Connection timeout if nothing received (ms)
    static constexpr uint32_t CONNECTION_TIMEOUT_MS = 3000;

    // I2C / PCF8574
    static constexpr uint8_t I2C_SDA_PIN = 38;
    static constexpr uint8_t I2C_SCL_PIN = 39;

    // PCF pins
    static constexpr uint8_t PCF_TP_RST  = P0;  // 0
    static constexpr uint8_t PCF_TP_INT  = P2;  // 2
    static constexpr uint8_t PCF_LCD_PWR = P3;  // 3
    static constexpr uint8_t PCF_LCD_RST = P4;  // 4
    static constexpr uint8_t PCF_RE_BTN  = P5;  // 5

    // Backlight PWM (ESP32 core 2.0.14 API)
    static constexpr uint8_t SCREEN_BACKLIGHT_PIN = 6;
    static constexpr uint32_t PWM_FREQ = 5000;
    static constexpr uint8_t PWM_CHANNEL = 0;
    static constexpr uint8_t PWM_RESOLUTION = 8;
    static constexpr uint8_t PWM_DUTY = 200;

    // Display
    static constexpr uint16_t SCREEN_WIDTH  = 480;
    static constexpr uint16_t SCREEN_HEIGHT = 480;

    // LVGL draw buffer (40 lines)
    lv_disp_draw_buf_t draw_buf;
    lv_color_t *buf1 = NULL;
    static constexpr uint32_t BUF_PIXELS = SCREEN_WIDTH * 40;

    // UI upddate frequency ~17 Hz (compass send rate is 53 ms)
    static constexpr uint8_t UI_UPDATE_INTERVAL_MS = 59;
    uint32_t last_ui_update = 0;

    // Diagnostics and debug interval 5 secs
    static constexpr uint32_t DIAG_PRINT_INTERVAL_MS = 5000; 

    // Diagnostic counters
    uint32_t diag_ui_updates = 0;
    uint32_t diag_ui_update_time_total = 0;
    uint32_t diag_ui_update_time_max = 0;
    uint32_t diag_lvgl_time_total = 0;
    uint32_t diag_lvgl_time_max = 0;
    uint32_t diag_lvgl_calls = 0;
    uint32_t diag_last_print = 0;

    // Arduino_GFX bus + panel
    Arduino_ESP32RGBPanel _bus;
    Arduino_ST7701_RGBPanel _gfx;

    // Knob button switch
    PCF8574 _pcf8574;

    // Core instances for the app
    ESPNowReceiver _receiver;
    CompassUI _compassUI;
    AttitudeUI _attitudeUI;
    BrightnessUI _brightnessUI;
    RotaryEncoder _encoder;
    ScreenManager _screenMgr;

    void initPcfAndResetLines();
    void initBacklight(uint8_t duty);
    void initDisplay();
    void initLvgl();
    void handleLvglTick();
    void handleKnobRotation();
    void handleKnobButtonPress();
    void handleUIUpdate(const uint32_t now);
    void handleDiagnostics(const uint32_t now);
    
};
