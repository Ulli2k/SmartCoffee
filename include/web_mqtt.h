#ifndef __WEB_MQTT_H__
#define __WEB_MQTT_H__

// #include <web.h>
#include <PubSubClient.h>
#include <web_global.h>
#ifdef ESP32
  #include <ESP32Ticker.h>
#else
  #include <Ticker.h>
#endif

#define MQTT_MAX_CONNECT_RETRIES        5
#define MQTT_PUBLISH_VALUE_INTERVALL    3000  // [ms]

#define MQTT_PATH(s)                  ((String(DEVICENAME) + "/" + (s)).c_str())

extern String callback_getValues4Interface();
extern void callback_setValuesFromInterface();

class ClassMQTT  {

private:
  WiFiClient espClient;
  PubSubClient client;
  Ticker ticker;

public:

  ClassMQTT() : client(espClient) { }

  void initialize() {
    using namespace std::placeholders;

    client.setServer(MQTT_BROCKER_IP, MQTT_PORT);
    client.setCallback(std::bind(&ClassMQTT::handleSetValues, this, _1,_2,_3));
    // client.setCallback(std::function(&ClassMQTT::handleSetValues, this, _1,_2,_3));
    reconnect();

    ticker.attach_ms(MQTT_PUBLISH_VALUE_INTERVALL, &ClassMQTT::staticPublishValuesPeriodically, this);
  }

  bool isConnected() {
    return (client.connected());
  }

  void reconnect() {
    uint8_t i=1;
    Serial.print("MQTT reconnecting...");
    while (!isConnected()) {
        if (!client.connect(DEVICENAME, NULL, MQTT_PASSWORD)) {
            if(i > MQTT_MAX_CONNECT_RETRIES) {
              Serial.print("failed, rc=");
              Serial.println(client.state());
            } else {
              Serial.print(".");
            }
            delay(500);
        } else {
          Serial.println(".successful");
          client.subscribe(MQTT_PATH("#"));
          send("state", "online");
          return;
        }
        i++;
    }
  }

  void poll() {
    if (!client.connected()) {
        reconnect();
    }
    client.loop();
  }

  void send(const char *topic, const char *msg) {
    if(!client.publish(MQTT_PATH(topic), msg)) {
        Serial.print("MQTT Publish <"); Serial.print(MQTT_PATH(topic)); Serial.print("|");
        Serial.print(msg); Serial.println("> failed");
    }
  }

  static void staticPublishValuesPeriodically(ClassMQTT *pThis) { pThis->publishValuesPeriodically(); }
  void publishValuesPeriodically() {
    String BrewingValues = "{"
                    " \"TempAvgSens\":\""        + getValueForInterface("TempAvgSens") + "\""
                  + ",\"SetPoint\":\""           + getValueForInterface("SetPoint") + "\""
                  + ",\"PID\":\""                + getValueForInterface("PID") + "\""
                    "}";
    send("BrewingValues", BrewingValues.c_str());
    send("BrewingTimer", getValueForInterface("BrewingTimer").c_str());
    send("StateMachine", getValueForInterface("StateMachine").c_str());
  }

  void handleSetValues(char* topic, byte* payload, unsigned int length) {
    // handle message arrived
    Serial.print("Received message [");
    topic += 1+strlen(DEVICENAME);
    Serial.print(topic);
    Serial.print("] ");
    char msg[length+1];
    for (unsigned int i = 0; i < length; i++) {
        // Serial.print((char)payload[i]);
        msg[i] = (char)payload[i];
    }
    // Serial.println();
    msg[length] = '\0';
    Serial.println(msg);
    setValueByInterface(topic,msg);
  }
};


#endif
