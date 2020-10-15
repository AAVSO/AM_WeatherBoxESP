/*
  HGAdler
  version 3, 2020-09-09
  MLX90614 (IR temp sensor) and AsyncMQTT with ESP8266
  added SHT20 (temp/rHum sensor) and RG-11 (rain sensor)
*/

// Import required libraries
#ifdef ESP32
#include <WiFi.h>
#include <ESPAsyncWebServer.h>
#else
#include <Arduino.h>
#include <ESP8266WiFi.h>
#include <Hash.h>
#include "ESPAsyncTCP.h"
#endif

#include <Adafruit_MLX90614.h>
Adafruit_MLX90614 mlx = Adafruit_MLX90614();

#include "DFRobot_SHT20.h"
DFRobot_SHT20    sht20;

#include <Ticker.h>  // this is only available for esp8266?
#include <AsyncMqttClient.h>

// WiFi credentials for local WiFi network
#define WIFI_SSID "JY4W5"
#define WIFI_PASSWORD "MilchFlasche"

// Raspberri Pi Mosquitto MQTT Broker
#define MQTT_HOST IPAddress (10, 0, 0, 199) //(192, 168, 1, XXX)
// For a cloud MQTT broker, type the domain name
//#define MQTT_HOST "example.com"
#define MQTT_PORT 1883

// need to define MQTT Topics
#define MQTT_PUB_OBJTEMP "esp/mlx90614/objTempC"
#define MQTT_PUB_AMBTEMP "esp/mlx90614/ambTempC"
#define MQTT_PUB_RSSI "esp/mlx90614/rssi"
#define MQTT_PUB_AIRTEMP "esp/sht20/airTempC"
#define MQTT_PUB_AIRHUM "esp/sht20/airHum"
#define MQTT_PUB_RAIN "esp/rg-11/Rainfall"


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
#define Bucket_Size 0.01 // in mm, set via switches in R-11

volatile unsigned long tipCount;     // bucket tip counter that is used in interrupt routine
volatile unsigned long ContactTime;  // Timer to manage any contact bounce in interrupt routine

long lastCount;
float Rainfall;
//

AsyncMqttClient mqttClient;
Ticker mqttReconnectTimer;

WiFiEventHandler wifiConnectHandler;
WiFiEventHandler wifiDisconnectHandler;
Ticker wifiReconnectTimer;

unsigned long previousMillis = 0;   // Stores the last time when temperature was published
const long interval = 10000;        // Interval at which to publish sensor readings in ms

// Interrrupt handler routine that is triggered when the RG-11 detects rain
// ICACHE_RAM_ATTR is needed for ESP, located before any function definition
ICACHE_RAM_ATTR void rgisr ()   {
  if ((millis() - ContactTime) > 15 ) {  // debounce of sensor signal
    tipCount++;
    ContactTime = millis();
  }
}
// end of rg-11 rain detection interrupt handler

void connectToWifi() {
  Serial.println("Connecting to Wi-Fi...");
  WiFi.begin(WIFI_SSID, WIFI_PASSWORD);
}

void onWifiConnect(const WiFiEventStationModeGotIP& event) {
  Serial.println("Connected to Wi-Fi.");
  connectToMqtt();
}

void onWifiDisconnect(const WiFiEventStationModeDisconnected& event) {
  Serial.println("Disconnected from Wi-Fi.");
  mqttReconnectTimer.detach(); // ensure we don't reconnect to MQTT while reconnecting to Wi-Fi
  wifiReconnectTimer.once(2, connectToWifi);
}

void connectToMqtt() {
  Serial.println("Connecting to MQTT...");
  mqttClient.connect();
}

void onMqttConnect(bool sessionPresent) {
  Serial.println("Connected to MQTT.");
  Serial.print("Session present: ");
  Serial.println(sessionPresent);
}

void onMqttDisconnect(AsyncMqttClientDisconnectReason reason) {
  Serial.println("Disconnected from MQTT.");

  if (WiFi.isConnected()) {
    mqttReconnectTimer.once(2, connectToMqtt);
  }
}

/*void onMqttSubscribe(uint16_t packetId, uint8_t qos) {
  Serial.println("Subscribe acknowledged.");
  Serial.print("  packetId: ");
  Serial.println(packetId);
  Serial.print("  qos: ");
  Serial.println(qos);
  }

  void onMqttUnsubscribe(uint16_t packetId) {
  Serial.println("Unsubscribe acknowledged.");
  Serial.print("  packetId: ");
  Serial.println(packetId);
  }*/

void onMqttPublish(uint16_t packetId) {
  Serial.print("Publish acknowledged.");
  Serial.print("  packetId: ");
  Serial.println(packetId);
}

void setup() {
  Serial.begin(115200);
  Serial.println();

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

  wifiConnectHandler = WiFi.onStationModeGotIP(onWifiConnect);
  wifiDisconnectHandler = WiFi.onStationModeDisconnected(onWifiDisconnect);

  mqttClient.onConnect(onMqttConnect);
  mqttClient.onDisconnect(onMqttDisconnect);
  //mqttClient.onSubscribe(onMqttSubscribe);
  //mqttClient.onUnsubscribe(onMqttUnsubscribe);
  mqttClient.onPublish(onMqttPublish);
  mqttClient.setServer(MQTT_HOST, MQTT_PORT);
  // If your broker requires authentication (username and password), set them below
  //mqttClient.setCredentials("REPlACE_WITH_YOUR_USER", "REPLACE_WITH_YOUR_PASSWORD");

  connectToWifi();

  Serial.println("Hydreon RG-11 Rain Sensor"); 
  Serial.print("Bucket Size: "); Serial.print(Bucket_Size); Serial.println(" mm");

  pinMode(RG11_Pin, INPUT);   // set the digital input pin to input for the RG-11 Sensor
  attachInterrupt(digitalPinToInterrupt(RG11_Pin), rgisr, RISING);// FALLING);     // attach interrupt handler to input pin.
  // we trigger the interrupt on the voltage rising from gnd to 3.3V for ESP8366 //falling from 5V to GND

  sei();         //Enables interrupts ( == interrupts() = set interrupt )
} // end setup

void loop() {
  // we only display the tip count when it has been incremented by the sensor
  cli();         //Disable interrupts

  if (tipCount != lastCount) {
    lastCount = tipCount;
    Rainfall = tipCount * Bucket_Size;
    Serial.print("Tip Count: "); Serial.print(tipCount);
    Serial.print("\t Rainfall: "); Serial.println(Rainfall);
  }
  sei();         //Enables interrupts


  unsigned long currentMillis = millis();
  // Every X number of seconds (interval = 10 seconds)
  // it publishes a new MQTT message
  if (currentMillis - previousMillis >= interval) {
    // Save the last time a new reading was published
    previousMillis = currentMillis;

    // New MLX90614 sensor readings
    atC = mlx.readAmbientTempC();
    otC = mlx.readObjectTempC();
    rssi = WiFi.RSSI();

    // New SHT20 sensor readings
    airtempC = sht20.readTemperature();              // Read AirTemperature
    airhum = sht20.readHumidity();                  // Read AirHumidity

    // Publish an MQTT message on topic esp/mlx90614/objTempC
    uint16_t packetIdPub1 = mqttClient.publish(MQTT_PUB_OBJTEMP, 1, true, String(otC).c_str());
    Serial.printf("Publishing on topic %s at QoS 1, packetId: %i ", MQTT_PUB_OBJTEMP, packetIdPub1);
    Serial.printf("Message: %.2f \n", otC);

    // Publish an MQTT message on topic esp/mlx90614/ambTempC
    uint16_t packetIdPub2 = mqttClient.publish(MQTT_PUB_AMBTEMP, 1, true, String(atC).c_str());
    Serial.printf("Publishing on topic %s at QoS 1, packetId: %i ", MQTT_PUB_AMBTEMP, packetIdPub2);
    Serial.printf("Message: %.2f \n", atC);

    // Publish an MQTT message on topic esp/mlx90614/rssi
    uint16_t packetIdPub3 = mqttClient.publish(MQTT_PUB_RSSI, 1, true, String(rssi).c_str());
    Serial.printf("Publishing on topic %s at QoS 1, packetId: %i ", MQTT_PUB_RSSI, packetIdPub3);
    Serial.printf("Message: %.3f \n", rssi);

    // Publish an MQTT message on topic esp/sht20/airTempC
    uint16_t packetIdPub4 = mqttClient.publish(MQTT_PUB_AIRTEMP, 1, true, String(airtempC).c_str());
    Serial.printf("Publishing on topic %s at QoS 1, packetId: %i ", MQTT_PUB_AIRTEMP, packetIdPub4);
    Serial.printf("Message: %.2f \n", airtempC);

    // Publish an MQTT message on topic esp/sht20/airhum
    uint16_t packetIdPub5 = mqttClient.publish(MQTT_PUB_AIRHUM, 1, true, String(airhum).c_str());
    Serial.printf("Publishing on topic %s at QoS 1, packetId: %i ", MQTT_PUB_AIRHUM, packetIdPub5);
    Serial.printf("Message: %.2f \n", airhum);

    // Publish an MQTT message on topic esp/rg-11/Rainfall
    uint16_t packetIdPub6 = mqttClient.publish(MQTT_PUB_RAIN, 1, true, String(Rainfall).c_str());
    Serial.printf("Publishing on topic %s at QoS 1, packetId: %i ", MQTT_PUB_RAIN, packetIdPub6);
    Serial.printf("Message: %.2f \n", Rainfall);
    Rainfall = 0;  // set to zero after publishing

    //    Serial.print("Time:");
    //    Serial.print(millis());
    //    Serial.print(" Temperature:");
    //    Serial.print(airtempC, 1);
    //    Serial.print("C");
    //    Serial.print(" Humidity:");
    //    Serial.print(airhum, 1);
    //    Serial.print("%");
    //    Serial.print(" Rainfall:");
    //    Serial.print(Rainfall, 1);
    //    Serial.print("mm");   
    //    Serial.println();

  }
}
