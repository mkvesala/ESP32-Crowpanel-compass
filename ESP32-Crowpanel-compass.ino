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
#include "CrowPanelApplication.h"

CrowPanelApplication app;

// === S E T U P ===

void setup() {

    Serial.begin(115200);
    delay(100);
    if (!app.begin()) while(1) delay(1000);

}

// === L O O P ===

void loop() {
    
    app.loop();
}
