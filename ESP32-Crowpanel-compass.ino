 // === M A I N  P R O G R A M ===
 //
 // Runs an app which receives magnetic heading, true heading, pitch and roll
 // as radians from CMPS14-ESP32-SignalK-gateway compass
 // and shows the values on CrowPanel with lvgl UI developed
 // with SquareLine Studio.

#include <Arduino.h>
#include "CrowPanelApplication.h"

CrowPanelApplication app;

// === S E T U P ===

void setup() {

    Serial.begin(115200);
    delay(100);
    app.begin();

}

// === L O O P ===

void loop() {
    
    app.loop();
}
