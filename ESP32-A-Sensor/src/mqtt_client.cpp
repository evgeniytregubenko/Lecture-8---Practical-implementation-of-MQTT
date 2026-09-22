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
// КОНФІГУРАЦІЯ MQTT-КЛІЄНТА
// ============================================================

void initMQTT() {

    mqttClient.setServer(MQTT_BROKER, MQTT_PORT); // Адреса та порт MQTT-брокера

    mqttClient.setKeepAlive(MQTT_KEEPALIVE_SEC); // Інтервал MQTT Keep Alive

    mqttClient.setSocketTimeout(MQTT_SOCKET_TIMEOUT_SEC); // Таймаут мережевого сокета
}

// ============================================================
// ЗМІННІ ДЛЯ ПОВТОРНОГО ПІДКЛЮЧЕННЯ MQTT
// ============================================================

// static зберігає значення змінних між викликами mqttLoop()
// та обмежує їх видимість поточним файлом mqtt_client.cpp

static unsigned long lastMQTTReconnectTime = 0; // Час останньої спроби підключення

static uint8_t mqttReconnectAttempts = 0; // Кількість спроб у поточному циклі

static unsigned long mqttRetryCycleStartTime = 0; // Час початку паузи між циклами

static bool mqttRetryCyclePaused = false; // Ознака паузи після 3 невдалих спроб

// ============================================================
// ЗМІННІ ДЛЯ ПОВТОРНОГО ПІДКЛЮЧЕННЯ WI-FI
// ============================================================

static unsigned long lastWiFiReconnectTime = 0;
static bool wifiReconnectInProgress = false; // Ознака процесу відновлення Wi-Fi

// ============================================================
// ПІДКЛЮЧЕННЯ ДО WI-FI
// ============================================================

bool connectWiFi() {

    Serial.print("Connecting to Wi-Fi: ");
    Serial.println(WIFI_SSID);

    WiFi.begin(WIFI_SSID, WIFI_PASSWORD);

    unsigned long startTime = millis();

    while (WiFi.status() != WL_CONNECTED) {

        if ((millis() - startTime) >= WIFI_TIMEOUT) { // Перевірка таймауту підключення

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
// ТЕСТОВЕ ВІДКЛЮЧЕННЯ WI-FI
// ============================================================

void disconnectWiFi() {

    Serial.println("TEST: Wi-Fi disconnect");  // Виведення повідомлення про тестове відключення Wi-Fi

    WiFi.disconnect();
}

// ============================================================
// ОБСЛУГОВУВАННЯ WI-FI
// ============================================================

void wifiLoop() {

    if (WiFi.status() == WL_CONNECTED) { // Якщо Wi-Fi підключений

        if (wifiReconnectInProgress) { // Якщо перед цим виконувалось повторне підключення

            Serial.println("Wi-Fi reconnected");

            Serial.print("IP address: ");
            Serial.println(WiFi.localIP());

            wifiReconnectInProgress = false;
        }

        return;
    }

    unsigned long currentTime = millis();

    if ((currentTime - lastWiFiReconnectTime) < WIFI_RETRY_DELAY) { // Очікуємо 5 секунд між спробами reconnect

        return;
    }

    lastWiFiReconnectTime = currentTime;

    wifiReconnectInProgress = true;

    Serial.println("Wi-Fi reconnect attempt");

    WiFi.reconnect();
}

// ============================================================
// ПІДКЛЮЧЕННЯ ДО MQTT
// ============================================================

bool connectMQTT() {

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

    if (WiFi.status() != WL_CONNECTED) { // Якщо Wi-Fi не підключений, MQTT reconnect неможливий

        return;
    }

    if (mqttClient.connected()) { // Якщо MQTT підключений

        mqttReconnectAttempts = 0;
        mqttRetryCyclePaused = false;

        mqttClient.loop();

        return;
    }

    unsigned long currentTime = millis();

    // ========================================================
    // ПАУЗА МІЖ ЦИКЛАМИ RECONNECT
    // ========================================================

    if (mqttRetryCyclePaused) {

        if ((currentTime - mqttRetryCycleStartTime) < MQTT_RETRY_CYCLE_DELAY) { // Після трьох невдалих спроб очікуємо 60 секунд

            return;
        }

        Serial.println("MQTT: starting new reconnect cycle");

        mqttReconnectAttempts = 0;
        mqttRetryCyclePaused = false;
    }

    // ========================================================
    // ІНТЕРВАЛ МІЖ СПРОБАМИ
    // ========================================================

    if ((currentTime - lastMQTTReconnectTime) < MQTT_RETRY_DELAY) {

        return;
    }

    lastMQTTReconnectTime = currentTime;

    mqttReconnectAttempts++;

    Serial.print("MQTT reconnect attempt ");
    Serial.print(mqttReconnectAttempts);
    Serial.print("/");
    Serial.println(MQTT_MAX_RETRIES);

    // ========================================================
    // СПРОБА ПОВТОРНОГО ПІДКЛЮЧЕННЯ
    // ========================================================

    if (connectMQTT()) {

        mqttReconnectAttempts = 0;
        mqttRetryCyclePaused = false;

        return;
    }

    // ========================================================
    // ТРИ СПРОБИ ВИЧЕРПАНО
    // ========================================================

    if (mqttReconnectAttempts >= MQTT_MAX_RETRIES) {

        Serial.println("MQTT: 3 attempts failed, next cycle in 60 s");

        mqttRetryCycleStartTime = currentTime;
        mqttRetryCyclePaused = true;
    }
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

        Serial.print("Published. Time: ");
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