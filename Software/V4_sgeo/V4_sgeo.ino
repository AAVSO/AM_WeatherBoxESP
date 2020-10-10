/*
  Board:   NodeMCU 0.9 (ESP-12 Module)
  Start with MLX90614_SHT20_RG-11_PangolinMQTT_ESP8266_hga4
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

#define DRIVER_VERSION  "0.4.1"
// Import required libraries
#ifdef ESP32
   #include <WiFi.h>
   #include <ESPAsyncWebServer.h>
#else
   #include <Arduino.h>
   #include <ESP8266WiFi.h>
   #include <Hash.h>
   //#include <ESPAsyncTCP.h>
   #include <ESP8266WebServer.h>  //https://github.com/esp8266/Arduino/tree/master/libraries/ESP8266WebServer
   ESP8266WebServer server(80);
#endif

// name at the router ESP-1E2B43-router.home, unless we do mDNS

#include <WiFiManager.h>
const String DEVICE_SSID = "AutoConnectAP";

#include <Ticker.h>  // this is only available for esp8266?. Used by several
  
#include "sensors.h" // everything related to the weather sensors
//#include "mqtt.h"   // everything related to MQTT
#include "alpaca.h"  



// WiFi credentials for local WiFi network
String WIFI_SSID; 
String WIFI_PASSWORD;



//WiFiEventHandler wifiConnectHandler;
//WiFiEventHandler wifiDisconnectHandler;
//Ticker wifiReconnectTimer;


void connectToWifi() {
  WiFi.begin(WIFI_SSID, WIFI_PASSWORD);
  Serial.print("Connecting to Wi-Fi");
  // Wait for connection
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(WiFi.status()); //".");
  }
  Serial.println("");
  Serial.print("Connected to ");
  Serial.println(WIFI_SSID);
  Serial.print("IP address: ");
  Serial.println(WiFi.localIP());
}
/*
   WL_NO_SHIELD        = 255,   // for compatibility with WiFi Shield library
    WL_IDLE_STATUS      = 0,
    WL_NO_SSID_AVAIL    = 1,
    WL_SCAN_COMPLETED   = 2,
    WL_CONNECTED        = 3,
    WL_CONNECT_FAILED   = 4,
    WL_CONNECTION_LOST  = 5,
    WL_DISCONNECTED     = 6 
*/
/*
void onWifiConnect(const WiFiEventStationModeGotIP& event) {
  Serial.println("Connected to Wi-Fi.");
  #ifdef _WBOX2_MQTT_H_
  connectToMqtt();
  #endif
}

void onWifiDisconnect(const WiFiEventStationModeDisconnected& event) {
  Serial.println("Disconnected from Wi-Fi.");
  #ifdef _WBOX2_MQTT_H_
  mqttReconnectTimer.detach(); // ensure we don't reconnect to MQTT while reconnecting to Wi-Fi
  #endif
  wifiReconnectTimer.once(2, connectToWifi);
}
*/



// put your setup code here, to run once:
void setup() { // ================================================
  Serial.begin(115200);

  // WiFiManger
  WiFi.mode(WIFI_STA); // explicitly set mode, esp defaults to STA+AP
  WiFiManager wm; //WiFiManager, Local intialization. Once its business is done, there is no need to keep it around
  //wm.resetSettings();  //reset settings - wipe credentials for testing

  // Automatically connect using saved credentials,
  // if connection fails, it starts an access point with the specified name ( "AutoConnectAP"),
  // if empty will auto generate SSID, if password is blank it will be anonymous AP (wm.autoConnect())
  // then goes into a blocking loop awaiting configuration and will return success result
  bool res;
  // res = wm.autoConnect(); // auto generated AP name from chipid
  // res = wm.autoConnect(DEVICE_SSID); // anonymous ap
  res = wm.autoConnect(DEVICE_SSID.c_str(),"password"); // password protected ap
  if(!res) {
    Serial.println("Failed to connect");
    // ESP.restart();
  } else {
    //if you get here you have connected to the WiFi    
    Serial.println("connected...yeey :)");
    WIFI_SSID= wm.getWiFiSSID();
    WIFI_PASSWORD= wm.getWiFiPass();
  }
  
  connectToWifi();

  #ifdef _WBOX2_MQTT_H_
  mqtt_setup();
  #endif

  sensors_setup();

/*
#ifdef ARDUINO_ARCH_ESP8266
  wifiConnectHandler = WiFi.onStationModeGotIP(onWifiConnect);
  wifiDisconnectHandler = WiFi.onStationModeDisconnected(onWifiDisconnect);
#else
  WiFi.onEvent(WiFiEvent);
#endif
*/  

  server.on("/", handleRoot);
  server.onNotFound(handleNotFound);
  server.on("/reset", handleReset);

  server.on("/setup", handleSetup);

  alpaca_setup();
  
  // server
  server.begin(); 
  Serial.println("HTTP server started");
  Serial.println(WiFi.localIP());

  sei();         //Enables interrupts ( == interrupts() = set interrupt )
  
} // end setup   ===================================================

void handleRoot() {
  const char* ss= "Use /reset to reset the wifi credentials.";
  Serial.println(ss);
  server.send(200, "text/plain", ss);
}

void handleNotFound() {
  String message = "File Not Found\n\n";
  message += "URI: ";
  message += server.uri();
  message += "\nMethod: ";
  message += (server.method() == HTTP_GET) ? "GET" : "POST";
  message += "\nArguments: ";
  message += server.args();
  message += "\n";
  for (uint8_t i = 0; i < server.args(); i++) {
    message += " " + server.argName(i) + ": " + server.arg(i) + "\n";
  }
  server.send(404, "text/plain", message);
}

void handleReset() {
  String ss= "Look for me at SSID: " + DEVICE_SSID + " on IP: 192.168.4.1";
  Serial.println(ss);
  server.send(200, "text/plain", ss.c_str() );
  delay(10000); // 10 sec
  WiFiManager wm;
  wm.resetSettings();
  ESP.restart();
}

void handleSetup() {
  server.send(200, "text/plain", "Setup page for setting Alpaca discovery port and MQTT interface");
}

unsigned long previousMillis = 0;   // Stores the last time when temperature was published
const long interval = 30000;        // Interval at which to publish sensor readings in ms, change to larger values!!!!
void loop() {
  server.handleClient();
  
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

    sensor_update();

    #ifdef _WBOX2_MQTT_H_
    mqtt_report();
    #endif
    
    Serial.printf("Message: %.2f \n", otC);
    Serial.printf("Message: %.2f \n", atC);
    Serial.printf("Message: %.3f \n", rssi);
    Serial.printf("Message: %.2f \n", airtempC);
    Serial.printf("Message: %.2f \n", airhum);
    Serial.printf("Message: %.2f \n", Rainfall);
    
    ///Rainfall = 0;  // set to zero after publishing
    tipCount = 0;
  }
}
