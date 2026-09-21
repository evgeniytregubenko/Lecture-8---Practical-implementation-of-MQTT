# Перевірка роботи

## Взаємодія ESP32-A та ESP32-B

Обидва пристрої одночасно запущені у двох екземплярах Wokwi та обмінюються даними через MQTT-брокер.

![Взаємодія ESP32-A та ESP32-B](images/esp32-a-b-mqtt-test.png)

На ESP32-A видно періодичну публікацію даних DHT22 та відправлення команди `manual_read`.

На ESP32-B видно:

- отримання температури;
- керування LED;
- підписку на температуру з QoS 1;
- отримання команди `manual_read`;
- повторне підключення до MQTT.

---

## Перевірка MQTT reconnect

![MQTT reconnect](images/esp32-a-mqtt-reconnect.png)

Після втрати MQTT-з'єднання пристрій виконує повторну спробу підключення:

```text
MQTT reconnect attempt 1/3
MQTT reconnected
```

Після успішного reconnect передача MQTT-повідомлень продовжується.

---

## Перевірка гістерезису

![Перевірка гістерезису](images/esp32-b-hysteresis-test.png)

При температурі в діапазоні `20...26 °C` стан LED не змінюється.

Наприклад, при:

```text
Temperature received: 24.60 C
```

ESP32-B отримує нове значення температури, але не виконує `LED ON` або `LED OFF`.

---

## Історія MQTT-повідомлень

![Історія MQTT](images/mqtt-history.png)

MQTT-клієнт підписаний на:

```text
iot-course/Tregubenko/#
```

що дозволяє контролювати повідомлення всіх топіків проєкту.

---

## Публікація стану LED

![MQTT повідомлення та стан LED](images/mqtt-message-history.png)

При зміні стану LED ESP32-B публікує:

```text
iot-course/Tregubenko/actuators/led
```

зі значенням:

```text
ON
```

або:

```text
OFF
```

На скріншоті одночасно видно роботу ESP32-A, ESP32-B та історію MQTT-повідомлень.

---

## Перевірка MQTT Status та Last Will

![MQTT status online/offline](images/mqtt-status-online-offline.png)

ESP32-B публікує retained-повідомлення:

```text
iot-course/Tregubenko/status → online
```

після успішного MQTT-підключення.

Якщо ESP32-B аварійно втрачає MQTT-з'єднання, брокер відповідно до налаштованого Last Will автоматично публікує:

```text
iot-course/Tregubenko/status → offline
```

Таким чином перевірено повний цикл:

```text
ESP32-B підключений       → online
ESP32-B втратив зв'язок   → offline
ESP32-B підключився знову → online
```