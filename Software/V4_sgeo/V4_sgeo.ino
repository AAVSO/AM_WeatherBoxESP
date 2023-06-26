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
  https://github.com/philbowles/PangolinMQTT

  all other libraries need to be installed directly via Arduino IDE library manager
  
*/

#define DRIVER_VERSION  "0.6.0"

#define ALPACA_PORT   80
#define ALPACA_DISCOVERY_PORT  32227

// Import required libraries
#ifdef ESP32  // not tested
   #include <WiFi.h>
   #include <ESPAsyncWebServer.h>
#else
   #include <Arduino.h>
   #include <ESP8266WiFi.h> // https://github.com/esp8266/Arduino/tree/master/libraries/ESP8266WiFi
   //#include <Hash.h>
   //#include <ESPAsyncTCP.h>
   #include <ESP8266WebServer.h>  //https://github.com/esp8266/Arduino/tree/master/libraries/ESP8266WebServer
   ESP8266WebServer server(ALPACA_PORT);
   //#include <ESP8266mDNS.h>
   #include <WiFiUdp.h>
   //todo  UDP Port can be edited in setup page
   int udpPort = ALPACA_DISCOVERY_PORT;
   WiFiUDP Udp;
#endif

// name at the router ESP-1E2B43 (last 3 fields of mac address)


#include <WiFiManager.h>
const String DEVICE_SSID = "AutoConnectAP";

#include "ArduinoOTA.h"  

#include <Ticker.h>  // this is only available for esp8266?. Used by several
  
#include "sensors.h" // everything related to the weather sensors
#include "alpaca.h"  


// WiFi credentials for local WiFi network
String WIFI_SSID; 
String WIFI_PASSWORD;

void connectToWifi() {
  WiFi.begin(WIFI_SSID, WIFI_PASSWORD);
  Serial.print("Connecting to Wi-Fi :");
  // Wait for connection
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(WiFi.status()); //".");  6 means working on it. 3 means done ?
  }
  Serial.println("");
  Serial.print("Connected to ");
  Serial.println(WIFI_SSID);
  Serial.print("IP address: ");
  Serial.println(WiFi.localIP());
  Serial.print("mac address: ");
  Serial.println(WiFi.macAddress());
}


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

  ArduinoOTA.begin();     

  server.on("/", handleRoot);
  server.onNotFound(handleNotFound);
  server.on("/reset", handleReset);

  server.on("/setup", handleSetup);

  sensors_setup();
  //x related to I2C scan, trying to find I2C devices
  //x  Wire.begin();
  //x  Serial.println("\nI2C Scanner");
  
  alpaca_setup();

  
  // server
  server.begin(); 
  Serial.println("HTTP server started");
  Serial.println(WiFi.localIP());

  //Starts the discovery responder server
  Udp.begin( udpPort);
  

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

int udpBytesIn;
void loop() {
  server.handleClient();
  
  // Every X number of seconds (interval set in sensors.h)
  sensor_update();
  
/* for checking I2C connections
//xxxxxxxxxxxxxxxx
  byte error, address;
  int nDevices;
 
  Serial.println("Scanning...");
 
  nDevices = 0;
  for(address = 1; address < 127; address++ )
  {
    // The i2c_scanner uses the return value of
    // the Write.endTransmisstion to see if
    // a device did acknowledge to the address.
    Wire.beginTransmission(address);
    error = Wire.endTransmission();
 
    if (error == 0)
    {
      Serial.print("I2C device found at address 0x");
      if (address<16)
        Serial.print("0");
      Serial.print(address,HEX);
      Serial.println("  !");
 
      nDevices++;
    }
    else if (error==4)
    {
      Serial.print("Unknown error at address 0x");
      if (address<16)
        Serial.print("0");
      Serial.println(address,HEX);
    }    
  }
  if (nDevices == 0)
    Serial.println("No I2C devices found\n");
  else
    Serial.println("done\n");
 
  delay(5000);           // wait 5 seconds for next scan
//xxxxxxxxxxxxxxxxxxxx
*/


  

  udpBytesIn = Udp.parsePacket();
  if( udpBytesIn > 0  ) 
     handleDiscovery( udpBytesIn );

  //ArduinoOTA.handle();     
  
}
