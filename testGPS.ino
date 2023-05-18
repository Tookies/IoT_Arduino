#include <GPRS_Shield_Arduino.h>                    // Библиотека для GSM/GPRS
#include <TinyGPS++.h>                              // Библиотека для GPS
#include <SoftwareSerial.h>                         // Software UART
#include <ArduinoJson.h>                            // Библиотека для JSON строк
#define INTERVAL 300000                             // Время через которое отправляются данные
#define LEN 110

String apn = "internet.beeline.ru";                    //APN
String apn_u = "beeline";                     //APN-Username
String apn_p = "beeline";                     //APN-Password
String url = "http://83.220.174.168/api/locations/ ";  //URL of Server

int PKPinGPRS = 8;                                  // Указание вывода подключения PK GPRS
int STPinGPRS = 9;                                  // Указание вывода подключения ST GPRS

int GPRSBaud = 9600 ;                                // Указание скорости передачи с GPRS Shield

GPRS gprs(Serial1, PKPinGPRS, STPinGPRS);        // Создание объекта GPRS

int RXPin = 10;                          // Указываем вывод подключения RX  
int TXPin = 6;                          // Указываем вывод подключения TX

int GPSBaud = 9600 ;                     // Указываем скорость передачи с NEO-6M

TinyGPSPlus gps;                        // Создание объекта 
SoftwareSerial gpsSerial(RXPin, TXPin); // Создайте последовательный связь под названием "gpsSerial"

// GPS Coordinates
double latitude_r = 0;
double longitude_r = 0;
double altitude_r = 0;

// Date
uint16_t year_r = 0;
uint8_t month_r = 0;
uint8_t day_r = 0;
uint8_t hour_r = 0;
uint8_t minute_r = 0;
uint8_t second_r = 0;
uint8_t centisecond_r = 0;
uint16_t now_time = 0;
uint16_t prev_time = 1;

const int capacity = JSON_OBJECT_SIZE(7);
StaticJsonDocument<capacity> doc;
char output[LEN];
char date_str[20];
long wtimer;

void setup()
{
  Serial.begin(9600 );                   // Запускаем последовательный порт с ПК на скорости 9600
  gpsSerial.begin(GPSBaud);             // Запустите последовательный порт с NEO-6M на скорости 9600
  Serial1.begin(GPRSBaud);                       // Запуск последовательного порта с GPRS Shield на скорости 9600
  doc["animal"].set(1);
  doc["location_lat"].set(0);
  doc["location_lon"].set(0);
  doc["time_sent"].set("0");
}

void loop()
{
  while (gpsSerial.available() > 0)
    if (gps.encode(gpsSerial.read()))
      displayInfo();

  // Если нет данных в течении 5000 миллисекунд, пишем сообщение об ошибки
  if (millis() > 5000 && gps.charsProcessed() < 10)
  {
    Serial.println("No GPS detected");
    while(true);
  }
}

void displayInfo()
{
  if (gps.location.isValid())
  {
    latitude_r = gps.location.lat();
    longitude_r = gps.location.lng();
    altitude_r = gps.altitude.meters();
    Serial.print("Latitude: ");
    Serial.println(latitude_r, 6);
    Serial.print("Longitude: ");
    Serial.println(longitude_r, 6);
    Serial.print("Altitude: ");
    Serial.println(altitude_r);

    Serial.print("Time: ");
    if (gps.time.isValid())
    {
      hour_r = gps.time.hour()+3;
      minute_r = gps.time.minute();
      second_r = gps.time.second();
      centisecond_r = gps.time.centisecond();
      if (hour_r < 10) Serial.print(F("0"));
      Serial.print(hour_r);
      Serial.print(":");
      if (minute_r < 10) Serial.print(F("0"));
      Serial.print(minute_r);
      Serial.print(":");
      if (second_r < 10) Serial.print(F("0"));
      Serial.print(second_r);
      Serial.print(".");
      if (centisecond_r < 10) Serial.print(F("0"));
      Serial.println(centisecond_r);

      Serial.print("Date: ");
      if (gps.date.isValid())
      {
        year_r = gps.date.year();
        month_r = gps.date.month();
        day_r = gps.date.day();
        Serial.print(month_r);
        Serial.print("/");
        Serial.print(day_r);
        Serial.print("/");
        Serial.println(year_r);
        if(now_time!=prev_time)
        {
          gprs.powerOn();
          while (!gprs.init())                              // Проверка есть ли связь с GPRS устройством
          {
            delay(1000);
          }
          httpRequest();
          prev_time=now_time;
        }
      }
      else
      {
        year_r = 0;
        month_r = 0;
        day_r = 0;
        Serial.println("Not Available");
      }
    }
    else
    {
      hour_r = 0;
      minute_r = 0;
      second_r = 0;
      centisecond_r = 0;
      Serial.println("Not Available");
    }
  }
  else
  {
    latitude_r = 0;
    longitude_r = 0;
    altitude_r = 0;
    Serial.println("Location: Not Available");
  }

  Serial.println();
  Serial.println();
  delay(500);
}

void httpRequest()
{
  doc["animal"].set(1);
  doc["location_lat"].set(latitude_r);
  doc["location_lon"].set(longitude_r);
  create_date_str();
  Serial.println(date_str);
  doc["time_sent"].set(date_str);
    
  serializeJson(doc, output);
  gsm_http_post(output);
  clear_output();
  clear_date_str();
  doc.clear();
}
void create_date_str()
{
  char * str_year = new char[4];
  itoa(year_r, str_year, 10);
  strcat(date_str, str_year);
  delete []str_year;
  strcat(date_str, "-");
  char * str_2 = new char[2];
  itoa(month_r, str_2, 10);
  strcat(date_str, str_2);
  strcat(date_str, "-");
  itoa(day_r, str_2, 10);
  strcat(date_str, str_2);
  strcat(date_str, "T");
  itoa(hour_r, str_2, 10);
  strcat(date_str, str_2);
  strcat(date_str, ":");
  itoa(minute_r, str_2, 10);
  strcat(date_str, str_2);
  strcat(date_str, ":");
  itoa(second_r, str_2, 10);
  strcat(date_str, str_2);
  delete []str_2;
}

void clear_output()
{
  for (int t = 0; t < LEN; t++) {
    // очищаем буфер,
    // присваивая всем индексам массива значение 0
    output[t] = 0;
  }
}

void clear_date_str()
{
  for (int t = 0; t < 20; t++) {
    // очищаем буфер,
    // присваивая всем индексам массива значение 0
    date_str[t] = 0;
  }
}

void gsm_config_gprs() {
  Serial.println(" --- CONFIG GPRS --- ");
  gsm_send_serial("AT+SAPBR=3,1,Contype,GPRS");
  gsm_send_serial("AT+SAPBR=3,1,APN," + apn);
  if (apn_u != "") {
    gsm_send_serial("AT+SAPBR=3,1,USER," + apn_u);
  }
  if (apn_p != "") {
    gsm_send_serial("AT+SAPBR=3,1,PWD," + apn_p);
  }
}

void gsm_http_post( String postdata) {
  Serial.println(" --- Start GPRS & HTTP --- ");
  gsm_send_serial("AT+SAPBR=1,1");
  gsm_send_serial("AT+SAPBR=2,1");
  gsm_config_gprs();
  delay(1500);
  gsm_send_serial("AT+HTTPINIT");
  gsm_send_serial("AT+HTTPPARA=CID,1");
  gsm_send_serial("AT+HTTPPARA=URL," + url);
  gsm_send_serial("AT+HTTPPARA=CONTENT,application/json");
  gsm_send_serial("AT+HTTPDATA=125,5000");
  gsm_send_serial(postdata);
  gsm_send_serial("AT+HTTPACTION=1");
  gsm_send_serial("AT+HTTPREAD");
  gsm_send_serial("AT+HTTPTERM");
  gsm_send_serial("AT+SAPBR=0,1");
  gprs.powerOff();
}

void gsm_send_serial(String command) {
  Serial.println("Send ->: " + command);
  Serial1.println(command);
  wtimer = millis();
  while (wtimer + 2500 > millis()) {
    while (Serial1.available()) {
      Serial.write(Serial1.read());
    }
  }
  Serial.println();
}
