#include <Arduino.h>

#include "config.h"
#include "button.h"

// ============================================================
// ЗМІННІ СТАНУ КНОПКИ
// ============================================================

// static зберігає значення змінної між викликами функції.
// Це необхідно для debounce, оскільки функція isButtonPressed()
// викликається багато разів у loop(), але повинна пам'ятати
// попередній стан кнопки та час останньої зміни.

static bool lastButtonState = HIGH; // Попереднє зчитане значення входу
static bool buttonState = HIGH;     // Підтверджений стан кнопки після debounce

static unsigned long lastDebounceTime = 0; // Час останньої зміни стану входу

// ============================================================
// ІНІЦІАЛІЗАЦІЯ КНОПКИ
// ============================================================

void initButton() {

    // Кнопка підключена між GPIO та GND,
    // тому використовуємо внутрішній підтягуючий резистор
    pinMode(BUTTON_PIN, INPUT_PULLUP);
}

// ============================================================
// ОБРОБКА НАТИСКАННЯ КНОПКИ
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