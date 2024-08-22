/*//////////////////////////////////
////////////////////////////////////
////////////////////////////////////
	Air Quality Monitoring System
	Code
	4/3/2023
	
				Andreas Josef Diaz
				Marc Jefferson Obeles
				Justin Gabriel Sy
////////////////////////////////////
////////////////////////////////////
//////////////////////////////////*/

/*//////////////////////////////////
		Libraries and Header Files 
//////////////////////////////////*/

#include "secret_info.h"
#include <WiFi.h>
#include <HTTPClient.h>
#include <ThingSpeak.h>
#include <Adafruit_Sensor.h>
#include <DHT.h>
#include <DHT_U.h>
#include "Adafruit_SGP30.h"
#include <MQUnifiedsensor.h>
#include "PMS.h"
#include "time.h"

/*//////////////////////////////////
						Definitions
//////////////////////////////////*/

#define DHTTYPE DHT22
#define DHTPIN 32
#define DHT11PIN 23

#define placa "ESP-32"
#define Voltage_Resolution 3.3
#define MQ135_pin 34
#define MQ135_type "MQ-135"
#define ADC_Bit_Resolution 12
/* Parameters to model temperature and humidity dependence */
#define CORA 0.00035
#define CORB 0.02718
#define CORC 1.39538
#define CORD 0.0018

#define MQ7_pin 39
#define MQ7_type "MQ-7"

#define ldr_pin 35

#define red 19
#define green 18
#define blue 5
#define RED_CHANNEL 0
#define GREEN_CHANNEL 1
#define BLUE_CHANNEL 2
#define LEDC_TIMER_12_BIT 12
#define LEDC_BASE_FREQ 5000

#if defined(BOX_1)
String Room_Name = "F-323%20A";
int LDR_Threshold = 3686;
#elif defined(BOX_2)
String Room_Name = "F-323%20B";
int LDR_Threshold = 3686;
#elif defined(BOX_3)
String Room_Name = "F-324";
int LDR_Threshold = 2790;
#endif

/*//////////////////////////////////
				Class Instantiations
//////////////////////////////////*/

WiFiClient client;
DHT_Unified dht(DHTPIN, DHTTYPE);
#if defined(BOX_3)
DHT_Unified dht11(DHT11PIN, DHT11);
#endif
Adafruit_SGP30 sgp;
MQUnifiedsensor MQ135(placa, Voltage_Resolution, ADC_Bit_Resolution, MQ135_pin, MQ135_type);
MQUnifiedsensor MQ7(placa, Voltage_Resolution, ADC_Bit_Resolution, MQ7_pin, MQ7_type);
PMS pms(Serial2);
PMS::DATA data;


/*//////////////////////////////////
				Constant Variables
//////////////////////////////////*/

const char* ssid = SECRET_SSID;
const char* password = SECRET_PASS;

// Calibration values
#if defined(BOX_1)
double MQ135_R0 = 1.893319104;
double MQ7_R0 = 0.1675133404;
uint16_t TVOC_R0 = 0x95C5;
uint16_t CO2_R0 = 0x96AC;
#elif defined(BOX_2)
double MQ135_R0 = 22.03808893;
double MQ7_R0 = 1.054153264;
uint16_t TVOC_R0 = 0x97F1;
uint16_t CO2_R0 = 0x9A68;
#elif defined(BOX_3)
double MQ135_R0 = 47.47781824;
double MQ7_R0 = 12.41954391;
uint16_t TVOC_R0 = 0x990C;
uint16_t CO2_R0 = 0x9D87;
#endif

/*//////////////////////////////////
				Changing Variables
//////////////////////////////////*/

String Time_Data = "";
int light = 0;
int diff_time = 0;
int poll_time = 40;
int eiaqi_store_count = 0;
float temperature = 0;
float humidity = 0;
#if defined(BOX_3)
float temperature11 = 0;
float humidity11 = 0;
#endif
float carbon_dioxide = 0;
float MQ135_CO = 0;
float MQ7_CO = 0;
float pm2_5 = 0;
float pm10 = 0;
float eiaqi_median = 0;
float eiaqi[10] = { 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 };
time_t base_time;
time_t current_time;

/*//////////////////////////////////
				Calibration Equations
//////////////////////////////////*/
float Corrected_Temp(float temperature) {
  float new_temp = 0;
#if defined(BOX_1)
  new_temp = 1.16 * temperature - 2.51;
#elif defined(BOX_2)
  new_temp = 1.13 * temperature - 1.6;
#elif defined(BOX_3)
  new_temp = 1.02 * temperature - 0.159;
#endif
  return (new_temp < 0) ? 0 : new_temp;
}

float Corrected_Humidity(float humidity) {
  float new_humid = 0;
#if defined(BOX_1)
  new_humid = 0.707 * humidity + 3.33;
#elif defined(BOX_2)
  new_humid = 0.689 * humidity + 8.6;
#elif defined(BOX_3)
  new_humid = 0.754 * humidity + 10.7;
#endif
  return (new_humid < 0) ? 0 : new_humid;
}

float Corrected_PM25(float pm2_5) {
  float new_pm2_5 = 0;
#if defined(BOX_1)
  new_pm2_5 = 0.5 * pm2_5 + 0.405;
#elif defined(BOX_2)
  new_pm2_5 = 0.479 * pm2_5 - 1.07;
#elif defined(BOX_3)
  new_pm2_5 = 0.507 * pm2_5 - 0.627;
#endif
  return (new_pm2_5 < 0) ? 0 : new_pm2_5;
}

float Corrected_PM10(float pm10) {
  float new_pm10 = 0;
#if defined(BOX_1)
  new_pm10 = 0.471 * pm10 + 0.894;
#elif defined(BOX_2)
  new_pm10 = 0.45 * pm10 - 0.761;
#elif defined(BOX_3)
  new_pm10 = 0.463 * pm10 - 0.539;
#endif
  return (new_pm10 < 0) ? 0 : new_pm10;
}

/*//////////////////////////////////
				Correction Equations
//////////////////////////////////*/
uint32_t getAbsoluteHumidity(float temperature, float humidity) {
  // approximation formula from Sensirion SGP30 Driver Integration chapter 3.15
  const float absoluteHumidity = 216.7f * ((humidity / 100.0f) * 6.112f * exp((17.62f * temperature) / (243.12f + temperature)) / (273.15f + temperature));  // [g/m^3]
  const uint32_t absoluteHumidityScaled = static_cast<uint32_t>(1000.0f * absoluteHumidity);                                                                 // [mg/m^3]
  return absoluteHumidityScaled;
}

float getCorrectionFactor(float t, float h) {
  return CORA * t * t - CORB * t + CORC - (h - 33.) * CORD;
}

/*//////////////////////////////////
						Functions
//////////////////////////////////*/
void ledcAnalogWrite(uint8_t channel, uint32_t value, uint32_t valueMax = 255) {
  // calculate duty, 4095 from 2 ^ 12 - 1
  uint32_t duty = (4095 / valueMax) * min(value, valueMax);

  // write duty to LEDC
  ledcWrite(channel, duty);
}

void RGBWrite(int Red_PWM, int Green_PWM, int Blue_PWM) {
  ledcAnalogWrite(RED_CHANNEL, Red_PWM);
  ledcAnalogWrite(GREEN_CHANNEL, Green_PWM);
  ledcAnalogWrite(BLUE_CHANNEL, Blue_PWM);
}

void RGBSetup() {
#ifdef DEBUG
  Serial.println("RGB Setup");
#endif
  ledcSetup(RED_CHANNEL, LEDC_BASE_FREQ, LEDC_TIMER_12_BIT);
  ledcAttachPin(red, RED_CHANNEL);
  ledcSetup(GREEN_CHANNEL, LEDC_BASE_FREQ, LEDC_TIMER_12_BIT);
  ledcAttachPin(green, GREEN_CHANNEL);
  ledcSetup(BLUE_CHANNEL, LEDC_BASE_FREQ, LEDC_TIMER_12_BIT);
  ledcAttachPin(blue, BLUE_CHANNEL);
}

void ErrorRGB(int ErrorCode) {
  for (int i = 0; i < 4; i++) {
    RGBWrite(255, 255, 255);
    delay(500);
    RGBWrite(0, 0, 0);
    delay(500);
  }
  RGBWrite(0, 0, 0);
}

void SGPSetup() {
#ifdef DEBUG
  Serial.println("SGP Setup");
#endif
  if (!sgp.begin()) {
#ifdef DEBUG
    Serial.println("SGP30 Sensor not found :(");
#endif
    while (1) {
      ErrorRGB(1);
    }
  }
  sgp.setIAQBaseline(CO2_R0, TVOC_R0);
}

void MQ135Setup() {
#ifdef DEBUG
  Serial.println("MQ135 Setup");
#endif
  MQ135.setRegressionMethod(1);
  MQ135.setA(605.18);
  MQ135.setB(-3.937);  // CO Concentration
  MQ135.init();
  MQ135.setRL(1);
#ifdef DEBUG
  Serial.println("MQ135_R0: " + String(MQ135_R0));
#endif
  MQ135.setR0(MQ135_R0);
}

void MQ7Setup() {
#ifdef DEBUG
  Serial.println("MQ7 Setup");
#endif
  MQ7.setRegressionMethod(1);
  MQ7.setA(99.042);
  MQ7.setB(-1.518);  // Calculate CO Concentration Value
  MQ7.init();
  MQ7.setRL(1);
#ifdef DEBUG
  Serial.println("MQ7_R0: " + String(MQ7_R0));
#endif
  MQ7.setR0(MQ7_R0);
}

void WiFiConnect() {
  if (WiFi.status() != WL_CONNECTED) {
#ifdef DEBUG
    Serial.print("Attempting to connect to SSID: ");
    Serial.println(SECRET_SSID);
#endif
    while (WiFi.status() != WL_CONNECTED) {
      RGBWrite(0, 0, 0);
      WiFi.begin(ssid, password);
#ifdef DEBUG
      Serial.print(".");
#endif
      delay(5000);
    }
#ifdef DEBUG
    Serial.println("\nConnected.");
#endif
    RGBWrite(0, 255, 0);
  }
}

String TimeData() {
#ifdef DEBUG
  Serial.println("Obtaining Time Data...");
#endif
  char timeStringBuff[50];
  struct tm timeinfo;
  if (!getLocalTime(&timeinfo)) {
#ifdef DEBUG
    Serial.println("Failed to obtain time");
#endif
    ErrorRGB(2);
  }
  strftime(timeStringBuff, sizeof(timeStringBuff), "%F %X", &timeinfo);
  String asString(timeStringBuff);
  asString.replace(" ", "%20");
  return asString;
}

void DHTData(float& Temperature, float& Humidity) {
#ifdef DEBUG
  Serial.println("Obtaining DHT Data...");
#endif
  sensors_event_t event;
  dht.temperature().getEvent(&event);
  if (isnan(event.temperature)) {
    ErrorRGB(3);
  } else Temperature = Corrected_Temp(event.temperature);
  dht.humidity().getEvent(&event);
  if (isnan(event.relative_humidity)) {
    ErrorRGB(4);
  } else Humidity = Corrected_Humidity(event.relative_humidity);
}
#if defined(BOX_3)
  float Corrected_Temp11(float temperature) {
    float new_temp = 0;
    new_temp = 1.06 * temperature - 0.75;
    return (new_temp < 0) ? 0 : new_temp;
  }

  float Corrected_Humidity11(float humidity) {
    float new_humid = 0;
    new_humid = 0.65 * humidity + 14;
    return (new_humid < 0) ? 0 : new_humid;
  }

  void DHT11Data(float& Temperature, float& Humidity) {
  #ifdef DEBUG
    Serial.println("Obtaining DHT11 Data...");
  #endif
    sensors_event_t event;
    dht11.temperature().getEvent(&event);
    if (isnan(event.temperature)) {
      ErrorRGB(3);
    } else Temperature = Corrected_Temp11(event.temperature);
    dht11.humidity().getEvent(&event);
    if (isnan(event.relative_humidity)) {
      ErrorRGB(4);
    } else Humidity = Corrected_Humidity11(event.relative_humidity);
  }
#endif

float SGP30Data(float Temperature, float Humidity) {
#ifdef DEBUG
  Serial.println("Obtaining SGP30 Data...");
#endif
  sgp.setHumidity(getAbsoluteHumidity(Temperature, Humidity));
  if (sgp.IAQmeasure()) {
    return sgp.eCO2;
  }
}

float MQ135Data(float Temperature, float Humidity) {
#ifdef DEBUG
  Serial.println("Obtaining MQ135 Data...");
#endif
  MQ135.update();
  return MQ135.readSensor(false, getCorrectionFactor(Temperature, Humidity));
}

float MQ7Data() {
#ifdef DEBUG
  Serial.println("Obtaining MQ7 Data...");
#endif
  MQ7.update();
  return MQ7.readSensor();
}

void PMSData(float& PM2_5, float& PM10) {
#ifdef DEBUG
  Serial.println("Obtaining PMS Data...");
#endif
  if (pms.readUntil(data)) {
    PM2_5 = Corrected_PM25(data.PM_AE_UG_2_5);
    PM10 = Corrected_PM10(data.PM_AE_UG_10_0);
  }
}

void SendThingSpeak(int Light, float Temperature,
                    float Humidity, float CO2, float MQ135_CO,
                    float MQ7_CO, float PM2_5, float PM10) {
#ifdef DEBUG
  Serial.println("Sending to ThingSpeak");
#endif
  ThingSpeak.setField(1, Light);
  ThingSpeak.setField(2, Temperature);
  ThingSpeak.setField(3, Humidity);
  ThingSpeak.setField(4, CO2);
  ThingSpeak.setField(5, MQ135_CO);
  ThingSpeak.setField(6, MQ7_CO);
  ThingSpeak.setField(7, PM2_5);
  ThingSpeak.setField(8, PM10);
  ThingSpeak.writeFields(ChannelNumber, WriteAPIKey);
}

void EIAQIPush(float new_eiaqi, int arr_len = 10) {
  for (int i = arr_len - 1; i > 0; i--) {
    eiaqi[i] = eiaqi[i - 1];
  }
  eiaqi[0] = new_eiaqi;
}

void sortArray(float myArray[], int numElements) {
  for (int i = 0; i < numElements; i++) {
    for (int j = i + 1; j < numElements; j++) {
      if (myArray[i] > myArray[j]) {
        float temp = myArray[i];
        myArray[i] = myArray[j];
        myArray[j] = temp;
      }
    }
  }
}

float EIAQIMedian(float eiaqi_array[10]) {
  float temp[10];
  for (int i = 0; i < 10; i++) {
    temp[i] = eiaqi_array[i];
  }
  int arraySize = sizeof(temp) / sizeof(temp[0]);
  sortArray(temp, arraySize);
  return (temp[4] + temp[5]) / 2;
}

void EIAQIAdjustRGB(float EIAQI) {
  // Hazardous
  if (EIAQI >= 0 && EIAQI < 16.68) {
    RGBWrite(255, 0, 0);
  }
  // Bad
  else if (EIAQI >= 16.68 && EIAQI < 50) {
    RGBWrite(255, 127, 0);
  }
  // Good
  else if (EIAQI >= 50 && EIAQI < 83.35) {
    RGBWrite(255, 255, 0);
  }
  // Excellent
  else {
    RGBWrite(0, 255, 0);
  }
}


#if defined(BOX_3)
  void SendGSheets11(String Time_Data, float Temperature1, float Temperature2,
                   float Humidity1, float Humidity2) {
  #ifdef DEBUG
    Serial.println("Sending to GSheets");
  #endif
    String urlFinal = "https://script.google.com/macros/s/AKfycbwlFXLXZJW3GpSuei_cI49eMJ2ShR0zRgTaxcharJ6Dg1-UlS9TdGkCDnQkirniaqs/exec?";
    urlFinal += "pass=" + PASSWORD;
    urlFinal += "&a=" + Time_Data;
    urlFinal += "&b=" + String(Temperature1);
    urlFinal += "&d=" + String(Temperature2);
    urlFinal += "&e=" + String(Humidity1);
    urlFinal += "&f=" + String(Humidity2);

    HTTPClient http11;
  #ifdef DEBUG
    Serial.println("URL: " + urlFinal);
  #endif
    http11.begin(urlFinal);
    http11.setFollowRedirects(HTTPC_STRICT_FOLLOW_REDIRECTS);
    int httpCode = http11.GET();
  #ifdef DEBUG
    Serial.println("HTTP Status Code: " + String(httpCode));
  #endif
    //getting response from google sheet
    String payload;
    if (httpCode > 0) {
      payload = http11.getString();
  #ifdef DEBUG
      Serial.println("HTTP Status Code: " + payload);
  #endif
    } else {
      ErrorRGB(6);
    }
    http11.end();
  }
#endif

void SendGSheets(String Time_Data, int Light, float Temperature,
                 float Humidity, float CO2, float MQ135_CO,
                 float MQ7_CO, float PM2_5, float PM10) {
#ifdef DEBUG
  Serial.println("Sending to GSheets");
#endif
  String urlFinal = "https://script.google.com/macros/s/" + GOOGLE_SCRIPT_ID + "/exec?";
  urlFinal += "pass=" + PASSWORD;
  urlFinal += "&a=" + Time_Data;
  urlFinal += "&b=" + Room_Name;
  urlFinal += "&d=" + String(Light);
  urlFinal += "&e=" + String(Temperature);
  urlFinal += "&f=" + String(Humidity);  // skip c, it is not recommended as it is used by the system
  urlFinal += "&g=" + String(CO2);
  urlFinal += "&h=" + String(MQ135_CO);
  urlFinal += "&i=" + String(MQ7_CO);
  urlFinal += "&j=" + String(PM2_5);
  urlFinal += "&k=" + String(PM10);

  HTTPClient http;
#ifdef DEBUG
  Serial.println("URL: " + urlFinal);
#endif
  http.begin(urlFinal.c_str());
  http.setFollowRedirects(HTTPC_STRICT_FOLLOW_REDIRECTS);
  int httpCode = http.GET();
#ifdef DEBUG
  Serial.println("HTTP Status Code: " + String(httpCode));
#endif
  //getting response from google sheet
  String payload;
  if (httpCode > 0) {
    payload = http.getString();
    EIAQIPush(payload.toFloat());
    eiaqi_median = EIAQIMedian(eiaqi);
#ifdef DEBUG
    Serial.println("HTTP Status Code: " + payload);
#endif
  } else {
    ErrorRGB(5);
  }
  http.end();
}

void EIAQIAdjustPollTime(float EIAQI) {
  if (light > LDR_Threshold) {
    poll_time = 60;
  } else {
    if (EIAQI >= 0 && EIAQI < 16.68) {
      poll_time = 10;
    }
    // Bad
    else if (EIAQI >= 16.68 && EIAQI < 50) {
      poll_time = 20;
    }
    // Good
    else if (EIAQI >= 50 && EIAQI < 83.35) {
      poll_time = 30;
    }
    // Excellent
    else {
      poll_time = 40;
    }
  }
}

#ifdef DEBUG
void ShowValues() {
  Serial.print("Time: ");
  Serial.println(Time_Data);
  Serial.print("Room: ");
  Serial.println(Room_Name);
  Serial.print("Light: ");
  Serial.println(light);
  Serial.print("Humidity: ");
  Serial.print(humidity);
  Serial.print(" | ");
  Serial.print("Temp: ");
  Serial.print(temperature);
  Serial.print(" | ");
  Serial.print("CO2: ");
  Serial.print(carbon_dioxide);
  Serial.print(" | ");
  Serial.print("MQ135 CO: ");
  Serial.print(MQ135_CO);
  Serial.print(" | ");
  Serial.print("MQ7 CO: ");
  Serial.print(MQ7_CO);
  Serial.print(" | ");
  Serial.print("PM2.5 : ");
  Serial.print(pm2_5);
  Serial.print(" | ");
  Serial.print("PM10 : ");
  Serial.print(pm10);
  Serial.print(" | ");
  Serial.print("EIAQI Median: ");
  Serial.println(eiaqi_median);
  Serial.println();
}
#endif

void setup() {
#ifdef DEBUG
  Serial.begin(9600);
#endif
  Serial2.begin(9600);
  RGBSetup();
  WiFi.mode(WIFI_STA);
  ThingSpeak.begin(client);
  dht.begin();
  #if defined(BOX_3)
    dht11.begin();
  #endif
  SGPSetup();
  MQ135Setup();
  MQ7Setup();
  WiFiConnect();
  configTime(gmtOffset_sec, daylightOffset_sec, ntpServer);
  delay(20000);
  base_time = time(nullptr);
}

void loop() {
  WiFiConnect();
  current_time = time(nullptr);
  diff_time = difftime(current_time, base_time);
#ifdef DEBUG
  Serial.println("Time Difference: " + String(diff_time));
#endif
  light = analogRead(ldr_pin);
  if (diff_time >= poll_time) {
    base_time = time(nullptr);
    Time_Data = TimeData();
    DHTData(temperature, humidity);
    #if defined(BOX_3)
      DHT11Data(temperature11, humidity11);
    #endif
    carbon_dioxide = SGP30Data(temperature, humidity);
    MQ135_CO = MQ135Data(temperature, humidity);
    MQ7_CO = MQ7Data();
    PMSData(pm2_5, pm10);
    light = analogRead(ldr_pin);
    SendThingSpeak(light, temperature,
                   humidity, carbon_dioxide,
                   MQ135_CO, MQ7_CO, pm2_5, pm10);
    #if defined(BOX_3)
      SendGSheets11(Time_Data, temperature, temperature11,
                   humidity, humidity11);
    #endif
    SendGSheets(Time_Data, light, temperature,
                humidity, carbon_dioxide,
                MQ135_CO, MQ7_CO, pm2_5, pm10);
    
#ifdef DEBUG
    ShowValues();
#endif
  }
  EIAQIAdjustRGB(eiaqi_median);
  EIAQIAdjustPollTime(eiaqi_median);
#ifdef DEBUG
  //delay(1000);
#endif
}