#include "LilyGo_RGBPanel.h"
#include "LV_Helper.h"
#include "ui/ui.h"
#include "ProtobufComm.h"

LilyGo_RGBPanel panel;

void setup() {
  Serial.begin(115200);

  // Initialize T-RGB, if the initialization fails, false will be returned.
  if (!panel.begin()) {
    while (1) {
      Serial.println("Error, failed to initialize T-RGB"); delay(1000);
    }
  }

  beginLvglHelper(panel);
  panel.setBrightness(16);
  ui_init();
}

void loop() {
  lv_timer_handler();
  delay(2);
}
