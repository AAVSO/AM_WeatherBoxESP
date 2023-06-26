// code related to the sensors on the WBox2

#ifndef _WBOX2_SENSORS_H_
#define _WBOX2_SENSORS_H_



#include <Adafruit_MLX90614.h>
Adafruit_MLX90614 mlx = Adafruit_MLX90614();

//#include "DFRobot_SHT30.h"
//DFRobot_SHT20    sht20;
#include <Wire.h>
#include "Adafruit_SHT31.h"
Adafruit_SHT31 sht30 = Adafruit_SHT31();   //  Create sht30, an Adafruit_SHT31 object

// RG-11
#define RG11_Pin 14  // gpio 14, was originally 2 for Arduino
//#define Bucket_Size 0.01 // in mm, set via switches in RG-11

/*
// Interrrupt handler routine that is triggered when the RG-11 detects rain
// ICACHE_RAM_ATTR is needed for ESP, located before any function definition
ICACHE_RAM_ATTR 
void rgisr ()   {
  if ((millis() - ContactTime) > 15 ) {    //15 ) {  // debounce of sensor signal
    tipCount++;
    ContactTime = millis();
  }
}
// end of rg-11 rain detection interrupt handler
*/

// data averaging management 
// The average age of the data in the simple-exponential-smoothing forecast is 1/α relative to the period for which the forecast is computed.
float sampleRate= 0.01; // hrs  

unsigned long currentMillis = 0;
unsigned long previousMillis = 0;   // Stores the last time when temperature was published
unsigned long interval;  // Interval at which to publish sensor readings in ms

long setInterval() {
   interval= sampleRate * 3600000.0; // hrs to ms
}
// global vars for readings
//float atC;
float skytempC;
float airtempC;
float airhum;
float dewpoint;
bool  RainSense= true; // ie, no rain

void sensors_setup() {

   setInterval();
     
   // Initialize MLX sensor
   if (!mlx.begin()) {
      Serial.println("Could not find a valid MLX90614 sensor, check wiring!");
      while (1);
   }
   skytempC = mlx.readObjectTempC();

   /*
   // Initialize SHT20 sensor
   sht20.initSHT20();                                  // Init SHT20 Sensor
   delay(100);
   sht20.checkSHT20();                                 // Check SHT20 Sensor
   airtempC = sht20.readTemperature();   // Read AirTemperature
   airhum = sht20.readHumidity();       // Read AirHumidity
   */
   sht30.begin(0x44);
   delay(100);
   airtempC = sht30.readTemperature();
   airhum = sht30.readHumidity();
   
   dewpoint = airtempC -((100.0-airhum)/5.0); 


   Serial.println("Hydreon RG-11 Rain Sensor"); 
   #define RG11_pin 13
   pinMode(RG11_pin, INPUT_PULLUP);
   
   //Serial.print("Bucket Size: "); Serial.print(Bucket_Size); Serial.println(" mm");

   //pinMode(RG11_Pin, INPUT);   // set the digital input pin to input for the RG-11 Sensor
   //attachInterrupt(digitalPinToInterrupt(RG11_Pin), rgisr, RISING);// FALLING);     // attach interrupt handler to input pin.
   // we trigger the interrupt on the voltage rising from gnd to 3.3V for ESP8366 //falling from 5V to GND

}

extern float AveragePeriod; // alpaca.h

float smooth(float oldd, float newd) {
  return oldd + (newd - oldd) / (AveragePeriod / sampleRate); // alpha = 1 / AveragePeriod from alpaca
}

void sensor_update() {
    currentMillis= millis();
    if (currentMillis - previousMillis >= interval) {

      // New MLX90614 sensor readings
      //atC = mlx.readAmbientTempC();  too affected by box temp?
      skytempC = smooth(skytempC, mlx.readObjectTempC());
  
      // New SHT20 sensor readings
      airtempC = smooth(airtempC, sht30.readTemperature());   // Read AirTemperature
      airhum = smooth(airhum, sht30.readHumidity());       // Read AirHumidity
      dewpoint = airtempC -((100.0-airhum)/5.0); 
      RainSense= 1 == digitalRead(RG11_pin);
   
      Serial.printf("skytemp  %.1f ", skytempC); //, atC);
      Serial.printf("airtemp  %.1f ", airtempC);
      Serial.printf("humidity %.1f ",  airhum);
      Serial.printf("dewpoint %.1f ", dewpoint);
      Serial.printf("rain %s\n", RainSense?"no": "yes");  
      
      previousMillis+= interval;
    }   
}











#endif  _WBOX2_SENSORS_H_
