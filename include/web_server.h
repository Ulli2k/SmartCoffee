#ifndef __WEB_SERVER__
#define __WEB_SERVER__

#ifdef ESP32
#include <WebServer.h>
#else
#include <ESP8266WebServer.h>
#endif
#include "web_server_index.h" //Our HTML webpage contents with javascripts
#include <web_global.h>

#ifdef FHEM_SERVER_URL
#include <FHEM.h>
#endif

class ClassWebServer {

private:
  #ifdef ESP32
  WebServer server;
  #else
  ESP8266WebServer server; //Server on port 80
  #endif

  #ifdef FHEM_SERVER_URL
  FHEM fhem;
  #endif

public:

  ClassWebServer() : server(80)
                              #ifdef FHEM_SERVER_URL
                              , fhem(FHEM_SERVER_URL)
                              #endif
                             { }

  void initialize() {

    attachQuery("/", std::bind(&ClassWebServer::handleRoot, this));
    attachQuery("/PowerOn", std::bind(&ClassWebServer::handlePowerOn, this));
    attachQuery("/PowerOff", std::bind(&ClassWebServer::handlePowerOff, this));
    // attachQuery("/getValues", std::bind(&ClassWebServer::handleGetValues, this));
    // attachQuery("/setValues", std::bind(&ClassWebServer::handleSetValues, this));

    hostWebPages();
    Serial.println("HTTP server started");
  }

  void poll() {
    server.handleClient();          //Handle client requests
  }

  void postPowerState(bool on) {
    #ifdef FHEM_SERVER_URL
    fhem.LoadFromServer(on ? FHEM_POST_MESSAGE_ON : FHEM_POST_MESSAGE_OFF);
    #endif
  }

  void handleRoot() {
    send(200, "text/html", "");
  }
  void handlePowerOn() {
    setValueByInterface("Power",(const char*)"on");
    send(200, "text/html", "");
  }
  void handlePowerOff() {
    setValueByInterface("Power",(const char*)"off");
    send(200, "text/html", "");
  }

  // void handleGetValues() {
  //
  //   String data =   "{"
  //                   " \"resetGraph\":\""          + getValueForInterface("resetGraph") + "\""
  //                 + ", \"TempSens\":\""           + getValueForInterface("TempSens") + "\""
  //                 + ", \"TempAvgSens\":\""        + getValueForInterface("TempAvgSens") + "\""
  //                 + ", \"SetPoint\":\""           + getValueForInterface("SetPoint") + "\""
  //                 + ", \"PID\":\""                + getValueForInterface("PID") + "\""
  //                 + ", \"OnOffState\":\""         + getValueForInterface("OnOffState") + "\""
  //                 + ", \"StateMachine\":\""       + getValueForInterface("StateMachine") + "\""
  //                 + ", \"BrewingTimer\":\""       + getValueForInterface("BrewingTimer") + "\""
  //                   "}";
  //
  //   send(200, "text/plane", data.c_str()); //Send ADC value only to client ajax request
  // }
  //
  // void handleSetValues() {
  //   String OnOff          = getQueryArguments("OnOff");
  //   String startBrewing   = getQueryArguments("startBrewing");
  //   String SetPointValue  = getQueryArguments("SetPoint");
  //
  //   if(OnOff != "")         setValueByInterface("OnOff", OnOff);
  //   if(startBrewing != "")  setValueByInterface("startBrewing", startBrewing);
  //   if(SetPointValue != "") setValueByInterface("SetPoint", SetPointValue);
  //
  //   stayAtQueryPage("/");
  // }

  #ifdef ESP32
  void attachQuery(const String& str, WebServer::THandlerFunction fkn) {
  #else
  void attachQuery(const String& str, ESP8266WebServer::THandlerFunction fkn) {
  #endif
    server.on(str, fkn);
  }

  const String&  getQueryArguments(const char *value) {
    return server.arg(value);
  }

  void stayAtQueryPage(const char *value = NULL) {
    if(value) server.sendHeader("Location", value);
    server.send(302, "text/plain", "Updated-- Press Back Button");  //This Line Keeps It on Same Page
  }

  // code - HTTP response code, can be 200 or 404
  // content_type - HTTP content type, like "text/plain" or "image/png"
  // content - actual content body
  void send(int code, char* content_type, const String& content) {
    server.send(code, content_type, content);
  }

  void hostWebPages() {
    // attachQuery("/", std::bind(&WebInterface::handleRoot, this));
    // server.on("/readADC", handleADC); //This page is called by java Script AJAX
    server.begin();                  //Start server
  }


};


#endif
