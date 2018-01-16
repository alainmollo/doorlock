// RFID Library
// Permit to detect rfid presence and give up meta data values about user

#include "RfidManager.h"

// Default constructor
RfidManagerClass::RfidManagerClass(void)
	: SoftwareSerial(D5, D0)
{
}

// RX & TX setting pin constructor
RfidManagerClass::RfidManagerClass(uint8_t receivePin, uint8_t transmitPin)
	: SoftwareSerial(receivePin, transmitPin)
{
}

// Setup serial baud rate for Rfid module
void RfidManagerClass::init()
{
	// Software serial for Rfid
	this->begin(9600);

	Logger.Log(F("Rfid module was initialised"));
}

bool RfidManagerClass::CheckRfid(uint8 * rfidCard)
{
	String rfidStr;

	// Check if data arrived from Rfid sensor
	if (this->available())
	{
		uint8 readchar;
		byte _count = 0;
		while (this->available() && (readchar = this->read()) != 3)
		{
			if (readchar != 2 && readchar != 3)
				rfidCard[_count++] = readchar;
			delay(10);
		}

		bool testValid = true;
		for (int i = 0; i < RFID_MAX_LEN; i++)
		{
			if (rfidCard[i] != lastrfid[i])
			{
				testValid = false;
				break;
			}
		}
		for (int i = 0; i < RFID_MAX_LEN; i++)
		{
			lastrfid[i] = rfidCard[i];
		}

		// Tags are correctly identified
		if (testValid)
		{
			bool testValid = false;
			for (int i = 0; i < RFID_MAX_LEN; i++)
			{
				if (rfidCard[i] != lastTag[i])
				{
					testValid = true;
					break;
				}
			}
			// To prevent multiple detection of the same tag
			if (testValid)
			{
				for (int i = 0; i < RFID_MAX_LEN; i++)
				{
					lastTag[i] = rfidCard[i];
				}

				for (int i = 0; i < RFID_MAX_LEN; i++)
				{
					rfidStr += String(rfidCard[i], HEX);
				}

				Logger.Log(F("Rfid tag was detected :"), false);
				Logger.Log(rfidStr);

				// to prevent wdt error
				yield();
				return true;
			}
		}
	}
	// to prevent wdt error
	yield();
	return false;
}

// Forget last tag id to permit same tag detection again
void RfidManagerClass::clearBuffer(void)
{
	Logger.Log(F("Clear Rfid tag buffer"));

	for (int i = 0; i < RFID_MAX_LEN; i++)
		lastTag[i] = 0;
}