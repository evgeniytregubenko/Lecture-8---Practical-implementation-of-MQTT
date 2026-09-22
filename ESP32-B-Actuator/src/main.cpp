#include <Arduino.h>

#include "config.h"
#include "actuator.h"
#include "mqtt_client.h"

// ============================================================
// SETUP
// ============================================================

void setup() {

    Serial.begin(115200);

    delay(500);

    Serial.println();
    Serial.println("ESP32-B started");

    initActuator(); // Ініціалізація світлодіода

    initMQTT(); // Ініціалізація MQTT-клієнта

    if (!connectWiFi()) { // Підключення до Wi-Fi

        Serial.println("Continue working without Wi-Fi");

        return;
    }

    if (!connectMQTT()) { // Підключення до MQTT-брокера

        Serial.println("Continue working without MQTT");
    }
}

// ============================================================
// LOOP
// ============================================================

void loop() {

    wifiLoop(); // Обслуговування Wi-Fi-з'єднання
    mqttLoop(); // Обслуговування MQTT-з'єднання та вхідних повідомлень

    actuatorLoop(); // Обслуговування неблокуючого мигання LED

    // ========================================================
    // ОБРОБКА ТЕМПЕРАТУРИ
    // ========================================================

    float temperature;

    if (getReceivedTemperature(temperature)) {

    Serial.print("Temperature received: ");
    Serial.print(temperature);
    Serial.println(" C");

    bool previousLEDState = getLEDState(); // Запам'ятовуємо стан LED до обробки температури

    updateLEDByTemperature(temperature); // Оновлюємо стан LED відповідно до температури

    if (getLEDState() != previousLEDState) { // Якщо стан LED змінився - публікуємо його через MQTT

        publishLEDState(getLEDState());
    }
}

    // ========================================================
    // ОБРОБКА КОМАНДИ MANUAL_READ
    // ========================================================

    if (getManualReadCommand()) {

        Serial.println("Manual trigger received");

        startManualBlink();
    }
}