#include <Arduino.h>

#include "config.h"
#include "button.h"

// ============================================================
// ЗМІННІ КНОПКИ MANUAL READ
// ============================================================

// static зберігає значення змінної між викликами функції.
// Це необхідно для debounce, оскільки функція isButtonPressed()
// викликається багато разів у loop(), але повинна пам'ятати
// попередній стан кнопки та час останньої зміни.

static bool lastButtonState = HIGH; // Попереднє зчитане значення входу
static bool buttonState = HIGH;     // Підтверджений стан кнопки після debounce

static unsigned long lastDebounceTime = 0; // Час останньої зміни стану входу

// ============================================================
// ЗМІННІ КНОПКИ WI-FI RESET
// ============================================================

static bool lastWiFiDisconnectButtonState = HIGH;
static bool wifiDisconnectButtonState = HIGH;

static unsigned long lastWiFiDisconnectDebounceTime = 0;

// ============================================================
// ІНІЦІАЛІЗАЦІЯ КНОПОК
// ============================================================

void initButton() {

    // Кнопка підключена між GPIO та GND,
    // тому використовуємо внутрішній підтягуючий резистор
    pinMode(BUTTON_PIN, INPUT_PULLUP);
}

void initWiFiDisconnectButton() {

    // Кнопка підключена між GPIO та GND,
    // тому використовуємо внутрішній підтягуючий резистор
    pinMode(WIFI_DISCONNECT_PIN, INPUT_PULLUP);
}

// ============================================================
// ОБРОБКА НАТИСКАННЯ КНОПКИ MANUAL READ
// ============================================================

bool isButtonPressed() {

    bool reading = digitalRead(BUTTON_PIN); // Зчитування поточного стану кнопки (HIGH або LOW)

    if (reading != lastButtonState) { // Якщо стан кнопки змінився, оновлюємо час останньої зміни

        lastDebounceTime = millis();
    }

    if ((millis() - lastDebounceTime) > DEBOUNCE_DELAY) { // Якщо пройшло достатньо часу після останньої зміни

        if (reading != buttonState) { // Якщо стан кнопки змінився після debounce

            buttonState = reading;

            if (buttonState == LOW) { // Якщо кнопку натиснуто

                lastButtonState = reading;

                return true; // Сигналізуємо про нове натискання кнопки
            }
        }
    }

    lastButtonState = reading; // Запам'ятовуємо стан для наступного виклику

    return false; // Нового натискання немає
}

// ============================================================
// ОБРОБКА КНОПКИ WI-FI DISCONNECT
// ============================================================

bool isWiFiDisconnectButtonPressed() {

    bool reading = digitalRead(WIFI_DISCONNECT_PIN);

    if (reading != lastWiFiDisconnectButtonState) {

        lastWiFiDisconnectDebounceTime = millis();
    }

    if ((millis() - lastWiFiDisconnectDebounceTime) > DEBOUNCE_DELAY) {

        if (reading != wifiDisconnectButtonState) {

            wifiDisconnectButtonState = reading;

            if (wifiDisconnectButtonState == LOW) {

                lastWiFiDisconnectButtonState = reading;

                return true;
            }
        }
    }

    lastWiFiDisconnectButtonState = reading;

    return false;
}