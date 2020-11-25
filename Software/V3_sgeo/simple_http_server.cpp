#include "simple_http_server.h"

#include "addresses.h"
#include "eeprom_io.h"


SimpleHttpServer::SimpleHttpServer(int chip_select_pin, int port)
    : server_(port) {
  // Tell the Ethernet library which pin to use for CS (Chip Select). SPI
  // buses (as used by the Wiznet W5000 series Ethernet chips) use 3 common
  // signalling lines, but each device (chip) gets its own chip select line
  // (also called slave select) so that each can be activated independently.
  // This does not initiate any communications, just records the pin number.
  Ethernet.init(chip_select_pin);
}

bool SimpleHttpServer::setup(const OuiPrefix* oui_prefix) {
  // Load the addresses saved to EEPROM, if they were previously saved. If they
  // were not successfully loaded, then generate them and save them into the
  // EEPROM.
  Addresses addresses;
  addresses.loadOrGenAndSave(oui_prefix);

  Serial.print("MAC: ");
  Serial.println(addresses.mac);
  Serial.print("Default IP: ");
  Serial.println(addresses.ip);

  if (Ethernet.begin(addresses.mac.mac)) {
    // Yeah, we were able to get an IP address via DHCP.
    using_dhcp_ = true;
  } else {
    // No DHCP server responded with a lease on an IP address.
    // Is there hardware?
    if (Ethernet.hardwareStatus() == EthernetNoHardware) {
      // Oops, this isn't the right board to run this sketch.
      return false;
    }
    Serial.println("No DHCP");

    // No DHCP server responded with a lease on an IP address, so we'll
    // fallback to using our randomly generated IP.
    using_dhcp_ = false;
    Ethernet.setLocalIP(addresses.ip);

    // The link-local address range must not be divided into smaller
    // subnets, so we set our subnet mask accordingly:
    IPAddress subnet(255, 255, 0, 0);
    Ethernet.setSubnetMask(subnet);

    // Assume that the gateway is on the same subnet, at address 1 within
    // the subnet. This code will work with many subnets, not just a /16.
    IPAddress gateway = addresses.ip;
    gateway[0] &= subnet[0];
    gateway[1] &= subnet[1];
    gateway[2] &= subnet[2];
    gateway[3] &= subnet[3];
    gateway[3] |= 1;
    Ethernet.setGatewayIP(gateway);
  }

  Serial.print("IP: ");
  Serial.println(Ethernet.localIP());

  // These other 3 addresses aren't particularly necessary when we aren't
  // initiating IP communications, IIUC.
  // Serial.print("Subnet: ");
  // Serial.println(Ethernet.subnetMask());
  // Serial.print("Gateway: ");
  // Serial.println(Ethernet.gatewayIP());
  // Serial.print("DNS: ");
  // Serial.println(Ethernet.dnsServerIP());

  // Start listening for clients
  server_.begin();
  return true;
}

bool SimpleHttpServer::loop(ClientFunc handler) {
  // If we're using an IP address assigned via DHCP, renew the lease
  // periodically. The Ethernet library will do so at the appropriate interval
  // if we call it often enough.
  if (using_dhcp_) {
    switch (Ethernet.maintain()) {
      case 1: // Renew failed
      case 3: // Rebind failed
        Serial.println("WARNING! lost our DHCP assigned address!");
        return false;
    }
  }

  // Has a client connected since we last checked?
  EthernetClient client = server_.available();

  if (client) {
    // Looks like we have a client. However, there is a bug in EthernetServer
    // w.r.t. the W5100. In general, EthernetServer.available() returns an
    // EthernetClient with a sockIndex of MAX_SOCK_NUM when there is no
    // connection. However, the W5100 supports at most 4 sockets at once, not
    // the 8 supported by the W5200 and W5500 chips, so available() stops
    // searching for a new connection when it gets to index 4; BUT instead of
    // returning MAX_SOCK_NUM (typically 8), it returns the index it stopped at
    // 4. We can cover that up here, while waiting for the fix to be patched
    // and distributed.
    if (!(client.getSocketNumber() == 4 && 
      Ethernet.hardwareStatus() == EthernetW5100)) {
      // Looks valid.
      handler(&client);

      // Give the web browser time to receive the data.
      // TODO(jamessynge): Figure out if this delay, from one of the Ethernet
      // library examples, is really necessary. Are we basically making sure
      // that the chip has sent the data before closing this side of the
      // connection?
      delay(1);

      client.stop();
    }
  }

  return true;
}

bool SimpleHttpServer::skipHttpRequestHeader(EthernetClient* client) {
  boolean currentLineIsBlank = true;
  while (client->connected()) {
    if (!client->available()) {
      // Keep trying until we have some input, or the connection is closed.
      continue;
    }
    char c = client->read();
    Serial.print(c);
    if (c == '\n') {
      // Reached the end of the current line. If there was nothing on it
      // (i.e. it just ended \r\n), then we've consumed an empty line, and
      // have reached the end of the header. Note that we're not validating
      // that each line ends with exactly \r\n.
      if (currentLineIsBlank) {
        return true;
      }
      // We're starting a new line, and the last line wasn't empty.
      currentLineIsBlank = true;
    } else if (c != '\r') {
      // c isn't a line terminator, so the line isn't empty.
      currentLineIsBlank = false;
    }
  }
  // The connection was broken.
  return false;
}
