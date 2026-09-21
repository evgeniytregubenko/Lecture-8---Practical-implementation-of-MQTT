#include <Arduino.h>

#include "config.h"
#include "actuator.h"

// ============================================================
// СТАН СВІТЛОДІОДА
// ============================================================

static bool ledState = false; // Нормальний стан LED, визначений температурою: false - OFF, true - ON

static bool blinkActive = false; // Ознака активного режиму мигання

static uint8_t blinkStep = 0; // Поточний крок мигання

static unsigned long lastBlinkTime = 0; // Час останнього перемикання LED

// ============================================================
// ІНІЦІАЛІЗАЦІЯ СВІТЛОДІОДА
// ============================================================

void initActuator() {

    pinMode(LED_PIN, OUTPUT); // Встановлення GPIO світлодіода як виходу

    digitalWrite(LED_PIN, LOW); // Початковий стан світлодіода - вимкнений

    ledState = false;
}

// ============================================================
// КЕРУВАННЯ СВІТЛОДІОДОМ ЗА ТЕМПЕРАТУРОЮ
// ============================================================

void updateLEDByTemperature(float temperature) {

    if ((temperature > TEMPERATURE_LED_ON) && !ledState) { // Температура вище 26 °C - вмикаємо LED

        ledState = true;

        if (!blinkActive) { // Під час manual blink фізичний стан LED не змінюємо

            digitalWrite(LED_PIN, HIGH);
        }

        Serial.println("LED ON");

        return;
    }

    if ((temperature < TEMPERATURE_LED_OFF) && ledState) { // Температура нижче 20 °C - вимикаємо LED

        ledState = false;

        if (!blinkActive) { // Під час manual blink фізичний стан LED не змінюємо

            digitalWrite(LED_PIN, LOW);
        }

        Serial.println("LED OFF");

        return;
    }

    // При температурі від 20 до 26 °C
    // поточний стан світлодіода не змінюється
}

// ============================================================
// ОТРИМАННЯ ПОТОЧНОГО СТАНУ СВІТЛОДІОДА
// ============================================================

bool getLEDState() { // Повертає поточний стан світлодіода, визначений температурою

    return ledState;
}

// ============================================================
// ЗАПУСК MANUAL BLINK
// ============================================================

void startManualBlink() { // Запуск триразового мигання LED

    blinkActive = true;

    blinkStep = 0;

    lastBlinkTime = millis();

    digitalWrite(LED_PIN, HIGH); // Перше вмикання виконуємо одразу
}

// ============================================================
// ОБСЛУГОВУВАННЯ MANUAL BLINK
// ============================================================

void actuatorLoop() {

    if (!blinkActive) {

        return;
    }

    unsigned long currentTime = millis();

    if ((currentTime - lastBlinkTime) < LED_BLINK_INTERVAL) { // Чекаємо наступного інтервалу без використання delay()

        return;
    }

    lastBlinkTime = currentTime; // Запам'ятовуємо час останнього перемикання LED

    blinkStep++; // Збільшуємо крок мигання

    // Непарний крок - вимикаємо LED
    // Парний крок - вмикаємо LED
    if ((blinkStep % 2) == 1) {

        digitalWrite(LED_PIN, LOW);

    } else {

        digitalWrite(LED_PIN, HIGH);
    }

    if (blinkStep >= (LED_BLINK_COUNT * 2 - 1)) { // 3 мигання = 6 перемикань ON/OFF

        blinkActive = false;

        digitalWrite( // Повертаємо LED у стан, визначений температурою
            LED_PIN,
            ledState ? HIGH : LOW
        );
    }
}