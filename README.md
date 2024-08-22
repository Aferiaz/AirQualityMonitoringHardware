# Air-Quality-Monitoring-Dashboard

## Tutorials
### Tutorial for ESP32
  - https://esp32io.com/esp32-tutorials
  - Start with ESP32 - Software Installation: https://esp32io.com/tutorials/esp32-software-installization
  - How to Connect to Wifi: https://www.youtube.com/watch?v=TnWDlHpY56o

### Tutorial for ESP8266
  - Pins - https://randomnerdtutorials.com/esp8266-pinout-reference-gpios/

### Tutorial for ThingSpeak
  - How to Connect to ESP32/8266 - https://nothans.com/measure-wi-fi-signal-levels-with-the-esp8266-and-thingspeak
  
### CCS811 Air Quality Sensor
  >Wiring - https://learn.adafruit.com/adafruit-ccs811-air-quality-sensor/arduino-wiring-test
  
### DHT 22 Sensor (Temperature and Humidity)
  - https://components101.com/sensors/dht22-pinout-specs-datasheet
  
  - Use DHT Sensor Library in Arduino IDE
  
### MQ-7 (CO Sensor)
  - VCC - 5V
  
  - GND - GND
  
  - AO - Analog Pins
  
  - Tutorial: https://www.hackster.io/ingo-lohs/gas-sensor-carbon-monoxide-mq-7-aka-flying-fish-e58457
  
  - Use analogWrite for ESP32 Library
  
### MQ-135 (Air Quality)
  - Pins - https://components101.com/sensors/mq135-gas-sensor-for-air-quality
  
  - Tutorial/Example - In MQ Sensor Library
 
### Dust Sensor
  - http://arduinolearning.com/code/arduino-dust-sensor-example.php
  
  - Remove LED parts
  
  - Not sure how to calibrate nor how to calculate PM
  
### LDR
  - Same Setup as MQ7, MQ135, and Dust Sensor
  

## TODO

1. Create Unified Sketch
   - ~~Connect to Network~~
   - ~~Connect to ThingSpeak~~
   - ~~Get CO2 Value~~
   - ~~Get CO Value~~
   - ~~Get Temperature~~
   - ~~Get Humidity~~
   - Get Air Quality
     - Get NOX  -  MQ135 has no NOX sensor
     - ~~Get NH3~~
   - Get PM2.5
   - Get PM10
2. Calibrate Sensors
   - eCO2 Sensor
      - ~~Connect DHT22 Humidity and Temperature Parameters~~
      - Compare with another calibrated sensor.(DOST? WhereintheworldisDOST?)
   - MQ135
      - Recheck regression model and numbers
      - Check if output is in ppm

