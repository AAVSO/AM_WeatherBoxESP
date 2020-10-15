/*
  HGAdler == AMH
  version 4, 2020-09-12
  MLX90614 (IR temp sensor) and AsyncMQTT with ESP8266
  added SHT20 (temp/rHum sensor) and RG-11 (rain sensor)
  replaced Async...MQTT library with PangolinMQTT

  external libraries need to be downloaded and manually installed as zip files via Arduino IDE library manager:
  https://github.com/me-no-dev/ESPAsyncTCP
  https://github.com/DFRobot/DFRobot_SHT20
  [https://github.com/marvinroger/async-mqtt-client] -> replaced by
  https://github.com/philbowles/PangolinMQTT

  all other libraries need to be installed directly via Arduino IDE library manager
  
*/

// Import required libraries
#ifdef ESP32
#include <WiFi.h>
#include <ESPAsyncWebServer.h>
#else
#include <Arduino.h>
#include <ESP8266WiFi.h>
#include <Hash.h>
#include <ESPAsyncTCP.h>
#endif

#include <Adafruit_MLX90614.h>
Adafruit_MLX90614 mlx = Adafruit_MLX90614();

#include "DFRobot_SHT20.h"
DFRobot_SHT20    sht20;

#include <Ticker.h>  // this is only available for esp8266?
#include <PangolinMQTT.h> // P new hga


// WiFi credentials for local WiFi network
#define WIFI_SSID ""
#define WIFI_PASSWORD ""

// Raspberri Pi Mosquitto MQTT Broker
#define MQTT_HOST IPAddress (192, 168, 1, 133) //e.g. (192, 168, 1, XXX)
// For a cloud MQTT broker, type the domain name
//#define MQTT_HOST "example.com"
#define MQTT_PORT 1883

#define RECONNECT_DELAY_W 5  // P new hga

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
#define Bucket_Size 0.01 // in mm, set via switches in RG-11

volatile unsigned long tipCount;     // bucket tip counter that is used in interrupt routine
volatile unsigned long ContactTime;  // Timer to manage any contact bounce in interrupt routine

long lastCount;
float Rainfall;
//

uint32_t elapsed=0;    // P new hga

//AsyncMqttClient mqttClient;
PangolinMQTT mqttClient;     // P new hga
Ticker mqttReconnectTimer;

WiFiEventHandler wifiConnectHandler;
WiFiEventHandler wifiDisconnectHandler;
Ticker wifiReconnectTimer;

unsigned long previousMillis = 0;   // Stores the last time when temperature was published
const long interval = 30000;        // Interval at which to publish sensor readings in ms, change to larger values!!!!

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
//   P new hga
std::string pload0 = "multi-line payload hex dumper which should split over several lines, with some left over";
std::string pload1 = "PAYLOAD QOS1";
std::string pload2 = "Save the Pangolin!";
//   ****

// P new hga
void onMqttConnect(bool sessionPresent) {
  elapsed=millis();
  Serial.printf("Connected to MQTT session=%d max payload size=%d\n",sessionPresent,mqttClient.getMaxPayloadSize());
  Serial.println("Subscribing at QoS 2");
  mqttClient.subscribe("test", 2);
  Serial.printf("T=%u Publishing at QoS 0\n",millis());
  mqttClient.publish("test", 0, false, pload0);
  Serial.printf("T=%u Publishing at QoS 1\n",millis());
  mqttClient.publish("test", 1, false, pload1);
  Serial.printf("T=%u Publishing at QoS 2\n",millis());
  mqttClient.publish("test", 2, false, pload2);
}
// ****

// P new hga
void onMqttDisconnect(int8_t reason) {
  Serial.printf("Disconnected from MQTT reason=%d\n",reason);

  if (WiFi.isConnected()) {
    mqttReconnectTimer.once(2, connectToMqtt);
  }
}
// ********

// P new hga
void onMqttMessage(const char* topic, uint8_t* payload, struct PANGO_PROPS props, size_t len, size_t index, size_t total) {
  Serial.printf("T=%u Message %s qos%d dup=%d retain=%d len=%d elapsed=%u\n",millis(),topic,props.qos,props.dup,props.retain,len,millis()-elapsed);
  PANGO::dumphex(payload,len,16);
}
// *****

void setup() {
  Serial.begin(115200);
  Serial.println("\nPangolinMQTT v0.0.7\n");

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

  // P new hga
#ifdef ARDUINO_ARCH_ESP8266
  wifiConnectHandler = WiFi.onStationModeGotIP(onWifiConnect);
  wifiDisconnectHandler = WiFi.onStationModeDisconnected(onWifiDisconnect);
#else
  WiFi.onEvent(WiFiEvent);
#endif
  // ***********
  
  mqttClient.onConnect(onMqttConnect);
  mqttClient.onDisconnect(onMqttDisconnect);

  mqttClient.onMessage(onMqttMessage);    // P new hga
  mqttClient.setServer(MQTT_HOST, MQTT_PORT);
  // If your broker requires authentication (username and password), set them below
  //mqttClient.setCredentials("REPlACE_WITH_YOUR_USER", "REPLACE_WITH_YOUR_PASSWORD");
  mqttClient.setCleanSession(true);       // P new hga
  connectToWifi();

  Serial.println("Hydreon RG-11 Rain Sensor"); 
  Serial.print("Bucket Size: "); Serial.print(Bucket_Size); Serial.println(" mm");

  pinMode(RG11_Pin, INPUT);   // set the digital input pin to input for the RG-11 Sensor
  attachInterrupt(digitalPinToInterrupt(RG11_Pin), rgisr, RISING);// FALLING);     // attach interrupt handler to input pin.
  // we trigger the interrupt on the voltage rising from gnd to 3.3V for ESP8366 //falling from 5V to GND

  //server.on("/", handleRoot);


  sei();         //Enables interrupts ( == interrupts() = set interrupt )
} // end setup

//void handleRoot() {
//  server.send(200, "text/plain", "hello from esp8266!");
//}

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
    mqttClient.publish(MQTT_PUB_OBJTEMP, 2, false, String(otC)); // P new hga
    Serial.printf("Message: %.2f \n", otC);

    // Publish an MQTT message on topic esp/mlx90614/ambTempC
    mqttClient.publish(MQTT_PUB_AMBTEMP, 2, false, String(atC)); // P new hga
    Serial.printf("Message: %.2f \n", atC);

    // Publish an MQTT message on topic esp/mlx90614/rssi
    mqttClient.publish(MQTT_PUB_RSSI, 2, false, String(rssi)); // P new hga
    Serial.printf("Message: %.3f \n", rssi);

    // Publish an MQTT message on topic esp/sht20/airTempC
    mqttClient.publish(MQTT_PUB_AIRTEMP, 2, false, String(airtempC)); // P new hga                                                                                                                                                                                                                               
    Serial.printf("Message: %.2f \n", airtempC);

    // Publish an MQTT message on topic esp/sht20/airhum
    mqttClient.publish(MQTT_PUB_AIRHUM, 2, false, String(airhum)); // P new hga
    Serial.printf("Message: %.2f \n", airhum);

    // Publish an MQTT message on topic esp/rg-11/Rainfall
    mqttClient.publish(MQTT_PUB_RAIN, 2, false, String(Rainfall)); // P new hga
    Serial.printf("Message: %.2f \n", Rainfall);
    ///Rainfall = 0;  // set to zero after publishing
    tipCount = 0;
  }
}
