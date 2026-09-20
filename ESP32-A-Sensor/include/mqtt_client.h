#pragma once

#include "data.h"

bool connectWiFi(); // Підключення до Wi-Fi

bool connectMQTT(); // Підключення до MQTT-брокера

void mqttLoop(); // Обслуговування MQTT-з'єднання

bool publishSensorData(const SensorData &data); // Публікація температури та вологості

bool publishManualRead(); // Публікація команди manual_read