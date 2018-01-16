/*
RfidManager.h

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

#ifndef _RFIDMANAGER_h
#define _RFIDMANAGER_h

#define RFID_MAX_LEN 12

#include "Logger.h"
#include <SoftwareSerial.h>

class RfidManagerClass : public SoftwareSerial
{
protected:
	// Store last rfid detection to ovoid multiple detection
	uint8 lastrfid[RFID_MAX_LEN];

	// Store last tag was check
	uint8 lastTag[RFID_MAX_LEN];

public:
	// Default constructor
	RfidManagerClass(void);

	// Setup Rx & Tx pins constructor
	RfidManagerClass(uint8_t, uint8_t);

	// Set up serial baud rate for rfid module communication
	void init();

	// Check if rfid sensor detect a tag and return tag value else null
	bool CheckRfid(uint8 *);

	// Reset the last tag buffer
	void clearBuffer(void);
};
#endif