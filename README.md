# ESP32-A Sensor — AWS IoT Core (Lecture 11)

Навчальний проєкт з передавання телеметрії ESP32 до AWS IoT Core через захищене MQTT/TLS-з'єднання, обробки повідомлень за допомогою AWS IoT Rules Engine, збереження даних у DynamoDB та реєстрації подій у CloudWatch Logs.

## 1. Архітектура

```text
DHT22 ──> ESP32-A (Wokwi / PlatformIO)
                │
                │ MQTT over TLS (порт 8883)
                ▼
          AWS IoT Core
                │
                ├── Lecture11_Rules_Engine
                │      ├── DynamoDBv2 → Lecture11_Telemetry_db
                │      └── Error Action → CloudWatch Logs
                │                          /iot-course/Tregubenko/rule-errors
                │
                └── Lecture11_Rule_Temperature_alert
                       └── temperature > 28 °C
                              └── CloudWatch Logs
                                  /iot-course/Tregubenko/temperature-alerts
```

**Середовище:** ESP32, датчик DHT22, дві кнопки (ручне зчитування та тестове відключення Wi-Fi), VS Code, PlatformIO, Wokwi. **AWS Region:** `eu-north-1` (Stockholm).

## 2. Структура проєкту

```text
ESP32-A-Sensor/
├── include/
│   ├── button.h
│   ├── config.h
│   ├── data.h
│   ├── mqtt_client.h
│   ├── secrets.example.h
│   ├── secrets.h              # локальні секрети, не додавати до Git
│   └── sensors.h
├── src/
│   ├── button.cpp
│   ├── main.cpp
│   ├── mqtt_client.cpp
│   └── sensors.cpp
├── certificate/               # локальні сертифікати й приватний ключ
├── images/                    # скриншоти перевірок
├── diagram.json
├── platformio.ini
└── wokwi.toml
```

Файли з обліковими даними (`include/secrets.h`, приватні ключі та сертифікати з `certificate/`) потрібно виключити з Git через `.gitignore`. Для налаштування використовується шаблон `include/secrets.example.h` без справжніх секретів.

## 3. Налаштування AWS IoT Core

1. Створено Thing: **`Lecture11_ESP32-A-Sensor`**.
2. Створено та активовано сертифікат пристрою, прив'язаний до Thing.
3. До сертифіката прив'язано IoT Policy **`Publisher`**, що обмежує підключення конкретним MQTT Client ID та публікацію конкретним топіком.
4. ESP32 використовує сертифікати для TLS-з'єднання з AWS IoT Core на порту `8883`.

**MQTT Client ID:** `Lecture11_ESP32-A-Sensor`  
**MQTT topic:** `iot-course/Tregubenko/sensors/data`

Приклад дозволів IoT Policy (з урахуванням конфігурації цього навчального проєкту):

```json
{
  "Version": "2012-10-17",
  "Statement": [
    {
      "Effect": "Allow",
      "Action": "iot:Connect",
      "Resource": "arn:aws:iot:eu-north-1:900670597961:client/Lecture11_ESP32-A-Sensor"
    },
    {
      "Effect": "Allow",
      "Action": "iot:Publish",
      "Resource": "arn:aws:iot:eu-north-1:900670597961:topic/iot-course/Tregubenko/sensors/data"
    }
  ]
}
```

![AWS IoT Thing](images/01_aws_thing.png)
![Сертифікат пристрою](images/02_aws_certificate.png)
![IoT Policy](images/03_aws_policy.png)

## 4. Телеметрія ESP32

Пристрій зчитує температуру та вологість з DHT22 кожні **30 секунд** і публікує JSON у MQTT-топік `iot-course/Tregubenko/sensors/data`.

Приклад повідомлення:

```json
{
  "device_id": "Lecture11_ESP32-A-Sensor",
  "timestamp": 1791619722,
  "temperature": 24.0,
  "humidity": 40.0
}
```

- `device_id` — ідентифікатор пристрою.
- `timestamp` — Unix-час пристрою в **секундах** після синхронізації NTP.
- `temperature` — температура, °C.
- `humidity` — відносна вологість, %.

Зелена кнопка запускає ручне зчитування; червона — тестове відключення Wi-Fi. Дані перевірено в Wokwi та AWS IoT MQTT Test Client.

![ESP32 у Wokwi та телеметрія](images/04_wokwi_telemetry.png)
![MQTT-повідомлення в AWS](images/05_aws_mqtt_messages.png)

## 5. Обробка помилок та відновлення роботи

Програма продовжує виконання після збоїв підключення або датчика:

| Ситуація | Поведінка програми | Перевірка |
|---|---|---|
| Втрата Wi-Fi | Повторне підключення через `WiFi.reconnect()` з інтервалом 5 с | Примусове відключення кнопкою; Wi-Fi і MQTT відновлено |
| Розрив MQTT | Перевірка стану Wi-Fi, закриття попереднього TLS-з'єднання, повторне MQTT-підключення | MQTT відновлено з першої спроби під час тесту |
| Повторні помилки MQTT | До 3 спроб, після чого пауза 60 с перед новим циклом | Логіка реалізована в `mqttLoop()` |
| Помилка NTP | MQTT не підключається до синхронізації часу; NTP повторюється з інтервалом `NTP_RETRY_DELAY` | Перевірено недоступний NTP та відновлення після двох імітованих помилок |
| Помилка DHT22 | Перевірка `isnan()`; некоректні дані не публікуються у періодичному циклі | Дві імітовані помилки, потім відновлення публікації |

Для повторних спроб використовується `millis()`; для зчитування DHT22 застосовується перевірка успішності читання. У програмі передбачено прапорці стану, зокрема `STATUS_WIFI_ERR`, `STATUS_MQTT_ERR`, `STATUS_DHT_ERR`.

**Примітка:** імітацію помилок NTP та DHT22 використовували лише під час тестування; після перевірки тимчасовий тестовий код видалено. Повідомлення TLS-бібліотеки після примусового розриву Wi-Fi не завадило успішному відновленню MQTT.

![Відновлення Wi-Fi та MQTT](images/06_wifi_mqtt_reconnect.png)
![Помилка синхронізації NTP](images/07_ntp_failure.png)
![Відновлення NTP](images/08_ntp_recovery.png)
![Відновлення читання DHT22](images/09_dht22_recovery.png)

## 6. AWS IoT Rules Engine → DynamoDB

**Правило:** `Lecture11_Rules_Engine` (Active).  
**Джерело:** `iot-course/Tregubenko/sensors/data`.  
**Action:** DynamoDBv2.  
**Таблиця:** `Lecture11_Telemetry_db`.

SQL:

```sql
SELECT *,
       timestamp() AS received_at,
       clientid() AS client_id,
       topic(2) AS student
FROM 'iot-course/Tregubenko/sensors/data'
```

Структура ключів DynamoDB:

| Ключ | Тип |
|---|---|
| `device_id` (Partition key) | String |
| `received_at` (Sort key) | Number |

Правило доповнює телеметрію полями:

- `received_at` — час приймання повідомлення AWS у **мілісекундах** Unix.
- `client_id` — MQTT Client ID.
- `student` — значення `Tregubenko` з другого рівня топіка.

**Виявлена та виправлена помилка:** початковий SQL використовував `cast(timestamp() AS String)`, але ключ `received_at` у DynamoDB має тип Number. Після заміни на `timestamp() AS received_at` записи почали надходити до таблиці. У DynamoDB підтверджено шість записів із температурою, вологістю та службовими полями.

![Записи телеметрії у DynamoDB](images/10_dynamodb_telemetry.png)

### Error Action: CloudWatch Logs

Для помилок виконання правила налаштовано окрему групу логів:

`/iot-course/Tregubenko/rule-errors`

Під час діагностики невідповідності типу `received_at` у CloudWatch були зафіксовані події правила `Lecture11_Rules_Engine`. Ці логи використовуються для **діагностики помилок обробки**, а не для температурних сповіщень.

![Потоки журналу помилок](images/11_cloudwatch_rule_error_streams.png)
![Події журналу помилок](images/12_cloudwatch_rule_error_events.png)

## 7. Температурне правило → CloudWatch Logs

**Правило:** `Lecture11_Rule_Temperature_alert` (Active).  
**Умова:** `temperature > 28`.  
**Action:** CloudWatch Logs (звичайна дія правила, не Error Action).  
**Log group:** `/iot-course/Tregubenko/temperature-alerts`.

SQL:

```sql
SELECT *,
       timestamp() AS received_at,
       clientid() AS client_id,
       topic(2) AS student
FROM 'iot-course/Tregubenko/sensors/data'
WHERE temperature > 28
```

Для тесту в Wokwi було встановлено температуру вище порога. У CloudWatch Logs зафіксовано повідомлення з **`temperature: 54.3` °C**, що підтверджує спрацювання правила. Повідомлення також містить `device_id`, `timestamp`, `humidity`, `received_at`, `client_id` та `student`.

![SQL та дія температурного правила](images/13_temperature_alert_rule.png)
![Потоки температурних подій](images/14_temperature_alert_streams.png)
![Подія перевищення температури](images/15_temperature_alert_events.png)

## 8. Запуск проєкту

1. Відкрити папку проєкту у VS Code з установленим PlatformIO та середовищем Wokwi.
2. Налаштувати `include/secrets.h` на основі `include/secrets.example.h` і додати локальні TLS-сертифікати та приватний ключ (не публікувати їх).
3. Перевірити параметри в `include/config.h`, зокрема Wi-Fi, AWS endpoint, MQTT topic, NTP та інтервал публікації.
4. Переконатися, що AWS Thing, сертифікат, Policy, обидва Rules, DynamoDB та групи CloudWatch Logs налаштовані.
5. Запустити симуляцію Wokwi. У терміналі перевірити підключення Wi-Fi, синхронізацію часу, MQTT та публікацію JSON.
6. Перевірити отримання повідомлень в AWS IoT MQTT Test Client і появу записів у DynamoDB.
7. Для перевірки температурного правила встановити температуру DHT22 вище 28 °C і переглянути CloudWatch Logs.

## 9. Результат

Під час тестування підтверджено:

- захищене підключення ESP32 до AWS IoT Core та публікацію телеметрії;
- відновлення Wi-Fi, MQTT та NTP після збоїв;
- обробку помилок DHT22 без публікації некоректних показників у періодичному циклі;
- запис телеметрії через AWS IoT Rules Engine у DynamoDB;
- надходження подій помилок правила до CloudWatch Logs;
- спрацювання температурного правила при значенні понад 28 °C.

Проєкт демонструє повний шлях IoT-даних від сенсора до хмарного зберігання та обробки подій.
