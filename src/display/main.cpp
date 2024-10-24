#include "main.h"
#include "LilyGo_RGBPanel.h"
#include "LV_Helper.h"
#include "ui/ui.h"
#include "NimBLEClientController.h"
#include "config.h"
#include <WiFi.h>
#include <WiFiClient.h>
#include <WebServer.h>
#include <ElegantOTA.h>


const char* SSID = WIFI_SSID;
const char* PASS = WIFI_PASS;

WebServer server(80);

LilyGo_RGBPanel panel;
hw_timer_t *timer = NULL;

int mode = MODE_BREW;
int targetBrewTemp = 90;
int targetSteamTemp = 145;
int targetWaterTemp = 80;
int targetDuration = 25000;
int currentTemp = 0;
unsigned long activeUntil = 0;
unsigned long lastPing = 0;
unsigned long lastProgress = 0;
unsigned long lastAction = 0;

NimBLEClientController clientController;

int getTargetTemp() {
  if (mode == MODE_BREW) return targetBrewTemp;
  if (mode == MODE_STEAM) return targetSteamTemp;
  if (mode == MODE_WATER) return targetWaterTemp;
  return 0;
}

bool isActive() {
  return activeUntil > millis();
}

void updateLastAction() {
  lastAction = millis();
}

void updateUiActive() {
  bool active = isActive();
  if (mode == MODE_BREW) {
    if (!active) {
      _ui_screen_change( &ui_BrewScreen, LV_SCR_LOAD_ANIM_NONE, 500, 0, &ui_BrewScreen_screen_init);
    }
  }
  lv_imgbtn_set_src(ui_SteamScreen_goButton, LV_IMGBTN_STATE_RELEASED, NULL, active ? &ui_img_646127855 : &ui_img_2106667244, NULL);
  lv_imgbtn_set_src(ui_WaterScreen_goButton, LV_IMGBTN_STATE_RELEASED, NULL, active ? &ui_img_646127855 : &ui_img_2106667244, NULL);
}

void updateRelay() {
  bool active = isActive();
  bool pump = false;
  bool valve = false;
  if (active) {
    if (mode == MODE_BREW) {
      valve = true;
    }
    if (mode == MODE_BREW || mode == MODE_WATER) {
      pump = true;
    }
  }
  clientController.sendPumpControl(pump);
  clientController.sendValveControl(valve);
}

void activate(unsigned long until) {
  if (!isActive()) {
    activeUntil = until;
  }
  updateUiActive();
  updateRelay();
  updateLastAction();
}

void deactivate() {
  activeUntil = 0;
  updateUiActive();
  updateRelay();
  updateLastAction();
}

void updateUiSettings() {
  int setTemp = getTargetTemp();
  lv_arc_set_value(ui_BrewScreen_tempTarget, setTemp);
  lv_arc_set_value(ui_StatusScreen_tempTarget, setTemp);
  lv_arc_set_value(ui_MenuScreen_tempTarget, setTemp);
  lv_arc_set_value(ui_SteamScreen_tempTarget, setTemp);

  lv_label_set_text_fmt(ui_StatusScreen_targetTemp, "%d°C", targetBrewTemp);
  lv_label_set_text_fmt(ui_BrewScreen_targetTemp, "%d°C", targetBrewTemp);
  lv_label_set_text_fmt(ui_SteamScreen_targetTemp, "%d°C", targetSteamTemp);
  lv_label_set_text_fmt(ui_WaterScreen_targetTemp, "%d°C", targetWaterTemp);

  double secondsDouble = targetDuration / 1000.0;
  int minutes = (int)(secondsDouble / 60.0 - 0.5);
  int seconds = (int) secondsDouble % 60;
  lv_label_set_text_fmt(ui_BrewScreen_targetDuration, "%2d:%02d", minutes, seconds);
  lv_label_set_text_fmt(ui_StatusScreen_targetDuration, "%2d:%02d", minutes, seconds);

  updateLastAction();
}

void setMode(int newMode) {
  mode = newMode;
  updateUiSettings();
  clientController.sendTemperatureControl(getTargetTemp());
}

void setTargetBrewTemp(int temp) {
  targetBrewTemp = temp;
  updateUiSettings();
  clientController.sendTemperatureControl(getTargetTemp());
}

void setTargetSteamTemp(int temp) {
  targetSteamTemp = temp;
  updateUiSettings();
  clientController.sendTemperatureControl(getTargetTemp());
}

void setTargetWaterTemp(int temp) {
  targetWaterTemp = temp;
  updateUiSettings();
  clientController.sendTemperatureControl(getTargetTemp());
}

void setTargetDuration(int duration) {
  targetDuration = duration;
  updateUiSettings();
}

void updateUiCurrentTemp() {
  lv_arc_set_value(ui_BrewScreen_tempGauge, currentTemp);
  lv_arc_set_value(ui_StatusScreen_tempGauge, currentTemp);
  lv_arc_set_value(ui_MenuScreen_tempGauge, currentTemp);
  lv_arc_set_value(ui_SteamScreen_tempGauge, currentTemp);
  lv_arc_set_value(ui_WaterScreen_tempGauge, currentTemp);

  lv_label_set_text_fmt(ui_BrewScreen_tempText, "%d°C", currentTemp);
  lv_label_set_text_fmt(ui_StatusScreen_tempText, "%d°C", currentTemp);
  lv_label_set_text_fmt(ui_MenuScreen_tempText, "%d°C", currentTemp);
  lv_label_set_text_fmt(ui_SteamScreen_tempText, "%d°C", currentTemp);
  lv_label_set_text_fmt(ui_WaterScreen_tempText, "%d°C", currentTemp);
}

void updateBrewProgress() {
  unsigned long now = millis();
  unsigned long progress = now - (activeUntil - targetDuration);
  double secondsDouble = targetDuration / 1000.0;
  int minutes = (int)(secondsDouble / 60.0 - 0.5);
  int seconds = (int) secondsDouble % 60;
  double progressSecondsDouble = progress / 1000.0;
  int progressMinutes = (int)(progressSecondsDouble / 60.0 - 0.5);
  int progressSeconds = (int) progressSecondsDouble % 60;
  lv_bar_set_range(ui_StatusScreen_progressBar, 0, (int) secondsDouble);
  lv_bar_set_value(ui_StatusScreen_progressBar, progress / 1000, LV_ANIM_OFF);
  lv_label_set_text_fmt(ui_StatusScreen_progressLabel,"%2d:%02d / %2d:%02d", progressMinutes, progressSeconds, minutes, seconds);
}

void onTempRead(float temperature) {
  currentTemp = temperature;
  updateUiCurrentTemp();
}

void setupWifi() {
  WiFi.mode(WIFI_STA);
  WiFi.begin(SSID, PASS);
  Serial.println("");

  // Wait for connection
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  Serial.println("");
  Serial.print("Connected to ");
  Serial.println(SSID);
  Serial.print("IP address: ");
  Serial.println(WiFi.localIP());

  server.on("/", []() {
    server.send(200, "text/plain", "Hi! This is ElegantOTA Demo.");
  });
  ElegantOTA.begin(&server);
  server.begin();
  Serial.print("OTA server started");
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
  updateUiSettings();

  clientController.registerTempReadCallback(onTempRead);

  lastPing = millis();

  setupWifi();
}

void loop() {
  lv_timer_handler();
  server.handleClient();
  ElegantOTA.loop();
  delay(2);
  if (clientController.isReadyForConnection()) {
    clientController.connectToServer();
  }
  unsigned long now = millis();
  if (now - lastPing > PING_INTERVAL) {
    lastPing = millis();
    clientController.sendPing();
    clientController.sendTemperatureControl(getTargetTemp());
    updateRelay();
  }
  if (now - lastProgress > PROGRESS_INTERVAL && mode == MODE_BREW) {
    lastProgress = millis();
    updateBrewProgress();
  }
  if (activeUntil != 0 && now > activeUntil) {
    deactivate();
  }
  if (mode != MODE_STANDBY && now > lastAction + STANDBY_TIMEOUT_MS) {
    deactivate();
    setMode(MODE_STANDBY);
    _ui_screen_change( &ui_StandbyScreen, LV_SCR_LOAD_ANIM_NONE, 500, 0, &ui_StandbyScreen_screen_init);
  }
}
