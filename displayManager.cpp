#include "displayManager.h"

displayManager::displayManager(uint8_t _address, uint8_t _sda, uint8_t _scl)
	: SSD1306(_address, _sda, _scl)
{
	displayLock = false;
}

void displayManager::lockDisplay(void)
{
	displayLock = true;
	countSeconds = currentSeconds;
}

void displayManager::unlockDisplay(void)
{
	displayLock = false;
}

void displayManager::printDateTime(RtcDS3231<TwoWire> & Rtc)
{
	RtcDateTime dt = Rtc.GetDateTime();
	RtcTemperature tm = Rtc.GetTemperature();

	currentSeconds = dt.TotalSeconds();
	if (!displayLock)
	{
		char datestring[11];
		char timestring[9];

		snprintf_P(datestring,
			countof(datestring),
			PSTR("%02u/%02u/%04u"),
			dt.Day(),
			dt.Month(),
			dt.Year());
		snprintf_P(timestring,
			countof(timestring),
			PSTR("%02u:%02u:%02u"),
			dt.Hour(),
			dt.Minute(),
			dt.Second());
		String logdisp;
		logdisp = datestring;
		logdisp += F(" ; ");
		logdisp += timestring;

		this->clear();
		if (Logger.IsLogging())
		{
			this->drawString(DISPLAY_WIDTH / 2, 10, "Normal Mode");
		}
		else
		{
			this->drawString(DISPLAY_WIDTH / 2, 10, "DoorLock V1.00");
		}
		this->drawString(DISPLAY_WIDTH / 2, 30, datestring);
		this->drawString(DISPLAY_WIDTH / 2, 45, timestring);
		this->display();
		delay(10);

		logdisp += " ; " + String(ESP.getFreeHeap()) + " ; ";
		logdisp += String(tm.AsFloat()) + " oC";
		Logger.Log(logdisp);
		delay(10);
	}
	else
		if (currentSeconds - countSeconds > LOCK_SECONDS)
			unlockDisplay();
}

displayManager display(0x3c, D1, D2);