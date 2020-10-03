// code for the WBox2 project related to MQTT

#ifndef _WBOX2_MQTT_H_
#define _WBOX2_MQTT_H_

#include <PangolinMQTT.h> 

// Raspberri Pi Mosquitto MQTT Broker
#define MQTT_HOST IPAddress (192, 168, 1, 133) //e.g. (192, 168, 1, XXX)
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



uint32_t elapsed=0;    // P new hga

//AsyncMqttClient mqttClient;
PangolinMQTT mqttClient;     // P new hga
Ticker mqttReconnectTimer;
#define RECONNECT_DELAY_W 5  // P new hga

void connectToMqtt() {
  Serial.println("Connecting to MQTT...");
  mqttClient.connect();
}

std::string pload0 = "multi-line payload hex dumper which should split over several lines, with some left over";
std::string pload1 = "PAYLOAD QOS1";
std::string pload2 = "Save the Pangolin!";


// event servers:
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

void onMqttDisconnect(int8_t reason) {
  Serial.printf("Disconnected from MQTT reason=%d\n",reason);

  if (WiFi.isConnected()) {
    mqttReconnectTimer.once(2, connectToMqtt);
  }
}

void onMqttMessage(const char* topic, uint8_t* payload, struct PANGO_PROPS props, size_t len, size_t index, size_t total) {
  Serial.printf("T=%u Message %s qos%d dup=%d retain=%d len=%d elapsed=%u\n",millis(),topic,props.qos,props.dup,props.retain,len,millis()-elapsed);
  PANGO::dumphex(payload,len,16);
}


void mqtt_setup() {

  Serial.println("\nPangolinMQTT v0.0.7\n");

  mqttClient.onConnect(onMqttConnect);
  mqttClient.onDisconnect(onMqttDisconnect);

  mqttClient.onMessage(onMqttMessage);    // P new hga
  mqttClient.setServer(MQTT_HOST, MQTT_PORT);
  // If your broker requires authentication (username and password), set them below
  //mqttClient.setCredentials("REPlACE_WITH_YOUR_USER", "REPLACE_WITH_YOUR_PASSWORD");
  mqttClient.setCleanSession(true);       // P new hga

}

void mqtt_report() {

    // Publish an MQTT message on topic esp/mlx90614/objTempC
    mqttClient.publish(MQTT_PUB_OBJTEMP, 2, false, String(otC)); // P new hga

    // Publish an MQTT message on topic esp/mlx90614/ambTempC
    mqttClient.publish(MQTT_PUB_AMBTEMP, 2, false, String(atC)); // P new hga

    // Publish an MQTT message on topic esp/mlx90614/rssi
    mqttClient.publish(MQTT_PUB_RSSI, 2, false, String(rssi)); // P new hga

    // Publish an MQTT message on topic esp/sht20/airTempC
    mqttClient.publish(MQTT_PUB_AIRTEMP, 2, false, String(airtempC)); // P new hga                                                                                                                                                                                                                               

    // Publish an MQTT message on topic esp/sht20/airhum
    mqttClient.publish(MQTT_PUB_AIRHUM, 2, false, String(airhum)); // P new hga

    // Publish an MQTT message on topic esp/rg-11/Rainfall
    mqttClient.publish(MQTT_PUB_RAIN, 2, false, String(Rainfall)); // P new hga

}

#endif  _WBOX2_MQTT_H_