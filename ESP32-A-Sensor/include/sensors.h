#pragma once // include guard

#include "data.h" // Підключаємо заголовочний файл з визначенням структури SensorData

// Ініціалізація сенсорів
void initSensors(); 

// Читання DHT22
bool readDHT(DHTData &data); 