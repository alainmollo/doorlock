/*
Tools.h

Copyright (c) 2017, Mollo Alain
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

#ifndef _TOOLS_h
#define _TOOLS_h

// Including System libraries
#include "Logger.h"
#include <ESP8266WiFi.h>
#include <ESP8266WebServer.h>
#include <ESP8266mDNS.h>
#include <ESP8266HTTPUpdateServer.h>
#include <WiFiUdp.h>
#include <FS.h>
#include <ArduinoOTA.h>
#include <Wire.h>
#include <SoftwareSerial.h>

// Including External libraries
#include "displayManager.h"
#include "SSD1306.h"
#include "RfidManager.h"
#include <RtcDS3231.h>
#include "EepromDs3231.h"
#include "Logger.h"
#include "Sim800L.h"
#include "PlanningManager.h"
#include "CommandManager.h"
#include "OtaManager.h"
#include "GSM_MQTT.h"

// for ESP external API C
extern "C" {
#include "user_interface.h"
}

// Loop cycle count for entire task process
#define MAX_CYCLE 40000
#define FAULT_CYCLE 100000

// Each minute dyty cycle
#define MAX_DUTY_CYCLE 60

// Max retry number of SMS reading
#define MAX_SMS_READING_ERROR 5

// Wait while booting in 1/100 eme of seconds
#define BOOT_DURATION 100

// Current boot state for booting workflow
int bootState = 0;
int dutyCycle = 0;

// Loop sequence identifier
uint8_t errorCounter = 0;

// Init Sim800l library
Sim800L Sim800(D6, D8, D7, D4);

// Setup DS3231 library
RtcDS3231<TwoWire> Rtc(Wire);

// Setup Rfid manager class
RfidManagerClass RfidManager;

// planning Manager
PlanningManager planning;

// Command Manager for all entries (Serial, Sim800, EspWifi...)
CommandManagerClass CommandManager(&Sim800, &Rtc, &RfidManager, &planning);
#endif