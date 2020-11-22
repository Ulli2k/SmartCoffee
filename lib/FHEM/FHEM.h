// FHEM.h

#ifndef FHEM_h
#define FHEM_h

#include "Arduino.h"

#if defined(ESP8266)
#include <ESP8266HTTPClient.h>
#else
#include <HTTPClient.h>
#endif

class FHEM
{
public:
	FHEM(String Server); //Server is required with port like this: http://192.168.1.1:8083/fhem
	FHEM(String Server, String User, String Password); //Server is required with port like this: http://192.168.1.1:8083/fhem
	FHEM(String Server, String Authentication); //auth is base64 encoded already
	String LoadFromServer(String command);
private:
	void PrepareClient(String URL);
	void PrepareClient(String URL, String command);
	void LoadCSRFToken();
	HTTPClient client;
	bool CSRFTokenLoaded = false;
	String CSRFToken = "";
	String FHEM_Server = "";
	String FHEM_Authentication = "";
};

#endif

