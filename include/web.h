#ifndef __WEB_H__
#define __WEB_H__

#ifdef ESP32
#include <WiFi.h>
#else
#include <ESP8266WiFi.h>
#endif
#include <WiFiClient.h>
#include <ArduinoOTA.h>

#include <web_mqtt.h>
#include <web_server.h>
#include <web_global.h>

class ClassWiFi {

private:

public:
  bool connect2WLAN(const char* ssid, const char* password) {
    uint8_t wifi_retry=0;
    uint8_t wifi_wait=0;

    while (!isConnected() && wifi_retry < 2) {
      wifi_wait=0;
      wifi_retry++;
      Serial.print("Connecting to "); Serial.print(ssid);
      WiFi.disconnect();
      WiFi.mode(WIFI_OFF);
      WiFi.mode(WIFI_STA);
      delay(500);
      WiFi.begin(ssid, password);     //Connect to your WiFi router
      while (!isConnected() && wifi_wait < 5) {
        wifi_wait++;
        // Wait for connection
        delay(500);
        Serial.print(".");
      }
      Serial.println();
    }
    if(isConnected()) {
      Serial.print("WiFi IP address "); Serial.println(getIP());
      return true;
    } else {
      Serial.println("WiFi failed, proceed offline.");
      return false;
    }
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

      if(!connect2WLAN(ssid, password)) return;

      publishOTA();

      // if(type == WEBPAGE) {
      //   web = new ClassWebServer(ssid, password);
      // } else {
      // }
      web.initialize();
    }

    void poll() {
      if(!isConnected()) return;
      pollOTA();
      web.poll();
    }
};

#endif
