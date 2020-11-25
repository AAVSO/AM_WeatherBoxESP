#ifndef _JAMESSYNGE_ARDUINO_EXPERIMENTS_SIMPLE_HTTP_SERVER_H_
#define _JAMESSYNGE_ARDUINO_EXPERIMENTS_SIMPLE_HTTP_SERVER_H_

#include "Ethernet.h"
#include "addresses.h"

// Some chip select pin numbers:
constexpr int kEthernetShieldCS = 10;    // Most Arduino shields
constexpr int kMkrEthShieldCS = 5;       // MKR ETH shield
constexpr int kTeensy20CS = 0;           // Teensy 2.0
constexpr int kTeensyPlusPlus20CS = 20;  // Teensy++ 2.0
constexpr int kAdafruitEsp8266FeatherwingCS = 15;  // ESP8266 with Adafruit Featherwing Ethernet
constexpr int kAdafruitEsp32FeatherwingCS = 33;    // ESP32 with Adafruit Featherwing Ethernet

// Wraps up all interactions with Ethernet and EthernetServer,
// except for handling a client connection, which is delegated
// to function ptr passed in to loop.
class SimpleHttpServer {
public:
  using ClientFunc = void(*)(EthernetClient*);

  SimpleHttpServer(int chip_select_pin, int port=80);

  // Setup the Ethernet chip and start listening for connections. Returns false
  // if unable to configure addresses or if there is no Ethernet hardware, else
  // returns true.
  // It *MAY* help you identify devices on your network as using this software
  // if they have the same "Organizationally Unique Identifier" (the first 3
  // bytes of the MAC address).
  bool setup(const OuiPrefix* oui_prefix=nullptr);

  // Check for a new client connection, and if found pass it to handler.
  // Also ensures that the DHCP lease (if there is one) is maintained.
  // Returns false if the DHCP lease is lost.
  bool loop(ClientFunc handler);

  // Skip input bytes until we reach the end of the HTTP request header
  // (i.e. read an empty line). Returns true if the connection is still open
  // after reading the entire header, false otherwise.
  static bool skipHttpRequestHeader(EthernetClient* client);

private:
  EthernetServer server_;
  bool using_dhcp_{false};
};

#endif  // _JAMESSYNGE_ARDUINO_EXPERIMENTS_SIMPLE_HTTP_SERVER_H_
