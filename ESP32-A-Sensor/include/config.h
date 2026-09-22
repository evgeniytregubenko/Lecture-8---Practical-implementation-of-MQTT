#pragma once // include guard

// ============================================================
// КОНФІГУРАЦІЯ ПІНІВ
// ============================================================

#define DHT_PIN       4 // DHT22 сенсор підключений до GPIO4
#define BUTTON_PIN    14 // Кнопка підключена до GPIO14
#define WIFI_DISCONNECT_PIN   32 // Тестова кнопка розриву Wi-Fi підключена до GPIO32
#define DHT_TYPE      DHT22 // Задаємо тип сенсора DHT22

// ═══════════════════════════════════════════════════════════
// КОНФІГУРАЦІЯ WI-FI
// ═══════════════════════════════════════════════════════════

#define WIFI_SSID     "Wokwi-GUEST"  // мережа Wokwi симулятора
#define WIFI_PASSWORD ""             // без пароля
#define WIFI_TIMEOUT  10000          // Час очікування підключення до WI-FI

#define WIFI_RETRY_DELAY 5000 // Інтервал між спробами відновлення Wi-Fi, мс

// ═══════════════════════════════════════════════════════════
// КОНФІГУРАЦІЯ MQTT
// ═══════════════════════════════════════════════════════════

#define MQTT_BROKER   "broker.hivemq.com"    // Адреса брокера
#define MQTT_PORT     1883                   // Порт брокера
#define MQTT_CLIENT_ID "ESP32-Tregubenko-A"  // Ідентифікатор клієнта

#define MQTT_RETRY_DELAY  5000 // Інтервал між спробами reconnect, мс
#define MQTT_MAX_RETRIES  3    // Максимальна кількість спроб reconnect
#define MQTT_RETRY_CYCLE_DELAY 60000  // Пауза після 3 невдалих спроб, мс

#define MQTT_KEEPALIVE_SEC      60 // Інтервал keep-alive в секундах
#define MQTT_SOCKET_TIMEOUT_SEC 30 // Таймаут сокета в секундах

// ============================================================
// MQTT ТОПІКИ
// ============================================================

#define MQTT_TOPIC_TEMPERATURE "iot-course/Tregubenko/sensors/temperature"
#define MQTT_TOPIC_HUMIDITY    "iot-course/Tregubenko/sensors/humidity"

#define MQTT_TOPIC_COMMANDS    "iot-course/Tregubenko/commands"
#define MQTT_TOPIC_STATUS      "iot-course/Tregubenko/status"

// ============================================================
// ІНТЕРВАЛИ
// ============================================================

#define SENSOR_INTERVAL 10000 // Публікація даних сенсорів кожні 10 секунд
#define DEBOUNCE_DELAY  50    // Debounce кнопки, мс