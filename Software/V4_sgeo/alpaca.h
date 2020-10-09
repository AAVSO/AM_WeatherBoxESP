// WBox2 alpaca interface


#ifndef _WBOX2_ALPACA_H_
#define _WBOX2_ALPACA_H_

/*
  Alpaca documentation:  https://github.com/ASCOMInitiative/ASCOMRemote/blob/master/Documentation/ASCOM%20Alpaca%20API%20Reference.pdf
*/


bool connected;
String DriverName=         "observingconditions"; // ASCOM name
String DriverVersion=      DRIVER_VERSION;
String DriverInfo=         "V4_sgeo";
String Description=        "AAVSO AM_WeatherBox2, ESP8266, NodeMCU 0.9(ESP-12 Module)" ;
String InterfaceVersion=   "1"; // alpaca v1
#define GUID    "fa7b12dc-dff9-407c-a7f5-3b5e73b77c04"

#include <ASCOMAPICommon_rest.h>

#define DEVICE_NUMBER 0

void handleAPIversions(void)
{
    String message;
    uint32_t clientID = (uint32_t)server.arg("ClientID").toInt();
    uint32_t clientTransID = (uint32_t)server.arg("ClientTransactionID").toInt();

    DynamicJsonBuffer jsonBuff(256);
    JsonObject& root = jsonBuff.createObject();
    jsonResponseBuilder( root, clientID, clientTransID, ++serverTransID, "APIversions", Success, "" );  
    JsonArray& values  = root.createNestedArray("Value");     
    values.add(atoi(InterfaceVersion.c_str()));    
    root.printTo(message);
#ifdef DEBUG_ESP_HTTP_SERVER
DEBUG_OUTPUT.println( message );
#endif
    
    server.send(200, "application/json", message);
    return ;
}

void handleAPIconfiguredDevices(void)
{
    String message;
    uint32_t clientID = (uint32_t)server.arg("ClientID").toInt();
    uint32_t clientTransID = (uint32_t)server.arg("ClientTransactionID").toInt();

    DynamicJsonBuffer jsonBuff(256);
    JsonObject& root = jsonBuff.createObject();
    jsonResponseBuilder( root, clientID, clientTransID, ++serverTransID, "APIversions", Success, "" );  
    JsonArray& values  = root.createNestedArray("Value");
    
    DynamicJsonBuffer jsonBuff2(256);
    JsonObject& device= jsonBuff2.createObject();
    device["DeviceName"]= Description;
    device["DeviceType"]= DriverName;
    device["DeviceNumber"]= DEVICE_NUMBER;
    device["UniqueID"]= GUID;    

    values.add(device);    
    root.printTo(message);
#ifdef DEBUG_ESP_HTTP_SERVER
DEBUG_OUTPUT.println( message );
#endif
    
    server.send(200, "application/json", message);
    return ;
}



void alpaca_setup() {
   // management api
   server.on("/management/apiversions", handleAPIversions);
   server.on("/management/v1/configureddevices", handleAPIconfiguredDevices);

   //Common ASCOM handlers
   String preUri = "/api/v"+ InterfaceVersion+ "/";
   preUri += DriverName;
   preUri += "/";
   preUri += DEVICE_NUMBER;
   preUri += "/";
   server.on(preUri+"action",              HTTP_PUT, handleAction );
   server.on(preUri+"commandblind",        HTTP_PUT, handleCommandBlind );
   server.on(preUri+"commandbool",         HTTP_PUT, handleCommandBool );
   server.on(preUri+"commandstring",       HTTP_PUT, handleCommandString );
   server.on(preUri+"connected",           handleConnected );
   server.on(preUri+"description",         HTTP_GET, handleDescriptionGet );     //
   server.on(preUri+"driverinfo",          HTTP_GET, handleDriverInfoGet );
   server.on(preUri+"driverversion",       HTTP_GET, handleDriverVersionGet );
   server.on(preUri+"interfaceversion",    HTTP_GET, handleInterfaceVersionGet );
   server.on(preUri+"name",                HTTP_GET, handleNameGet );
   server.on(preUri+"supportedactions",    HTTP_GET, handleSupportedActionsGet );
}





#endif