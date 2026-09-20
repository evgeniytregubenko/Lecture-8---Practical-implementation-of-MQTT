#include <Arduino.h>
#include <DHT.h>

#include "config.h"
#include "sensors.h"

// ============================================================
// DHT22
// ============================================================

DHT dht(DHT_PIN, DHT_TYPE); // Створення об'єкта dht для роботи з сенсором DHT22, використовуючи визначений пін та тип сенсора

// ============================================================
// ІНІЦІАЛІЗАЦІЯ
// ============================================================

void initSensors() {

    dht.begin(); // Ініціалізація сенсора DHT22

}

bool readDHT(DHTData &data) {

    float humidity = dht.readHumidity(); // Зчитування вологості з сенсора DHT22
    float temperature = dht.readTemperature(); // Зчитування температури з сенсора DHT22

    if (isnan(temperature) || isnan(humidity)) { // Перевірка на наявність помилок при зчитуванні даних
        return false;
    }

    data.temperature = temperature; // Збереження зчитаної температури у структуру даних
    data.humidity = humidity; // Збереження зчитаної вологості у структуру даних

    return true;
}
