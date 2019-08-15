#ifndef __WEB_GLOBAL__
#define __WEB_GLOBAL__

#define DEVICENAME  "SmartCoffee"


enum INTERFACETYPE { MQTT, WEBPAGE };

const char ssid[]     = "";
const char password[] = "";

#define MQTT_BROCKER_IP                 ""
#define MQTT_PORT                       1883
#define MQTT_PASSWORD                   ""

extern String getValueForInterface  (String name);
extern void   setValueByInterface   (String name, String value);

#endif
