// This allows you to communicate with a FHEM Server.
// (c) Copyright 2018 Philipp Pfeiffer
// 

#include "Arduino.h"
#include "FHEM.h"
#include <base64.h>

#if defined(ESP8266)
#include <ESP8266HTTPClient.h>
#else
#include <HTTPClient.h>
#endif

FHEM::FHEM(String Server, String User, String Password)
{
	FHEM_Server = Server;
	String auth = User + ":" + Password;
	FHEM_Authentication = base64::encode(auth);
}

FHEM::FHEM(String Server, String Authentication)
{
	FHEM_Server = Server;
	FHEM_Authentication = Authentication;
}

FHEM::FHEM(String Server)
{
	FHEM_Server = Server;
}

String FHEM::LoadFromServer(String command)
{
	if (!CSRFTokenLoaded)
	{
		LoadCSRFToken();
	}

	PrepareClient(FHEM_Server, command);

	int httpCode = client.GET();

	String result = "";
	
	if (httpCode > 0 && httpCode < 400) //Check the returning code, 200 is ok
	{
		result = client.getString(); //Get the request response payload
	}
	else if (httpCode == 400)
	{
		//CSRF Token was not provided or otherwise false, reload CSRF Token next time:
		CSRFTokenLoaded = false;
	}
	else if (httpCode == 401)
	{
		//authentication was either missing or wrong
	}

	client.end();   //Close connection

	return result;
}

void FHEM::PrepareClient(String URL, String command)
{
	command.replace(" ", "%20");  //remove blank spaces. Other special characters are not regarded, these must be replaced before
	URL = URL + "?XHR=1&cmd=" + command;
	if (CSRFToken != "")
	{
		URL = URL + "&fwcsrf=" + CSRFToken;
	}
	PrepareClient(URL);
}

void FHEM::PrepareClient(String URL)
{
	if (FHEM_Authentication != "")
	{
		client.setAuthorization(FHEM_Authentication.c_str());
	}

	client.begin(URL);
}


void FHEM::LoadCSRFToken()
{
	PrepareClient(FHEM_Server);

	const char* headerNames[] = { "X-FHEM-csrfToken" };
	client.collectHeaders(headerNames, sizeof(headerNames) / sizeof(headerNames[0]));

	int httpCode = client.GET();
	if (httpCode > 0)
	{
		if (client.hasHeader("X-FHEM-csrfToken"))
		{
			CSRFToken = client.header("X-FHEM-csrfToken");
		}
	}
	client.end();

	CSRFTokenLoaded = true;
}
