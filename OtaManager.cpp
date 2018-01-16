#include "OtaManager.h"

const char *ssid = "Livebox-33C0";
const char *password = "hoeQi9dcNzNFXhvemC";
const char* mySsid = "ESP8266";

IPAddress ip(192, 168, 11, 4);
IPAddress gateway(192, 168, 11, 1);
IPAddress subnet(255, 255, 255, 0);

ESP8266WebServer server(80);

// Init state
OtaManagerClass::OtaManagerClass()
{
	State = false;
}

// Return Ota State
bool OtaManagerClass::CheckState()
{
	return State;
}

// Refresh OTA actions
void OtaManagerClass::Refresh()
{
	ArduinoOTA.handle();
	server.handleClient();
}

// Response for root web site of server
void OtaManagerClass::handleRoot() {
	Logger.Log(F("Ota handle Root"));

	server.send(200, "text/plain", "hello from esp8266!");
}

// Prepare OTA in Access Point Mode
void OtaManagerClass::AccessPointOTA()
{
	Logger.Log(F("Ota Access Point"));
	WiFi.mode(WIFI_AP);

	WiFi.softAPConfig(ip, gateway, subnet);
	boolean apsuccess = WiFi.softAP(mySsid, password);

	if (apsuccess)
	{
		// Wait for connection
		String st;
		st = "";
		while (WiFi.softAPgetStationNum() == 0) {
			delay(500);
			st += ".";
			display.clear();
			display.drawString(64, 10, "Access Point");
			display.drawString(64, 30, WiFi.softAPIP().toString());
			display.drawString(64, 40, st);
			display.display();
			if (st.length() > 30)
			{
				st = ".";
				display.clear();
				display.drawString(64, 10, "Access Point");
				display.drawString(64, 30, WiFi.softAPIP().toString());
				display.drawString(64, 40, st);
				display.display();
				delay(10);
			}
			delay(10);
		}

		Logger.Log(F("Ota AP Ok..."));
		State = true;
	}
	else
	{
		Logger.Log(F("Ota cannot be AP"));

		// Access point was unavaible => Restart...
		display.clear();
		display.drawString(64, 10, "Error");
		display.drawString(64, 30, "occured.");
		display.drawString(64, 50, "Restarting...");
		display.display();
		delay(1000);
		ESP.restart();
	}
}

// OTA in Appearing Mode
void OtaManagerClass::AppearingOTA()
{
	Logger.Log(F("Ota Appearing"));

	display.clear();
	display.drawString(64, 10, "Connecting to");
	display.drawString(64, 30, String(ssid));
	display.display();
	delay(10);

	WiFi.mode(WIFI_STA);

	WiFi.begin(ssid, password);

	// Wait for connection
	String st;
	st = "";
	while (WiFi.status() != WL_CONNECTED) {
		delay(500);
		st += ".";
		display.drawString(64, 40, st);
		display.display();
		if (st.length() > 30)
		{
			st = ".";
			display.clear();
			display.drawString(64, 10, "Connecting to");
			display.drawString(64, 30, String(ssid));
			display.drawString(64, 40, st);
			display.display();
			delay(10);
		}
		delay(10);
	}

	Logger.Log(F("Ota Appearing Ok..."));
	State = true;
}

// Ota mechanism
void OtaManagerClass::Ota(bool fromCommand) {
	String hostname(HOSTNAME);
	hostname += String(ESP.getChipId(), HEX);
	WiFi.hostname(hostname);

	delay(1000);
	// if we stay pressed D3 => Accesspoint OTA also Appearing OTA
	if (digitalRead(D3) == 1 && !fromCommand)
		AccessPointOTA();
	else
		AppearingOTA();

	MDNS.begin(HOSTNAME);
	server.on("/", OtaManagerClass::handleRoot);
	server.on("/version", []() {
		server.send(200, "text/plain", "DoorLock version ");
		server.send(200, "text/plain", String(VERSION));
	});

	server.begin();
	ArduinoOTA.setHostname((const char *)hostname.c_str());
	ArduinoOTA.begin();
	ArduinoOTA.onStart([]() {
		display.clear();
		display.drawString(DISPLAY_WIDTH / 2, DISPLAY_HEIGHT / 2 - 10, "OTA Update");
		display.display();
	});

	ArduinoOTA.onProgress([](unsigned int progress, unsigned int total) {
		display.drawProgressBar(4, 32, 120, 8, progress / (total / 100));
		display.display();
	});

	ArduinoOTA.onEnd([]() {
		display.clear();
		display.drawString(DISPLAY_WIDTH / 2, DISPLAY_HEIGHT / 2, "Restart");
		display.display();
	});

	// Align text vertical/horizontal center
	display.clear();
	display.drawString(DISPLAY_WIDTH / 2, DISPLAY_HEIGHT / 2, "Ready for OTA\n" + (WiFi.localIP().toString() == "0.0.0.0" ? WiFi.softAPIP().toString() : WiFi.localIP().toString()));
	Logger.Log(F("Ready for OTA"));
	Logger.Log(WiFi.localIP().toString() == "0.0.0.0" ? WiFi.softAPIP().toString() : WiFi.localIP().toString());
	display.display();
}

// OTA Manager instanciate
OtaManagerClass otaManager;