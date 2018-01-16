/*
CommandManager.h

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

#ifndef _COMMANDMANAGER_h
#define _COMMANDMANAGER_h

// I2C adress for PCF8574
#define PCF8574_ADDRESS 0x38

// Command sender declaration
#define COMMAND_FROM_SERIAL   0
#define COMMAND_FROM_SIM800   1
#define COMMAND_FROM_MQTT     2
#define COMMAND_FROM_INTERNAL 3

#include "Logger.h"
#include <SoftwareSerial.h>
#include <Wire.h>
#include "RtcDS3231.h"
#include "SSD1306.h"
#include "Sim800L.h"
#include "RfidManager.h"
#include "PlanningManager.h"
#include "displayManager.h"
#include "OtaManager.h"

class CommandManagerClass
{
protected:
	// Init Sim800l library
	Sim800L * Sim800;

	// Setup DS3231 library
	RtcDS3231<TwoWire> * Rtc;

	// Setup Rfid manager class
	RfidManagerClass * RfidManager;

	// planning Manager
	PlanningManager * planning;

	// Real treatment
	bool LaunchCommand(String *, String *, uint8_t);

	bool sendSms(String *, String *);

	bool AnalyseSms(String *, String);

	bool ReplyToSender(String, String *, uint8_t);

	bool ENTRY(String * Message, String * Who, uint8_t From);

	bool SENDATCOMMAND(String * Message, String * Who, uint8_t From);
public:
	// Default constructor
	CommandManagerClass(Sim800L *, RtcDS3231<TwoWire> *, RfidManagerClass *, PlanningManager *);

	// Launch command treatment
	bool TreatCommand(String *, String *, uint8_t);

	// Launch command treatment
	bool TreatCommand(char *, String *, uint8_t);

	// Launch command treatment
	bool TreatCommand(String *, char *, uint8_t);

	// Launch command treatment
	bool TreatCommand(char *, char *, uint8_t);

	// Modifiy PCF8574 port
	void WritePCF8574(byte);

	// Launch rfid treatment
	void rfidTreatCommand(uint8 *);

	// Launch rfid treatment
	void rfidTreatCommand(uint8 *, String *, uint8_t);
};
#endif