// BLEClientController.cpp

#include "BLEClientController.h"
#include "esp_log.h"

#define MAX_CONNECT_RETRIES 3
#define BLE_SCAN_DURATION_SECONDS 10

BLEClientController::BLEClientController() :
    tempControlChar(nullptr),
    pinControlChar(nullptr),
    tempReadChar(nullptr),
    pingChar(nullptr),
    errorChar(nullptr),
    autotuneChar(nullptr),
    serverDevice(nullptr),
    readyForConnection(false),
    remoteErrorCallback(nullptr),
    tempReadCallback(nullptr) {}

void BLEClientController::initClient() {
    BLEDevice::init("GPBLC");
    BLEDevice::setPower(ESP_PWR_LVL_P9);  // Set to maximum power
    esp_log_level_set("*", ESP_LOG_INFO);
    esp_log_level_set("BLEDevice", ESP_LOG_DEBUG);  // Set log level for BLE device
    esp_log_level_set("bt", ESP_LOG_DEBUG);  // Set log level for BLE device
    esp_log_level_set("BLEClient", ESP_LOG_DEBUG);  // Set log level for BLE device


    // Scan for BLE Server
    scan();
}

void BLEClientController::scan() {
    BLEScan* pBLEScan = BLEDevice::getScan();
    pBLEScan->setAdvertisedDeviceCallbacks(this);  // Use this class as the callback handler
    pBLEScan->setActiveScan(true);
    pBLEScan->start(BLE_SCAN_DURATION_SECONDS);
}

void BLEClientController::registerTempReadCallback(temp_read_callback_t callback) {
    tempReadCallback = callback;
}

void BLEClientController::registerRemoteErrorCallback(remote_err_callback_t callback) {
    remoteErrorCallback = callback;
}

bool BLEClientController::connectToServer() {
    BLEClient* pClient = BLEDevice::createClient();
    if (pClient == NULL) Serial.println("Failed to create BLE client");
    Serial.println("Connecting to advertised device");

    unsigned int tries = 0;
    while (!pClient->isConnected()) {
        if (!pClient->connect(BLEAddress(serverDevice->getAddress()))) {
            Serial.println("Failed connecting to BLE server. Retrying...");
        }

        tries++;

        if (tries >= MAX_CONNECT_RETRIES) {
            Serial.println("Connection timeout! Unable to connect to BLE server.");
            scan();
            return false;  // Exit the connection attempt if timed out
        }

        delay(500);  // Add a small delay to avoid busy-waiting
    }

    Serial.println("Successfully connected to BLE server");

    // Obtain the remote service we wish to connect to
    BLERemoteService* pRemoteService = pClient->getService(BLEUUID(SERVICE_UUID));
    if (pRemoteService == nullptr) {
        Serial.println("Error getting remote service");
        return false;
    }

    // Obtain the remote write characteristics
    tempControlChar = pRemoteService->getCharacteristic(BLEUUID(TEMP_CONTROL_CHAR_UUID));
    pinControlChar = pRemoteService->getCharacteristic(BLEUUID(PIN_CONTROL_CHAR_UUID));
    autotuneChar = pRemoteService->getCharacteristic(BLEUUID(AUTOTUNE_CHAR_UUID));
    pingChar = pRemoteService->getCharacteristic(BLEUUID(PING_CHAR_UUID));

    // Obtain the remote notify characteristic and subscribe to it
    tempReadChar = pRemoteService->getCharacteristic(BLEUUID(TEMP_READ_CHAR_UUID));
    if (tempReadChar->canNotify()) {
        tempReadChar->registerForNotify(*this->notifyCallback);
    }

    errorChar = pRemoteService->getCharacteristic(BLEUUID(ERROR_CHAR_UUID));
    if (errorChar->canNotify()) {
        errorChar->registerForNotify(*this->notifyCallback);
    }

    readyForConnection = false;
    return true;
}

void BLEClientController::sendTemperatureControl(float setpoint) {
    if (tempControlChar != nullptr) {
        char tempStr[8];
        snprintf(tempStr, sizeof(tempStr), "%.2f", setpoint);
        tempControlChar->writeValue(tempStr);
    }
}

void BLEClientController::sendPinControl(bool pinState) {
    if (pinControlChar != nullptr) {
        pinControlChar->writeValue(pinState ? "1" : "0");
    }
}

void BLEClientController::sendPing() {
    if (pingChar != nullptr) {
        pingChar->writeValue(millis());
    }
}

void BLEClientController::sendAutotune() {
    if (autotuneChar != nullptr) {
        autotuneChar->writeValue(millis());
    }
}

bool BLEClientController::isReadyForConnection() {
    return readyForConnection;
}

// BLEAdvertisedDeviceCallbacks override
void BLEClientController::onResult(BLEAdvertisedDevice advertisedDevice) {
    Serial.printf("Advertised Device found: %s \n", advertisedDevice.toString().c_str());

    // Check if this is the device we're looking for
    if (advertisedDevice.haveServiceUUID()) {
        Serial.println("Found BLE service. Checking for ID...");
        if (advertisedDevice.isAdvertisingService(BLEUUID(SERVICE_UUID))) {
            Serial.println("Found target BLE device. Connecting...");
            BLEDevice::getScan()->stop();  // Stop scanning once we find the correct device
            serverDevice = &advertisedDevice;
            readyForConnection = true;
        }
    }
}

// Notification callback
void BLEClientController::notifyCallback(BLERemoteCharacteristic *pRemoteCharacteristic, uint8_t *pData, size_t length, bool isNotify) {
    // Process notifications from the server (e.g., temperature data)
    if (pRemoteCharacteristic->getUUID().equals(BLEUUID(TEMP_READ_CHAR_UUID))) {
        float temperature = atof((char*)pData);
        Serial.printf("Temperature read: %.2f\n", temperature);
        if (tempReadCallback != nullptr) {
            tempReadCallback(temperature);
        }
    }
    if (pRemoteCharacteristic->getUUID().equals(BLEUUID(ERROR_CHAR_UUID))) {
        int errorCode = atoi((char*)pData);
        Serial.printf("Error read: %d\n", errorCode);
        if (remoteErrorCallback != nullptr) {
            remoteErrorCallback(errorCode);
        }
    }
}