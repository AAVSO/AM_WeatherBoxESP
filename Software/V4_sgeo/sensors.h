// code related to the sensors on the WBox2

#ifndef _WBOX2_SENSORS_H_
#define _WBOX2_SENSORS_H_



#include <Adafruit_MLX90614.h>
Adafruit_MLX90614 mlx = Adafruit_MLX90614();

#include "DFRobot_SHT20.h"
DFRobot_SHT20    sht20;

// Variables to hold sensor readings
// mlx90614
float otC;
float atC;
float rssi;

// sht20
float airtempC;
float airhum;

// RG-11
#define RG11_Pin 14  // gpio 14, was originally 2 for Arduino
#define Bucket_Size 0.01 // in mm, set via switches in RG-11

volatile unsigned long tipCount;     // bucket tip counter that is used in interrupt routine
volatile unsigned long ContactTime;  // Timer to manage any contact bounce in interrupt routine

long lastCount;
float Rainfall;


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



void sensors_setup() {
  
   lastCount = 0;
   tipCount = 0;
   Rainfall = 0;

   // Initialize SHT20 sensor
   sht20.initSHT20();                                  // Init SHT20 Sensor
   delay(100);
   sht20.checkSHT20();                                 // Check SHT20 Sensor

   // Initialize MLX sensor
   if (!mlx.begin()) {
      Serial.println("Could not find a valid MLX90614 sensor, check wiring!");
      while (1);
   }

   Serial.println("Hydreon RG-11 Rain Sensor"); 
   Serial.print("Bucket Size: "); Serial.print(Bucket_Size); Serial.println(" mm");

   pinMode(RG11_Pin, INPUT);   // set the digital input pin to input for the RG-11 Sensor
   attachInterrupt(digitalPinToInterrupt(RG11_Pin), rgisr, RISING);// FALLING);     // attach interrupt handler to input pin.
   // we trigger the interrupt on the voltage rising from gnd to 3.3V for ESP8366 //falling from 5V to GND

}




void sensor_update() {
    // New MLX90614 sensor readings
    atC = mlx.readAmbientTempC();
    otC = mlx.readObjectTempC();
    rssi = WiFi.RSSI();

    // New SHT20 sensor readings
    airtempC = sht20.readTemperature();              // Read AirTemperature
    airhum = sht20.readHumidity();                  // Read AirHumidity



}











#endif  _WBOX2_SENSORS_H_