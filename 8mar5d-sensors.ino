#include <TroykaMQ.h> // Библиотека для работы с датчиками MQ (Troyka-модуль)
#include <TroykaDHT.h> // библиотека для работы с датчиками серии DHT

// Имя для пина, к которому подключен датчик
#define PIN_MQ2 A0
#define PIN_DHT11 4

// Описания комманд
const char *serialCommandInfo = "/info"; // Получение информации о контроллере
const char *serialCommandSensorDHT11 = "/sensor/DHT-11"; // Получение метрик сенсора DHT-11
const char *serialCommandSensorMQ2 = "/sensor/MQ-2"; // Получение метрик сенсора MQ-2

// Описания ошибок
const char *errorMessageUnknowCommand = "unknow command"; // Не известная команда

// Коды ошибок
const unsigned int *errorCodeUnknowCommand = 0x01; // Не известная команда
const unsigned int *errorCodeDHT11Error = 0x10; // Ошибка сенсора DHT-11


// создаём объект для работы с датчиком и передаём ему номер пина
MQ2 mq2(PIN_MQ2);

// создаём объект класса DHT
// передаём номер пина к которому подключён датчик и тип датчика
// типы сенсоров: DHT11, DHT21, DHT22
DHT dht(PIN_DHT11, DHT11);

String inputString = ""; // a String to hold incoming data
bool stringComplete = false; // whether the string is complete


void setup()
{
  // открываем последовательный порт
  Serial.begin(9600); // 115200
  
  // перед калибровкой датчика прогрейте его 60 секунд
  // выполняем калибровку датчика на чистом воздухе
  mq2.calibrate();

  // reserve 200 bytes for the inputString:
  inputString.reserve(200);
}

void loop()
{
  // print the string when a newline arrives:
  if (stringComplete) {
    if (inputString.equals(serialCommandInfo)) {
      printInfo();
    } else if (inputString.equals(serialCommandSensorDHT11)) {
      printSensorDHT11();
    } else if (inputString.equals(serialCommandSensorMQ2)) {
      printSensorMQ2();
    } else {
      printError(errorCodeUnknowCommand, errorMessageUnknowCommand);
    }

    // clear the string:
    inputString = "";
    stringComplete = false;
  }
}

//  SerialEvent occurs whenever a new data comes in the hardware serial RX. This
//  routine is run between each time loop() runs, so using delay inside loop can
//  delay response. Multiple bytes of data may be available.
void serialEvent()
{
  while (Serial.available()) {
    // get the new byte:
    char inChar = (char)Serial.read();

    // if the incoming character is a newline, set a flag so the main loop can
    // do something about it:
    if (inChar == '\n') {
      stringComplete = true;
      continue;
    }

    // add it to the inputString:
    inputString += inChar;
  }
}

// Выводит информацию о контроллере.
void printInfo () {
  Serial.println("{\"status\":\"ok\",\"data\":{\"type\":\"AVR\",\"controller\":\"Arduino Nano 3.0\",\"types\":[\"sensors\"],\"version\":1,\"sensors\":[{\"type\":\"DHT-11\",\"version\":1},{\"type\":\"MQ-2\",\"version\":1}]}}");
}

// Выводит поисание ошибки.
void printError (unsigned int code, char *msg)
{
  char buffer[256];
  sprintf(
    buffer,
    "{\"status\":\"error\",\"code\":%u,\"message\":\"%s\"}",
    code, msg
  );
  
  Serial.println(buffer);
}

// Выводит данные сенсора DHT-11 (температура и влажность).
void printSensorDHT11 ()
{
  // считывание данных с датчика
  dht.read();

  // проверяем состояние данных
  switch(dht.getState()) {
    case DHT_OK: // всё OK
      // выводим показания влажности и температуры
      char strtTemperatureC[6];
      dtostrf(dht.getTemperatureC(), 4, 2, strtTemperatureC);

      char strtHumidity[6];
      dtostrf(dht.getHumidity(), 4, 2, strtHumidity);

      char buffer[256];
      sprintf(
        buffer,
        "{\"status\":\"ok\",\"data\":{\"temperature\":{\"value\":%s,\"measure\":\"C\"},\"humidity\":{\"value\":%s,\"measure\":\"%%\"}}}",
        strtTemperatureC, strtHumidity
      );
        
      Serial.println(buffer);

      break;
    case DHT_ERROR_CHECKSUM: // ошибка контрольной суммы
      printError(errorCodeDHT11Error, "checksum error");
      break;
    case DHT_ERROR_TIMEOUT:     // превышение времени ожидания
      printError(errorCodeDHT11Error, "time out error");
      break;
    case DHT_ERROR_NO_REPLY:     // данных нет, датчик не реагирует или отсутствует
      printError(errorCodeDHT11Error, "sensor not connected");
      break;
  }
}

// Выводит данные сенсора MQ-2 (газы).
void printSensorMQ2 ()
{
  // 4 is mininum width, 2 is precision; float value is copied onto strRatio
  char strRatio[6];
  dtostrf(mq2.readRatio(), 4, 2, strRatio);
    
  char buffer[256];
  sprintf(
    buffer,
    "{\"status\":\"ok\",\"data\":{\"ratio\":{\"value\":%s,\"measure\":null},\"lpg\":{\"value\":%u,\"measure\":\"ppm\"},\"methane\":{\"value\":%u,\"measure\":\"ppm\"},\"smoke\":{\"value\":%u,\"measure\":\"ppm\"},\"hydrogen\":{\"value\":%u,\"measure\":\"ppm\"}}}",
    strRatio, mq2.readLPG(), mq2.readMethane(), mq2.readSmoke(), mq2.readHydrogen()
  );

  Serial.println(buffer);
}
