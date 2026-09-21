#include <Arduino.h>
#include <WiFi.h>
#include <PubSubClient.h>

#include "config.h"
#include "mqtt_client.h"

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
// MQTT КЛІЄНТ
// ============================================================

WiFiClient wifiClient; // TCP-клієнт для мережевого з'єднання

PubSubClient mqttClient(wifiClient); // MQTT-клієнт, який використовує TCP-з'єднання wifiClient

// ============================================================
// ВНУТРІШНІ ФУНКЦІЇ MQTT
// ============================================================

static void mqttCallback(char* topic, byte* payload, unsigned int length);
static void subscribeMQTTTopics();

// ============================================================
// ЗМІННІ ДЛЯ ПОВТОРНОГО ПІДКЛЮЧЕННЯ MQTT
// ============================================================

// static зберігає значення змінних між викликами функцій
// та обмежує їх видимість поточним файлом mqtt_client.cpp

static unsigned long lastMQTTReconnectTime = 0; // Час останньої спроби підключення
static uint8_t mqttReconnectAttempts = 0;       // Кількість спроб повторного підключення

// ============================================================
// ОТРИМАНІ ДАНІ MQTT
// ============================================================

static float receivedTemperature = 0.0;       // Остання отримана температура
static bool newTemperatureReceived = false;   // Ознака отримання нового значення температури
static bool manualReadReceived = false;        // Ознака отримання команди manual_read

// ============================================================
// ОБРОБКА ВХІДНИХ MQTT-ПОВІДОМЛЕНЬ
// ============================================================

static void mqttCallback(char* topic, byte* payload, unsigned int length) {

    char message[32]; // Буфер для перетворення payload у C-рядок

    if (length >= sizeof(message)) { // Перевіряємо, чи повідомлення поміститься у буфер

        return;
    }

    memcpy(message, payload, length); // Копіюємо отримані байти у символьний масив

    message[length] = '\0'; // Додаємо символ завершення C-рядка

    // ========================================================
    // ТЕМПЕРАТУРА
    // ========================================================

    if (strcmp(topic, MQTT_TOPIC_TEMPERATURE) == 0) {

        receivedTemperature = atof(message); // Перетворення тексту у float

        newTemperatureReceived = true; // Встановлення прапорця нових даних

        return;
    }

    // ========================================================
    // КОМАНДА MANUAL_READ
    // ========================================================

    if (strcmp(topic, MQTT_TOPIC_COMMANDS) == 0) {

        if (strcmp(message, "manual_read") == 0) {

            manualReadReceived = true; // Встановлення прапорця отримання команди
        }
    }
}

// ============================================================
// ПІДКЛЮЧЕННЯ ДО MQTT
// ============================================================

bool connectMQTT() {

    mqttClient.setServer(MQTT_BROKER, MQTT_PORT); // Адреса та порт MQTT-брокера

    mqttClient.setCallback(mqttCallback); // Реєстрація callback-функції

    mqttClient.setKeepAlive(MQTT_KEEPALIVE_SEC); // Інтервал MQTT Keep Alive

    mqttClient.setSocketTimeout(MQTT_SOCKET_TIMEOUT_SEC); // Таймаут мережевого сокета

    Serial.println("Connecting to MQTT broker...");

    if (mqttClient.connect(
        MQTT_CLIENT_ID,     // Client ID
        MQTT_TOPIC_STATUS,  // Last Will topic
        0,                  // Last Will QoS
        true,               // Retain
        "offline"           // Last Will message
    )) {

    Serial.println("MQTT connected");

    subscribeMQTTTopics();

    mqttClient.publish( // Публікуємо поточний стан пристрою
        MQTT_TOPIC_STATUS,
        "online",
        true
    );

    Serial.println("Status published: online");

    return true;
}

    Serial.print("MQTT connection failed, state: ");
    Serial.println(mqttClient.state());

    return false;
}

// ============================================================
// ПІДПИСКА НА MQTT-ТОПІКИ
// ============================================================

static void subscribeMQTTTopics() {

    if (mqttClient.subscribe(MQTT_TOPIC_TEMPERATURE, 1)) { // Підписка на температуру з QoS 1

        Serial.println("Subscribed to temperature topic, QoS 1");

    } else {

        Serial.println("Temperature subscription failed");
    }

    // Підписка на команди з QoS 0
    if (mqttClient.subscribe(MQTT_TOPIC_COMMANDS, 0)) {

        Serial.println("Subscribed to commands topic, QoS 0");

    } else {

        Serial.println("Commands subscription failed");
    }
}

// ============================================================
// ОБСЛУГОВУВАННЯ MQTT
// ============================================================

void mqttLoop() {

    if (WiFi.status() != WL_CONNECTED) { // Якщо Wi-Fi не підключений, повторне підключення до MQTT неможливе

        return;
    }

    if (mqttClient.connected()) { // Якщо MQTT підключений

        mqttReconnectAttempts = 0; // Скидаємо лічильник спроб

        mqttClient.loop(); // Обслуговування MQTT-з'єднання та вхідних повідомлень

        return;
    }

    if (mqttReconnectAttempts >= MQTT_MAX_RETRIES) { // Якщо вже виконано максимальну кількість спроб

        return;
    }

    unsigned long currentTime = millis();

    if ((currentTime - lastMQTTReconnectTime) < MQTT_RETRY_DELAY) { // Перевіряємо, чи минув інтервал між спробами reconnect

        return;
    }

    lastMQTTReconnectTime = currentTime; // Запам'ятовуємо час поточної спроби

    mqttReconnectAttempts++; // Збільшуємо лічильник спроб

    Serial.print("MQTT reconnect attempt ");
    Serial.print(mqttReconnectAttempts);
    Serial.print("/");
    Serial.println(MQTT_MAX_RETRIES);

    if (mqttClient.connect( // Спроба повторного підключення з Last Will
        MQTT_CLIENT_ID,
        MQTT_TOPIC_STATUS,
        0,
        true,
        "offline"
    )) {

    Serial.println("MQTT reconnected");   

    mqttReconnectAttempts = 0;

    subscribeMQTTTopics(); // Після reconnect необхідно знову підписатися на топіки

    mqttClient.publish( // Публікуємо поточний стан пристрою
        MQTT_TOPIC_STATUS,
        "online",
        true
    );

    Serial.println("Status published: online");

    return;
}

    Serial.print("MQTT reconnect failed, state: ");
    Serial.println(mqttClient.state());
}

 // ============================================================
// ОТРИМАННЯ ТЕМПЕРАТУРИ
// ============================================================

bool getReceivedTemperature(float &temperature) {

    if (!newTemperatureReceived) { // Якщо нових даних температури немає

        return false;
    }

    temperature = receivedTemperature; // Передаємо отриману температуру

    newTemperatureReceived = false; // Скидаємо прапорець після обробки даних

    return true;
}

// ============================================================
// ОТРИМАННЯ КОМАНДИ MANUAL_READ
// ============================================================

bool getManualReadCommand() {

    if (!manualReadReceived) { // Якщо команда manual_read не була отримана

        return false;
    }

    manualReadReceived = false; // Скидаємо прапорець після обробки команди

    return true;
}

// ============================================================
// ПУБЛІКАЦІЯ СТАНУ LED
// ============================================================

bool publishLEDState(bool state) {

    if (!mqttClient.connected()) {

        Serial.println("MQTT not connected");

        return false;
    }

    const char* message;

    if (state) {

        message = "ON";

    } else {

        message = "OFF";
    }

    bool published = mqttClient.publish(
        MQTT_TOPIC_LED,
        message
    );

    if (published) {

        Serial.print("LED state published: ");
        Serial.println(message);

        return true;
    }

    Serial.println("LED state publish failed");

    return false;
}