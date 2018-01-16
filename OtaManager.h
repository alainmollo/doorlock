/*
OtaManager.h

Copyright (c) 2015, Arduino LLC
Original code (pre-library): Copyright (c) 2016, Alain Mollo

This library is free software; you can redistribute it and/or
modify it under the terms of the GNU Lesser General Public
License as published by the Free Software Foundation; either
version 2.1 of the License, or (at your option) any later version.

This library is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
Lesser General Public License for more details.

You should have received a copy of the GNU Lesser General Public
License along with this library; if not, write to the Free Software
Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA  02110-1301  USA
*/

#ifndef _OTAMANAGER_h
#define _OTAMANAGER_h

#define HOSTNAME "ESP8266-OTA-"
#define VERSION "1.00"

#include <ESP8266WiFi.h>
#include <ESP8266WebServer.h>
#include <ESP8266mDNS.h>
#include <ESP8266HTTPUpdateServer.h>
#include <WiFiUdp.h>
#include <ArduinoOTA.h>

#include "displayManager.h"
#include "Logger.h"

class OtaManagerClass
{
protected:
	bool State;

	static void handleRoot();

	void AccessPointOTA(void);

	void AppearingOTA(void);
public:
	OtaManagerClass(void);

	void Refresh(void);

	void Ota(bool);

	bool CheckState(void);
};

extern OtaManagerClass otaManager;
#endif