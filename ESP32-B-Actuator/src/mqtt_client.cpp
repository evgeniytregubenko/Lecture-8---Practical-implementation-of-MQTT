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
// ВНУТРІШНІ ФУНКЦІЇ MQTT
// ============================================================

static void mqttCallback(char* topic, byte* payload, unsigned int length);
static void subscribeMQTTTopics();

// ============================================================
// КОНФІГУРАЦІЯ MQTT-КЛІЄНТА
// ============================================================

void initMQTT() {

    mqttClient.setServer(MQTT_BROKER, MQTT_PORT); // Адреса та порт MQTT-брокера

    mqttClient.setCallback(mqttCallback); // Реєстрація callback-функції

    mqttClient.setKeepAlive(MQTT_KEEPALIVE_SEC); // Інтервал MQTT Keep Alive

    mqttClient.setSocketTimeout(MQTT_SOCKET_TIMEOUT_SEC); // Таймаут мережевого сокета
}

// ============================================================
// ЗМІННІ ДЛЯ ПОВТОРНОГО ПІДКЛЮЧЕННЯ MQTT
// ============================================================

// static зберігає значення змінних між викликами функцій
// та обмежує їх видимість поточним файлом mqtt_client.cpp

static unsigned long lastMQTTReconnectTime = 0; // Час останньої спроби підключення

static uint8_t mqttReconnectAttempts = 0; // Кількість спроб у поточному циклі

static unsigned long mqttRetryCycleStartTime = 0; // Час початку паузи між циклами

static bool mqttRetryCyclePaused = false; // Ознака паузи після 3 невдалих спроб

// ============================================================
// ЗМІННІ ДЛЯ ПОВТОРНОГО ПІДКЛЮЧЕННЯ WI-FI
// ============================================================

static unsigned long lastWiFiReconnectTime = 0; // Час останньої спроби підключення Wi-Fi

static bool wifiReconnectInProgress = false; // Ознака процесу відновлення Wi-Fi

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

    Serial.println("Connecting to MQTT broker...");

    if (mqttClient.connect(
        MQTT_CLIENT_ID,     // Client ID
        MQTT_TOPIC_STATUS,  // Last Will topic
        0,                  // Last Will QoS
        true,               // Retain
        "offline"           // Last Will message
    )) {

        Serial.println("MQTT connected");

        subscribeMQTTTopics(); // Після підключення підписуємося на топіки

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

    if (WiFi.status() != WL_CONNECTED) { // Якщо Wi-Fi не підключений, MQTT reconnect неможливий

        return;
    }

    if (mqttClient.connected()) { // Якщо MQTT підключений

        mqttReconnectAttempts = 0;
        mqttRetryCyclePaused = false;

        mqttClient.loop(); // Обслуговування MQTT та вхідних повідомлень

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
// ОБСЛУГОВУВАННЯ WI-FI
// ============================================================

void wifiLoop() {

    if (WiFi.status() == WL_CONNECTED) {

        if (wifiReconnectInProgress) {

            Serial.println("Wi-Fi reconnected");

            Serial.print("IP address: ");
            Serial.println(WiFi.localIP());

            wifiReconnectInProgress = false;
        }

        return;
    }

    unsigned long currentTime = millis();

    if ((currentTime - lastWiFiReconnectTime) < WIFI_RETRY_DELAY) {

        return;
    }

    lastWiFiReconnectTime = currentTime;

    wifiReconnectInProgress = true;

    Serial.println("Wi-Fi reconnect attempt");

    WiFi.reconnect();
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