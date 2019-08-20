#ifndef __WEB_H__
#define __WEB_H__

#ifdef ESP32
#include <WiFi.h>
#include <time.h>
#else
#include <ESP8266WiFi.h>
#endif
#include <WiFiClient.h>
#include <ArduinoOTA.h>

// #include <web_mqtt.h>
#include <web_server.h>
#include <web_global.h>
#ifdef ESP32
  #include <ESP32Ticker.h>
#else
  #define IRAM_ATTR
  #include <Ticker.h>
#endif

#define NTP_SERVER            "europe.pool.ntp.org"
#define GMT_OFFSET_SEC        3600
#define DAYLIGHT_OFFSET_SEC   3600

class ClassWiFi {

private:
  Ticker ticker;

  void setupTime() {
    struct tm timeinfo;
    configTime(GMT_OFFSET_SEC, DAYLIGHT_OFFSET_SEC, NTP_SERVER);
    if(!getLocalTime(&timeinfo)){
      Serial.println("Failed to obtain time");
      return;
    }
    globalValues.TimeDate.min = 0b111111;
    staticUpdateTime();
    Serial.println(&timeinfo, "Local Time: %A, %B %d %Y %H:%M:%S");

    ticker.attach(20, &ClassWiFi::staticUpdateTime);
  }

public:
  static void staticUpdateTime() {
    struct tm timeinfo;
    if(!getLocalTime(&timeinfo)){
      Serial.println("Failed to obtain time");
      return;
    }
    if(globalValues.TimeDate.min == timeinfo.tm_min) return;
    globalValues.TimeDate.hour    = timeinfo.tm_hour;
    globalValues.TimeDate.min     = timeinfo.tm_min;
    globalValues.TimeDate.wday    = timeinfo.tm_wday;
    globalValues.TimeDate.day     = timeinfo.tm_mday;
    globalValues.TimeDate.month   = timeinfo.tm_mon+1;
    globalValues.TimeDate.updated = true;
  }

  bool connect2WLAN(const char* ssid, const char* password) {
    uint8_t wifi_retry=0;
    uint8_t wifi_wait=0;

    //WiFi.waitForConnectResult() != WL_CONNECTED <-- Testen!
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
      setupTime();
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
class ClassWeb : public ClassOTA, public ClassWebMessages {

  private:
    ClassWiFi wifi;
    WEBconnection web;

  public:
    ClassWeb () { }

    void send(const char *topic, const char *msg) {
      web.send(topic,msg);
    }

    void postPowerState(bool on) {
      if(!wifi.isConnected()) return;
      web.postPowerState(on);
    }

    void initialize() {

      if(!wifi.connect2WLAN(ssid, password)) return;

      publishOTA();

      // if(type == WEBPAGE) {
      //   web = new ClassWebServer(ssid, password);
      // } else {
      // }
      web.initialize();

      web.postPowerState(false);
    }

    void poll() {
      if(!wifi.isConnected()) return;
      pollOTA();
      web.poll();
    }
};

#endif
