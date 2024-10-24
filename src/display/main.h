//
// Created by Jochen Ullrich on 23.10.24.
//

#ifndef MAIN_H
#define MAIN_H


#define SDA_PIN 8
#define SCL_PIN 48

#define PING_INTERVAL                   1000
#define PROGRESS_INTERVAL                250
#define HOT_WATER_SAFETY_DURATION_MS   30000
#define STEAM_SAFETY_DURATION_MS       60000
#define BREW_MIN_DURATION_MS            5000
#define BREW_MAX_DURATION_MS          120000
#define STANDBY_TIMEOUT_MS            900000
#define TEMP_OFFSET                        7

#define MODE_STANDBY 0
#define MODE_BREW    1
#define MODE_STEAM   2
#define MODE_WATER   3

#define MDNS_NAME "gaggia"

bool isActive();
void activate(unsigned long until);
void deactivate();
void setTargetBrewTemp(int temp);
void setTargetSteamTemp(int temp);
void setTargetWaterTemp(int temp);
void setTargetDuration(int duration);
void setMode(int mode);
void updateLastAction();

#endif //MAIN_H
