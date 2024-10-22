// BLEServerController.cpp

#include "BLEServerController.h"

BLEServerController::BLEServerController() :
    deviceConnected(false),
    tempControlChar(nullptr),
    pinControlChar(nullptr),
    tempReadChar(nullptr),
    pingChar(nullptr),
    errorChar(nullptr),
    autotuneChar(nullptr),
    tempControlCallback(nullptr),
    pinControlCallback(nullptr),
    pingCallback(nullptr),
    autotuneCallback(nullptr) {}

void BLEServerController::initServer() {
    BLEDevice::init("GPBLS");
    BLEDevice::setPower(ESP_PWR_LVL_P9);  // Set to maximum power
    esp_log_level_set("*", ESP_LOG_INFO);
    esp_log_level_set("BLEDevice", ESP_LOG_DEBUG);  // Set log level for BLE device
    esp_log_level_set("bt", ESP_LOG_DEBUG);  // Set log level for BLE device
    esp_log_level_set("BLEServer", ESP_LOG_DEBUG);  // Set log level for BLE device

    // Create BLE Server
    BLEServer *pServer = BLEDevice::createServer();
    pServer->setCallbacks(this);  // Use this class as the callback handler

    // Create BLE Service
    BLEService *pService = pServer->createService(SERVICE_UUID);

    // Temperature Control Characteristic (Client writes setpoint)
    tempControlChar = pService->createCharacteristic(
                        TEMP_CONTROL_CHAR_UUID,
                        BLECharacteristic::PROPERTY_WRITE
                      );
    tempControlChar->setCallbacks(this);  // Use this class as the callback handler

    // Pin Control Characteristic (Client writes pin state)
    pinControlChar = pService->createCharacteristic(
                        PIN_CONTROL_CHAR_UUID,
                        BLECharacteristic::PROPERTY_WRITE
                     );
    pinControlChar->setCallbacks(this);  // Use this class as the callback handler

    // Temperature Read Characteristic (Server notifies client of temperature)
    tempReadChar = pService->createCharacteristic(
                     TEMP_READ_CHAR_UUID,
                     BLECharacteristic::PROPERTY_NOTIFY
                   );
    tempReadChar->addDescriptor(new BLE2902());

    // Ping Characteristic (Client writes ping, Server reads)
    pingChar = pService->createCharacteristic(
                 PING_CHAR_UUID,
                 BLECharacteristic::PROPERTY_WRITE
               );
    pingChar->setCallbacks(this);  // Use this class as the callback handler

    // Error Characteristic (Server writes error, Client reads)
    errorChar = pService->createCharacteristic(
                 ERROR_CHAR_UUID,
                 BLECharacteristic::PROPERTY_NOTIFY
               );
    errorChar->setCallbacks(this);  // Use this class as the callback handler

    // Ping Characteristic (Client writes autotune, Server reads)
    autotuneChar = pService->createCharacteristic(
                 AUTOTUNE_CHAR_UUID,
                 BLECharacteristic::PROPERTY_WRITE
               );
    autotuneChar->setCallbacks(this);  // Use this class as the callback handler

    pService->start();
    BLEAdvertising *pAdvertising = BLEDevice::getAdvertising();
    pAdvertising->addServiceUUID(SERVICE_UUID);
    pAdvertising->setScanResponse(true);
    BLEDevice::startAdvertising();
    Serial.println("BLE Server started, advertising...");
}

void BLEServerController::sendTemperature(float temperature) {
    if (deviceConnected) {
        // Send temperature notification to the client
        char tempStr[8];
        snprintf(tempStr, sizeof(tempStr), "%.2f", temperature);
        tempReadChar->setValue(tempStr);
        tempReadChar->notify();
    }
}

void BLEServerController::sendError(int errorCode) {
    if (deviceConnected) {
        // Send temperature notification to the client
        char errorStr[8];
        snprintf(errorStr, sizeof(errorStr), "%d", errorCode);
        errorChar->setValue(errorStr);
        errorChar->notify();
    }
}

void BLEServerController::registerTempControlCallback(temp_control_callback_t callback) {
    tempControlCallback = callback;
}

void BLEServerController::registerPinControlCallback(pin_control_callback_t callback) {
    pinControlCallback = callback;
}

void BLEServerController::registerPingCallback(ping_callback_t callback) {
    pingCallback = callback;
}

void BLEServerController::registerAutotuneCallback(autotune_callback_t callback) {
    autotuneCallback = callback;
}

// BLEServerCallbacks override
void BLEServerController::onConnect(BLEServer* pServer) {
    Serial.println("Client connected.");
    deviceConnected = true;
}

void BLEServerController::onDisconnect(BLEServer* pServer) {
    Serial.println("Client disconnected.");
    deviceConnected = false;
    pServer->startAdvertising();  // Restart advertising so clients can reconnect
}

// BLECharacteristicCallbacks override
void BLEServerController::onWrite(BLECharacteristic* pCharacteristic) {
    Serial.println("Write received!");

    if (pCharacteristic->getUUID().equals(BLEUUID(TEMP_CONTROL_CHAR_UUID))) {
        float setpoint = atof(pCharacteristic->getValue().c_str());
        Serial.printf("Received temperature setpoint: %.2f\n", setpoint);
        if (tempControlCallback != nullptr) {
            tempControlCallback(setpoint);
        }
    }
    else if (pCharacteristic->getUUID().equals(BLEUUID(PIN_CONTROL_CHAR_UUID))) {
        bool pinState = (pCharacteristic->getValue()[0] == '1');
        Serial.printf("Received pin control: %s\n", pinState ? "ON" : "OFF");
        if (pinControlCallback != nullptr) {
            pinControlCallback(pinState);
        }
    }
    else if (pCharacteristic->getUUID().equals(BLEUUID(PING_CHAR_UUID))) {
        Serial.printf("Received ping\n");
        if (pingCallback != nullptr) {
            pingCallback();
        }
    }
    else if (pCharacteristic->getUUID().equals(BLEUUID(AUTOTUNE_CHAR_UUID))) {
        Serial.printf("Received autotune\n");
        if (autotuneCallback != nullptr) {
            autotuneCallback();
        }
    }
}
