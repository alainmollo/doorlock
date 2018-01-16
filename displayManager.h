/*
displayManager.h

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

#ifndef _DISPLAYMANAGER_h
#define _DISPLAYMANAGER_h

#define LOCK_SECONDS 5

// Calculate size of instance in object metric
#define countof(a) (sizeof(a) / sizeof(a[0]))

#include "SSD1306.h"
#include "RtcDS3231.h"
#include "Logger.h"

class displayManager : public SSD1306
{
protected:
	bool displayLock;

	uint32_t currentSeconds;

	uint32_t countSeconds;
public:
	displayManager(uint8_t, uint8_t, uint8_t);

	void lockDisplay(void);

	void unlockDisplay(void);

	void printDateTime(RtcDS3231<TwoWire> &);
};

extern displayManager display;
#endif