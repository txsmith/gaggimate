#include "LilyGo_RGBPanel.h"
#include "LV_Helper.h"
#include "ui/ui.h"
#include "BLEClientController.h"

#define SDA_PIN 8
#define SCL_PIN 48

LilyGo_RGBPanel panel;
hw_timer_t *timer = NULL;

extern lv_obj_t *ui_BrewScreen_tempGauge;
extern lv_obj_t *ui_BrewScreen_tempTarget;
extern lv_obj_t *ui_StatusScreen_tempGauge;
extern lv_obj_t *ui_StatusScreen_tempTarget;
extern lv_obj_t *ui_MenuScreen_tempGauge;
extern lv_obj_t *ui_MenuScreen_tempTarget;
extern lv_obj_t *ui_SteamScreen_tempGauge;
extern lv_obj_t *ui_SteamScreen_tempTarget;

int setTemp = 90;
int currentTemp = 0;

BLEClientController clientController;

void IRAM_ATTR sendPingInterrupt() {
  clientController.sendPing();
}

void updateUiTargetTemp() {
  lv_arc_set_value(ui_BrewScreen_tempTarget, setTemp);
  lv_arc_set_value(ui_StatusScreen_tempTarget, setTemp);
  lv_arc_set_value(ui_MenuScreen_tempTarget, setTemp);
  lv_arc_set_value(ui_SteamScreen_tempTarget, setTemp);
}

void updateUiCurrentTemp() {
  lv_arc_set_value(ui_BrewScreen_tempGauge, currentTemp);
  lv_arc_set_value(ui_StatusScreen_tempGauge, currentTemp);
  lv_arc_set_value(ui_MenuScreen_tempGauge, currentTemp);
  lv_arc_set_value(ui_SteamScreen_tempGauge, currentTemp);
}

void onTempRead(float temperature) {
  currentTemp = temperature;
  updateUiCurrentTemp();
}

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

  clientController.initClient();

  timer = timerBegin(0, 80, true); // Timer 0, clock divisor 80
  timerAttachInterrupt(timer, &sendPingInterrupt, true); // Attach the interrupt handling function
  timerAlarmWrite(timer, 1000000, true); // Interrupt every 1 second
  timerAlarmEnable(timer); // Enable the alarm

  updateUiCurrentTemp();
  updateUiTargetTemp();

  clientController.registerTempReadCallback(onTempRead);
}

void loop() {
  lv_timer_handler();
  delay(2);
  if (clientController.isReadyForConnection()) {
    clientController.connectToServer();
  }
}
