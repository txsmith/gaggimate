#include "main.h"
#include "LilyGo_RGBPanel.h"
#include "LV_Helper.h"
#include "ui/ui.h"
#include "NimBLEClientController.h"

LilyGo_RGBPanel panel;
hw_timer_t *timer = NULL;

extern lv_obj_t *ui_BrewScreen_tempGauge;
extern lv_obj_t *ui_StatusScreen_tempGauge;
extern lv_obj_t *ui_MenuScreen_tempGauge;
extern lv_obj_t *ui_SteamScreen_tempGauge;

extern lv_obj_t *ui_BrewScreen_tempTarget;
extern lv_obj_t *ui_StatusScreen_tempTarget;
extern lv_obj_t *ui_MenuScreen_tempTarget;
extern lv_obj_t *ui_SteamScreen_tempTarget;

extern lv_obj_t *ui_BrewScreen_tempText;
extern lv_obj_t *ui_StatusScreen_tempText;
extern lv_obj_t *ui_MenuScreen_tempText;
extern lv_obj_t *ui_SteamScreen_tempText;

extern lv_obj_t *ui_StatusScreen_targetTemp;
extern lv_obj_t *ui_BrewScreen_targetTemp;
extern lv_obj_t *ui_SteamScreen_targetTemp;
extern lv_obj_t *ui_WaterScreen_targetTemp;

int mode = MODE_BREW;
int targetBrewTemp = 90;
int targetSteamTemp = 145;
int targetWaterTemp = 80;
int currentTemp = 0;
unsigned long lastPing = 0;

NimBLEClientController clientController;

int getTargetTemp() {
  if (mode == MODE_BREW) return targetBrewTemp;
  if (mode == MODE_STEAM) return targetSteamTemp;
  if (mode == MODE_WATER) return targetWaterTemp;
  return 0;
}

void updateUiTargetTemp() {
  int setTemp = getTargetTemp();
  lv_arc_set_value(ui_BrewScreen_tempTarget, setTemp);
  lv_arc_set_value(ui_StatusScreen_tempTarget, setTemp);
  lv_arc_set_value(ui_MenuScreen_tempTarget, setTemp);
  lv_arc_set_value(ui_SteamScreen_tempTarget, setTemp);

  lv_label_set_text_fmt(ui_StatusScreen_targetTemp, "%d°C", targetBrewTemp);
  lv_label_set_text_fmt(ui_BrewScreen_targetTemp, "%d°C", targetBrewTemp);
  lv_label_set_text_fmt(ui_SteamScreen_targetTemp, "%d°C", targetSteamTemp);
  lv_label_set_text_fmt(ui_WaterScreen_targetTemp, "%d°C", targetWaterTemp);
}

void setMode(int newMode) {
  mode = newMode;
  updateUiTargetTemp();
  clientController.sendTemperatureControl(getTargetTemp());
}

void setTargetBrewTemp(int temp) {
  targetBrewTemp = temp;
  updateUiTargetTemp();
  clientController.sendTemperatureControl(getTargetTemp());
}

void setTargetSteamTemp(int temp) {
  targetSteamTemp = temp;
  updateUiTargetTemp();
  clientController.sendTemperatureControl(getTargetTemp());
}

void setTargetWaterTemp(int temp) {
  targetWaterTemp = temp;
  updateUiTargetTemp();
  clientController.sendTemperatureControl(getTargetTemp());
}

void updateUiCurrentTemp() {
  lv_arc_set_value(ui_BrewScreen_tempGauge, currentTemp);
  lv_arc_set_value(ui_StatusScreen_tempGauge, currentTemp);
  lv_arc_set_value(ui_MenuScreen_tempGauge, currentTemp);
  lv_arc_set_value(ui_SteamScreen_tempGauge, currentTemp);

  lv_label_set_text_fmt(ui_BrewScreen_tempText, "%d°C", currentTemp);
  lv_label_set_text_fmt(ui_StatusScreen_tempText, "%d°C", currentTemp);
  lv_label_set_text_fmt(ui_MenuScreen_tempText, "%d°C", currentTemp);
  lv_label_set_text_fmt(ui_SteamScreen_tempText, "%d°C", currentTemp);
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

  updateUiCurrentTemp();
  updateUiTargetTemp();

  clientController.registerTempReadCallback(onTempRead);

  lastPing = millis();
}

void loop() {
  lv_timer_handler();
  delay(2);
  if (clientController.isReadyForConnection()) {
    clientController.connectToServer();
  }
  if (millis() - lastPing > PING_INTERVAL) {
    lastPing = millis();
    clientController.sendPing();
  }
}
