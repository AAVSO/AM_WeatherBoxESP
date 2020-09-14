/*
  HGAdler == AMH
  version 3, 2020-09-12
  - ESP8266 and BME280
  - MQTT (PangolinMQTT)

  external libraries that need to be downloaded and manually installed as zip files via Arduino IDE library manager:
  https://github.com/philbowles/PangolinMQTT

  all other libraries can be installed directly via Arduino IDE library manager

*/

#include <Wire.h>
#include <Adafruit_Sensor.h>
#include <Adafruit_BME280.h>
#include <ESP8266WiFi.h>
#include <Ticker.h>
#include <PangolinMQTT.h> // P new hga

#define WIFI_SSID "XXXXX"
#define WIFI_PASSWORD "YYYYY"

// Raspberri Pi Mosquitto MQTT Broker
#define MQTT_HOST IPAddress(aaa, bbb, c, ddd)   //e.g. IPAddress(192, 168, 1, XXX)
// For a cloud MQTT broker, type the domain name
//#define MQTT_HOST "example.com"
#define MQTT_PORT 1883
#define RECONNECT_DELAY_W 5  // P new hga
// Temperature MQTT Topics
#define MQTT_PUB_TEMP "esp/bme280/airTempC"
#define MQTT_PUB_HUM "esp/bme280/airHum"
#define MQTT_PUB_PRES "esp/bme280/airPress"

// BME280 I2C
Adafruit_BME280 bme;
// Variables to hold sensor readings
float temp;
float hum;
float pres;

uint32_t elapsed = 0;  // P new hga

PangolinMQTT mqttClient;     // P new hga
Ticker mqttReconnectTimer;

WiFiEventHandler wifiConnectHandler;
WiFiEventHandler wifiDisconnectHandler;
Ticker wifiReconnectTimer;

unsigned long previousMillis = 0;   // Stores last time temperature was published
const long interval = 20000;        // Interval at which to publish sensor readings

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
  elapsed = millis();
  Serial.printf("Connected to MQTT session=%d max payload size=%d\n", sessionPresent, mqttClient.getMaxPayloadSize());
  Serial.println("Subscribing at QoS 2");
  mqttClient.subscribe("test", 2);
  Serial.printf("T=%u Publishing at QoS 0\n", millis());
  mqttClient.publish("test", 0, false, pload0);
  Serial.printf("T=%u Publishing at QoS 1\n", millis());
  mqttClient.publish("test", 1, false, pload1);
  Serial.printf("T=%u Publishing at QoS 2\n", millis());
  mqttClient.publish("test", 2, false, pload2);
}
// ****

// P new hga
void onMqttDisconnect(int8_t reason) {
  Serial.printf("Disconnected from MQTT reason=%d\n", reason);

  if (WiFi.isConnected()) {
    mqttReconnectTimer.once(2, connectToMqtt);
  }
}
// ********

// P new hga
void onMqttMessage(const char* topic, uint8_t* payload, struct PANGO_PROPS props, size_t len, size_t index, size_t total) {
  Serial.printf("T=%u Message %s qos%d dup=%d retain=%d len=%d elapsed=%u\n", millis(), topic, props.qos, props.dup, props.retain, len, millis() - elapsed);
  PANGO::dumphex(payload, len, 16);
}
// *****

void setup() {
  Serial.begin(115200);
  Serial.println("\nPangolinMQTT v0.0.7\n");

  // Initialize BME280 sensor
  if (!bme.begin(0x76)) {
    Serial.println("Could not find a valid BME280 sensor, check wiring!");
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
}

void loop() {
  unsigned long currentMillis = millis();
  // Every X number of seconds (interval = 10 seconds)
  // it publishes a new MQTT message
  if (currentMillis - previousMillis >= interval) {
    // Save the last time a new reading was published
    previousMillis = currentMillis;
   
    // New BME280 sensor readings
    temp = bme.readTemperature();
    //temp = 1.8*bme.readTemperature() + 32; // to calculate temp in Fahrenheit
    hum = bme.readHumidity();
    pres = bme.readPressure() / 100.0F;

    // Publish an MQTT message on topic esp/bme280/temperature
    mqttClient.publish(MQTT_PUB_TEMP, 2, false, String(temp));
    Serial.printf("Message: %.2f \n", temp);

    // Publish an MQTT message on topic esp/bme280/humidity
    mqttClient.publish(MQTT_PUB_HUM, 2, false, String(hum));
    Serial.printf("Message: %.2f \n", hum);

    // Publish an MQTT message on topic esp/bme280/pressure
    mqttClient.publish(MQTT_PUB_PRES, 2, false, String(pres));
    Serial.printf("Message: %.3f \n", pres);
  }
}
