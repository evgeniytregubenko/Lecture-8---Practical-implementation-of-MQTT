#include <Arduino.h>
#include <WiFi.h>
#include <WiFiClientSecure.h>
#include <PubSubClient.h>
#include <time.h>


#include "config.h"
#include "mqtt_client.h"
#include "secrets.h"

// ============================================================
// MQTT КЛІЄНТ
// ============================================================

WiFiClientSecure wifiClient; // TCP-клієнт для захищеного мережевого з'єднання

PubSubClient mqttClient(wifiClient); // MQTT-клієнт, який використовує TCP-з'єднання wifiClient

// ============================================================
// КОНФІГУРАЦІЯ MQTT-КЛІЄНТА
// ============================================================

void initMQTT() {

    wifiClient.setCACert(AWS_CERT_CA); // Кореневий сертифікат Amazon

    wifiClient.setCertificate(AWS_CERT_CRT);  //  Сертифікат пристрою ESP32

    wifiClient.setPrivateKey(AWS_CERT_PRIVATE); // Приватний ключ пристрою

    mqttClient.setServer(AWS_IOT_ENDPOINT, MQTT_PORT); // AWS IoT Core endpoint, порт 8883

    mqttClient.setBufferSize(512); // Розмір буфера MQTT для JSON-повідомлень

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
// ЗМІННІ ДЛЯ СИНХРОНІЗАЦІЇ ЧАСУ
// ============================================================

static bool timeSynchronized = false;  //  

static unsigned long lastTimeSyncAttempt = 0;  //  



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
// СИНХРОНІЗАЦІЯ ЧАСУ ЧЕРЕЗ NTP
// ============================================================

bool syncTime() {

    Serial.println("Synchronizing time...");

    configTime(
        NTP_GMT_OFFSET_SEC,
        NTP_DAYLIGHT_OFFSET,
        NTP_SERVER
    );

    struct tm timeInfo;
    unsigned long startTime = millis();

    while (!getLocalTime(&timeInfo)) {

        if (millis() - startTime >= NTP_TIMEOUT) {

            Serial.println("Time synchronization failed");

            timeSynchronized = false;

            return false;
        }

        delay(100);
    }

    timeSynchronized = true;

    Serial.println("Time synchronized");

    Serial.print("Current time: ");
    Serial.println(&timeInfo, "%Y-%m-%d %H:%M:%S");

    return true;
}

// ============================================================
// ПІДКЛЮЧЕННЯ ДО MQTT
// ============================================================

bool connectMQTT() {

    if (WiFi.status() != WL_CONNECTED) { // MQTT-підключення неможливе без Wi-Fi

        return false;
    }

    if (!timeSynchronized) { // TLS-підключення до AWS виконуємо тільки після синхронізації часу

        Serial.println("MQTT connection skipped: time is not synchronized");

        return false;
    }

    Serial.println("Connecting to AWS IoT Core...");

    if (mqttClient.connect(THINGNAME)) { // Підключення до AWS IoT Core з використанням Thing Name як Client ID

        Serial.println("MQTT connected");

        return true;
    }

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

    // ========================================================
    // ПЕРЕВІРКА СИНХРОНІЗАЦІЇ ЧАСУ
    // ========================================================

    if (!timeSynchronized) {

        unsigned long currentTime = millis();

        if (currentTime - lastTimeSyncAttempt >= NTP_RETRY_DELAY) {

            lastTimeSyncAttempt = currentTime;

            syncTime();
        }

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

    // Закриваємо попереднє TLS-з'єднання перед новою спробою
    wifiClient.stop();
    
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

    char payload[256]; // Буфер для формування JSON-повідомлення

    time_t currentTimestamp = time(nullptr); // Отримуємо поточний Unix timestamp

    snprintf(
        payload,
        sizeof(payload),
        "{\"device_id\":\"%s\",\"timestamp\":%ld,\"temperature\":%.2f,\"humidity\":%.2f}",
        THINGNAME,
        (long)currentTimestamp,
        data.dht.temperature,
        data.dht.humidity
    );

    bool published = mqttClient.publish( // Публікація JSON у єдиний топік sensors/data
        MQTT_TOPIC_DATA,
        payload
    );

    if (published) {

        Serial.print("Published: ");
        Serial.println(payload);

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