#ifndef __ARDUINO_OTA_H
#define __ARDUINO_OTA_H

#include <ESP8266WiFi.h>
#include <WiFiUdp.h>
#include <functional>

class UdpContext;

typedef enum {
  OTA_IDLE,
  OTA_WAITAUTH,
  OTA_RUNUPDATE
} ota_state_t;

typedef enum {
  OTA_AUTH_ERROR,
  OTA_BEGIN_ERROR,
  OTA_CONNECT_ERROR,
  OTA_RECEIVE_ERROR,
  OTA_END_ERROR
} ota_error_t;

class ArduinoOTAClass
{
  public:
	typedef std::function<void(void)> THandlerFunction;
	typedef std::function<void(ota_error_t)> THandlerFunction_Error;
	typedef std::function<void(unsigned int, unsigned int)> THandlerFunction_Progress;

    ArduinoOTAClass();
    ~ArduinoOTAClass();

    //Sets the service port. Default 8266
    void setPort(uint16_t port);

    //Sets the device hostname. Default esp8266-xxxxxx
    void setHostname(const char *hostname);
    String getHostname();

    //Sets the password that will be required for OTA. Default NULL
    void setPassword(const char *password);

    //Sets the password as above but in the form MD5(password). Default NULL
    void setPasswordHash(const char *password);

    //Sets if the device should be rebooted after successful update. Default true
    void setRebootOnSuccess(bool reboot);

    //This callback will be called when OTA connection has begun
    void onStart(THandlerFunction fn);

    //This callback will be called when OTA has finished
    void onEnd(THandlerFunction fn);

    //This callback will be called when OTA encountered Error
    void onError(THandlerFunction_Error fn);

    //This callback will be called when OTA is receiving data
    void onProgress(THandlerFunction_Progress fn);

    //Starts the ArduinoOTA service
    void begin();

    //Call this in loop() to run the service
    void handle();

    //Gets update command type after OTA has started. Either U_FLASH or U_SPIFFS
    int getCommand();

  private:
    int _port;
    String _password;
    String _hostname;
    String _nonce;
    UdpContext *_udp_ota;
    bool _initialized;
    bool _rebootOnSuccess;
    ota_state_t _state;
    int _size;
    int _cmd;
    int _ota_port;
    IPAddress _ota_ip;
    String _md5;

    THandlerFunction _start_callback;
    THandlerFunction _end_callback;
    THandlerFunction_Error _error_callback;
    THandlerFunction_Progress _progress_callback;

    void _runUpdate(void);
    void _onRx(void);
    int parseInt(void);
    String readStringUntil(char end);
};

#if !defined(NO_GLOBAL_INSTANCES) && !defined(NO_GLOBAL_ARDUINOOTA)
extern ArduinoOTAClass ArduinoOTA;
#endif

void OTA_setup() {
// Port defaults to 8266
  // ArduinoOTA.setPort(8266);

  // Hostname defaults to esp8266-[ChipID]
  // ArduinoOTA.setHostname("myesp8266");

  // No authentication by default
  // ArduinoOTA.setPassword("admin");

  // Password can be set with it's md5 value as well
  // MD5(admin) = 21232f297a57a5a743894a0e4a801fc3
  // ArduinoOTA.setPasswordHash("21232f297a57a5a743894a0e4a801fc3");
  #define U_FLASH true
  ArduinoOTA.onStart([]() {
    String type;
    if (ArduinoOTA.getCommand() == U_FLASH)
      type = "sketch";
    else // U_SPIFFS
      type = "filesystem";

    // NOTE: if updating SPIFFS this would be the place to unmount SPIFFS using SPIFFS.end()
    Serial.println("Start updating " + type);
  });
  ArduinoOTA.onEnd([]() {
    Serial.println("\nEnd");
  });
  ArduinoOTA.onProgress([](unsigned int progress, unsigned int total) {
    Serial.printf("Progress: %u%%\r", (progress / (total / 100)));
  });
  ArduinoOTA.onError([](ota_error_t error) {
    Serial.printf("Error[%u]: ", error);
    if (error == OTA_AUTH_ERROR) Serial.println("Auth Failed");
    else if (error == OTA_BEGIN_ERROR) Serial.println("Begin Failed");
    else if (error == OTA_CONNECT_ERROR) Serial.println("Connect Failed");
    else if (error == OTA_RECEIVE_ERROR) Serial.println("Receive Failed");
    else if (error == OTA_END_ERROR) Serial.println("End Failed");
  });
  ArduinoOTA.begin();
  Serial.println("Ready OTA8");


}


#endif /* __ARDUINO_OTA_H */
