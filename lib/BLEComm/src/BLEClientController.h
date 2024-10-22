//
// Created by Jochen Ullrich on 22.10.24.
//

#ifndef BLECLIENTCONTROLLER_H
#define BLECLIENTCONTROLLER_H

#include "BLEComm.h"

class BLEClientController : public BLEAdvertisedDeviceCallbacks {
public:
    BLEClientController();
    void initClient();
    bool connectToServer();
    void sendTemperatureControl(float setpoint);
    void sendPinControl(bool pinState);
    void sendPing();
    void sendAutotune();
    bool isReadyForConnection();
    void scan();
    void registerTempReadCallback(temp_read_callback_t callback);
    void registerRemoteErrorCallback(remote_err_callback_t callback);

private:
    BLERemoteCharacteristic* tempControlChar;
    BLERemoteCharacteristic* pinControlChar;
    BLERemoteCharacteristic* tempReadChar;
    BLERemoteCharacteristic* pingChar;
    BLERemoteCharacteristic* errorChar;
    BLERemoteCharacteristic* autotuneChar;
    BLEAdvertisedDevice* serverDevice;
    bool readyForConnection;

    temp_read_callback_t tempReadCallback;
    remote_err_callback_t remoteErrorCallback;

    // BLEAdvertisedDeviceCallbacks override
    void onResult(BLEAdvertisedDevice advertisedDevice) override;

    // Notification callback
    void notifyCallback(BLERemoteCharacteristic *pRemoteCharacteristic, uint8_t *pData, size_t length, bool isNotify);
};

#endif //BLECLIENTCONTROLLER_H
