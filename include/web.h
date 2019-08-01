#ifndef __WEB_H__
#define __WEB_H__

// #include <ESP8266WiFi.h>
#include <WiFi.h>
#include <WiFiClient.h>
#include <ArduinoOTA.h>

#include <web_mqtt.h>
#include <web_server.h>
#include <web_global.h>

class ClassWiFi {

private:

public:
  void connect2WLAN(const char* ssid, const char* password) {
    WiFi.begin(ssid, password);     //Connect to your WiFi router
    // wifi_station_set_hostname(DEVICENAME);

    Serial.print("Connecting to "); Serial.print(ssid);
    while (!isConnected()) {
      // Wait for connection
      delay(500);
      Serial.print(".");
    }
    Serial.print(".successful with IP address "); Serial.println(getIP());
  }

  bool isConnected() {
    return (WiFi.status() == WL_CONNECTED);
  }

  IPAddress getIP() {
    return WiFi.localIP();
  }
};


class ClassOTA {

private:
  bool OTAactive;

public:
  void publishOTA() {
    // OTA - Flash
    activateOTA(true);
    ArduinoOTA.begin();
    ArduinoOTA.setHostname(DEVICENAME);
  }

  void activateOTA(bool on) {
    OTAactive = on;
  }

  void pollOTA() {
    if(OTAactive) { ArduinoOTA.handle(); }
  }
};

class ClassWebMessages {

private:

public:

};

class noWebInterface {
  public:
  void initialize() {}
  void poll() {}
};
typedef noWebInterface noWeb;


template <typename WEBconnection>
class ClassWeb : public ClassWiFi, public ClassOTA, public ClassWebMessages {

  private:
    WEBconnection web;

  public:
    ClassWeb () { }

    void send(const char *topic, const char *msg) {
      web.send(topic,msg);
    }

    void initialize() {

      connect2WLAN(ssid, password);
      publishOTA();

      // if(type == WEBPAGE) {
      //   web = new ClassWebServer(ssid, password);
      // } else {
      // }
      web.initialize();
    }

    void poll() {
      pollOTA();
      web.poll();
    }
};

#endif
