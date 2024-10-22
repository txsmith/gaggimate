//
// Created by Jochen Ullrich on 22.10.24.
//

#ifndef BLESERVERCONTROLLER_H
#define BLESERVERCONTROLLER_H

#include "BLEComm.h"

class BLEServerController : public BLEServerCallbacks, public BLECharacteristicCallbacks {
public:
    BLEServerController();
    void initServer();
    void sendTemperature(float temperature);
    void sendError(int errorCode);
    void registerTempControlCallback(temp_control_callback_t callback);
    void registerPinControlCallback(pin_control_callback_t callback);
    void registerPingCallback(ping_callback_t callback);
    void registerAutotuneCallback(autotune_callback_t callback);

private:
    bool deviceConnected;
    BLECharacteristic* tempControlChar;
    BLECharacteristic* pinControlChar;
    BLECharacteristic* tempReadChar;
    BLECharacteristic* pingChar;
    BLECharacteristic* errorChar;
    BLECharacteristic* autotuneChar;

    temp_control_callback_t tempControlCallback;
    pin_control_callback_t pinControlCallback;
    ping_callback_t pingCallback;
    autotune_callback_t autotuneCallback;

    // BLEServerCallbacks overrides
    void onConnect(BLEServer* pServer) override;
    void onDisconnect(BLEServer* pServer) override;

    // BLECharacteristicCallbacks overrides
    void onWrite(BLECharacteristic* pCharacteristic) override;
};

#endif //BLESERVERCONTROLLER_H
