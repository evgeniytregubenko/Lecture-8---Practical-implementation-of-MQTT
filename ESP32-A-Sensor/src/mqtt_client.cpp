#include <Arduino.h>
#include <WiFi.h>
#include <PubSubClient.h>

#include "config.h"
#include "mqtt_client.h"

// ============================================================
// MQTT КЛІЄНТ
// ============================================================

WiFiClient wifiClient; // TCP-клієнт для мережевого з'єднання

PubSubClient mqttClient(wifiClient); // MQTT-клієнт, який використовує TCP-з'єднання wifiClient

// ============================================================
// ЗМІННІ ДЛЯ ПОВТОРНОГО ПІДКЛЮЧЕННЯ MQTT
// ============================================================

// static зберігає значення змінних між викликами mqttLoop()
// та обмежує їх видимість поточним файлом mqtt_client.cpp

static unsigned long lastMQTTReconnectTime = 0; // Час останньої спроби підключення
static uint8_t mqttReconnectAttempts = 0;       // Кількість спроб повторного підключення

// ============================================================
// ПІДКЛЮЧЕННЯ ДО WI-FI
// ============================================================

bool connectWiFi() {

    Serial.print("Connecting to Wi-Fi: ");
    Serial.println(WIFI_SSID);

    WiFi.begin(WIFI_SSID, WIFI_PASSWORD);

    unsigned long startTime = millis();

    while (WiFi.status() != WL_CONNECTED) {

        // Перевірка таймауту підключення
        if ((millis() - startTime) >= WIFI_TIMEOUT) {

            Serial.println();
            Serial.println("Wi-Fi connection failed");

            return false;
        }

        delay(250);
        Serial.print(".");
    }

    Serial.println();
    Serial.println("Wi-Fi connected");

    Serial.print("IP address: ");
    Serial.println(WiFi.localIP());

    return true;
}

// ============================================================
// ПІДКЛЮЧЕННЯ ДО MQTT
// ============================================================

bool connectMQTT() {

    mqttClient.setServer(MQTT_BROKER, MQTT_PORT); // Встановлення адреси та порту MQTT-брокера
    mqttClient.setKeepAlive(MQTT_KEEPALIVE); // Встановлення інтервалу keep-alive
    mqttClient.setSocketTimeout(MQTT_SOCKET_TIMEOUT); // Встановлення таймауту сокета в секундах

    Serial.println("Connecting to MQTT broker...");

    if (mqttClient.connect(MQTT_CLIENT_ID)) { // Одна спроба підключення до MQTT-брокера

        Serial.println("MQTT connected");

        return true;
    }

    // Виведення коду помилки MQTT
    Serial.print("MQTT connection failed, state: ");
    Serial.println(mqttClient.state());

    return false;
}

// ============================================================
// ОБСЛУГОВУВАННЯ MQTT
// ============================================================

void mqttLoop() {

    if (WiFi.status() != WL_CONNECTED) { // Перевіряємо підключення до Wi-Fi

        return;
    }

    if (mqttClient.connected()) { // Якщо MQTT підключений

        mqttReconnectAttempts = 0; // Скидаємо лічильник спроб

        mqttClient.loop(); // Обслуговування MQTT-з'єднання

        return;
    }

    if (mqttReconnectAttempts >= MQTT_MAX_RETRIES) { // Якщо вже виконано максимальну кількість спроб

        return;
    }

    unsigned long currentTime = millis();

    if ((currentTime - lastMQTTReconnectTime) < MQTT_RETRY_DELAY) { // Очікуємо MQTT_RETRY_DELAY перед наступною спробою

        return;
    }

    lastMQTTReconnectTime = currentTime; // Запам'ятовуємо час поточної спроби

    mqttReconnectAttempts++; // Збільшуємо лічильник спроб

    Serial.print("MQTT reconnect attempt ");
    Serial.print(mqttReconnectAttempts);
    Serial.print("/");
    Serial.println(MQTT_MAX_RETRIES);

    if (mqttClient.connect(MQTT_CLIENT_ID)) { // Спроба повторного підключення

        Serial.println("MQTT reconnected");

        mqttReconnectAttempts = 0;

        return;
    }

    Serial.print("MQTT reconnect failed, state: ");
    Serial.println(mqttClient.state());
}

// ============================================================
// ПУБЛІКАЦІЯ ДАНИХ СЕНСОРА
// ============================================================

bool publishSensorData(const SensorData &data) {

    if (!mqttClient.connected()) { // Перевіряємо підключення до MQTT-брокера

        Serial.println("MQTT not connected");

        return false;
    }

    // Буфери для перетворення числових значень у текст
    char temperatureBuffer[16];
    char humidityBuffer[16];

    snprintf(   // Перетворення температури у текстовий формат
        temperatureBuffer,
        sizeof(temperatureBuffer),
        "%.2f",
        data.dht.temperature
    );

    snprintf( // Перетворення вологості у текстовий формат
        humidityBuffer,
        sizeof(humidityBuffer),
        "%.2f",
        data.dht.humidity
    );

    bool temperaturePublished = mqttClient.publish( // Публікація температури
        MQTT_TOPIC_TEMPERATURE,
        temperatureBuffer
    );

    bool humidityPublished = mqttClient.publish( // Публікація вологості
        MQTT_TOPIC_HUMIDITY,
        humidityBuffer
    );

    if (temperaturePublished && humidityPublished) { // Перевірка результату публікації

        Serial.print("Publushed. Time: ");
        Serial.print(data.timestamp / 1000);
        Serial.print(" s | Temperature: ");
        Serial.print(temperatureBuffer);
        Serial.print(" C | Humidity: ");
        Serial.print(humidityBuffer);
        Serial.println(" %");

        return true;
    }

    Serial.println("MQTT publish failed");

    return false;
}

// ============================================================
// ПУБЛІКАЦІЯ КОМАНДИ MANUAL_READ
// ============================================================

bool publishManualRead() {

    if (!mqttClient.connected()) { // Перевіряємо підключення до MQTT-брокера

        Serial.println("MQTT not connected");

        return false;
    }

    bool published = mqttClient.publish( // Публікація команди manual_read
        MQTT_TOPIC_COMMANDS,
        "manual_read"
    );

    if (published) { // Перевірка результату публікації

        Serial.println("Command published: manual_read");

        return true;
    }

    Serial.println("Command publish failed");

    return false;
}