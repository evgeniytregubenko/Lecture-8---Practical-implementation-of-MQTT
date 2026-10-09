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

#define WIFI_TIMEOUT  10000          // Час очікування підключення до WI-FI
#define WIFI_RETRY_DELAY 5000 // Інтервал між спробами відновлення Wi-Fi, мс

// ============================================================
// КОНФІГУРАЦІЯ MQTT (AWS IoT Core)
// ============================================================

#define MQTT_PORT      8883

#define MQTT_RETRY_DELAY  5000 // Інтервал між спробами reconnect, мс
#define MQTT_MAX_RETRIES  3    // Максимальна кількість спроб reconnect
#define MQTT_RETRY_CYCLE_DELAY 60000  // Пауза після 3 невдалих спроб, мс

#define MQTT_KEEPALIVE_SEC      60 // Інтервал keep-alive в секундах
#define MQTT_SOCKET_TIMEOUT_SEC 30 // Таймаут сокета в секундах

// ============================================================
// MQTT ТОПІКИ
// ============================================================

#define MQTT_TOPIC_DATA     "iot-course/Tregubenko/sensors/data"
#define MQTT_TOPIC_COMMANDS "iot-course/Tregubenko/commands"
#define MQTT_TOPIC_STATUS   "iot-course/Tregubenko/status"

// ============================================================
// ІНТЕРВАЛИ
// ============================================================

#define SENSOR_INTERVAL 30000 // Публікація даних сенсорів кожні 30 секунд
#define DEBOUNCE_DELAY  50    // Debounce кнопки, мс

// ============================================================
// КОНФІГУРАЦІЯ NTP
// ============================================================

#define NTP_SERVER          "pool.ntp.org"
#define NTP_GMT_OFFSET_SEC  0
#define NTP_DAYLIGHT_OFFSET 0
#define NTP_TIMEOUT         10000 // Таймаут синхронізації часу, мс

#define NTP_RETRY_DELAY 10000 // Інтервал між повторними спробами синхронізації, мс