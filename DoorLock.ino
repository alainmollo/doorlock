/*
DoorLock.ino

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

// Embeded libraries
#include "Tools.h"
#include "displayManager.h"

// Start up method entry
void setup() {
	// Setup Serial communication to 115200 bauds for debugging over serial/usb line
	Serial.begin(115200);

	// Setup Rfid serial module
	RfidManager.init();

	// Set D3 as input pull up for detecting key press action
	// at start up to select normal mode or OTA
	pinMode(D4, INPUT);
	pinMode(D3, INPUT);
	bootState = digitalRead(D3);

	// Setup wire library = I2C
	Wire.begin(D1, D2);

	//Setup Rtc library
	Rtc.Begin();

	display.init();
	display.setContrast(255);
	display.setTextAlignment(TEXT_ALIGN_CENTER_BOTH);
	display.setFont(ArialMT_Plain_16);
	display.drawString(DISPLAY_WIDTH / 2, DISPLAY_HEIGHT / 2, "Booting ...\n");
	display.display();
	delay(10);

	// Check wich kind of booting we want during startUp
	int count = 0;
	while (count < BOOT_DURATION && bootState != 0)
	{
		bootState = digitalRead(D3);
		delay(10);
		count++;
	}

	// Ota Booting
	if (bootState == 0)
		otaManager.Ota(false);
	else
	{
		// Cut wifi for reduce energy consumption
		WiFi.mode(WIFI_OFF);

		display.init();
		display.setContrast(255);
		display.clear();
		display.drawString(DISPLAY_WIDTH / 2, 20, F("Loading ...\n"));
		display.drawString(DISPLAY_WIDTH / 2, 40, F("Please wait !\n"));
		display.display();

		SetUpNormalMode();
	}
}

// Booting entry in normal mode
void SetUpNormalMode()
{
	Logger.Log(F("Check Sim800 Init State"));
	// Initialization of Sim800L module
	if (!Sim800.Init())
	{
		// Signal an error
		display.clear();
		display.drawString(DISPLAY_WIDTH / 2, 20, F("Sim800 fault"));
		display.display();

		Logger.Log(F("Sim800 fault"));

		delay(10000);
		ESP.restart();
	}
	else
	{
		Logger.Log(F("Sim800 was init."));

		// Show GSM Level signal
		display.clear();
		display.drawString(DISPLAY_WIDTH / 2, 20, F("Signal  level:"));
		uint8_t levelSignal = Sim800.getLevelSignal(false);
		display.drawString(DISPLAY_WIDTH / 2, 40, String(levelSignal));
		display.display();

		String * dtetme = new String();

		// Get network date and time
		Sim800L::dateTime * dte = new Sim800L::dateTime();
		Sim800.RTCtime(dtetme, dte);

		Logger.Log(F("Obtain DateTime from network:"), false);
		Logger.Log(*dtetme);

		if (!(dte->year < 15))
		{
			// Set DS3231 date and time
			RtcDateTime dt(dte->year, dte->month, dte->day, dte->hour, dte->minute, dte->second);

			Rtc.SetDateTime(dt);
			delay(10);

			Logger.Log(F("Set DateTime from network"));
		}
	}

	// Clear PCF outputs
	CommandManager.WritePCF8574(0xFF);
}

// Redirect SMS Message to Command Treatment
void smsTreatCommand(Sim800L::smsReader * smsCommand)
{
	if (!CommandManager.TreatCommand(&smsCommand->Message, &smsCommand->WhoSend, COMMAND_FROM_SIM800))
	{
		Logger.Log(F("Command error :"), false);
		Logger.Log(smsCommand->Message);
	}
}

// Loop process to survey every tasks
void loop()
{
	if (otaManager.CheckState())
		bootState = 0;

	// If bootState == 0, we choose OTA firmware upgrading
	if (bootState == 0)
	{
		otaManager.Refresh();
		// Check if serial data arrived and send to TreatCommand
		if (Serial.available())
		{
			String * cmdstr = new String();
			while (Serial.available())
			{
				cmdstr->concat((char)Serial.read());
				delay(10);
			}
			if (cmdstr->length() > 0)
				if (!CommandManager.TreatCommand(cmdstr, "SERIAL", COMMAND_FROM_SERIAL))
				{
					Logger.Log(F("Command error :"), false);
					Logger.Log(cmdstr->c_str());
				}
			delete cmdstr;
		}
		yield();
	}
	// Else Normal Mode
	else
	{
		if (Sim800.checkBootOk() && bootState < FAULT_CYCLE)
		{
			// Check if serial data arrived and send to TreatCommand
			if (Serial.available())
			{
				String * cmdstr = new String();
				while (Serial.available())
				{
					cmdstr->concat((char)Serial.read());
					delay(10);
				}
				if (cmdstr->length() > 0)
					if (!CommandManager.TreatCommand(cmdstr, "SERIAL", COMMAND_FROM_SERIAL))
					{
						Logger.Log(F("Command error :"), false);
						Logger.Log(cmdstr->c_str());
					}
				delete cmdstr;
			}

			// Check if Rfid Tag was detected
			uint8 rfidCard[RFID_MAX_LEN];
			if (RfidManager.CheckRfid(rfidCard))
			{
				dutyCycle = 0;
				if (Logger.IsLogging())
				{
					String rfidStr;
					display.clear();
					display.drawString(DISPLAY_WIDTH / 2, 10, "Normal Mode");
					for (int i = 0; i < RFID_MAX_LEN / 2; i++)
					{
						rfidStr += String(rfidCard[i], HEX);
					}
					display.drawString(DISPLAY_WIDTH / 2, 30, rfidStr);
					rfidStr = "";
					for (int i = RFID_MAX_LEN / 2; i < RFID_MAX_LEN; i++)
					{
						rfidStr += String(rfidCard[i], HEX);
					}
					display.drawString(DISPLAY_WIDTH / 2, 45, rfidStr);
					display.display();
					delay(10);
				}
				else
				{
					display.clear();
					display.drawString(DISPLAY_WIDTH / 2, 10, "Controle carte");
					display.display();
					delay(10);
				}
				CommandManager.rfidTreatCommand(&rfidCard[0]);
			}

			// After a long cycle time (approximatively 1.5 second), we check date/time validity
			if (bootState != FAULT_CYCLE && bootState++ > MAX_CYCLE)
			{
				if (!Rtc.IsDateTimeValid())
					Logger.Error(F("RTC lost confidence in the DateTime"));

				display.printDateTime(Rtc);

				//if (!mqtt.IsMqttStart())
				//{
				//	// Check if SMS is present and send message to TreatCommand method
				//	if (Sim800.checkSMS())
				//	{
				//		// Launch SMS treatment if present
				//		if (!Sim800.ReadSMSTreatment(smsTreatCommand))
				//		{
				//			if (errorCounter++ > MAX_SMS_READING_ERROR)
				//				bootState = FAULT_CYCLE;
				//		}
				//		else
				//			errorCounter = 0;
				//	}
				//}

				// Reset the bootState value to 1
				if (bootState != FAULT_CYCLE)
					bootState = 1;

				dutyCycle++;
			}

			// We reset PCF outputs
			if ((dutyCycle % 6) == 5 && bootState == 1)
			{
				CommandManager.WritePCF8574(0xFF);
			}

			// We can tag again after 10 seconds
			if ((dutyCycle % 11) == 10 && bootState == 1)
			{
				RfidManager.clearBuffer();
			}

			//if (mqtt.IsMqttStart())
			//{
			//	if (mqtt.available())
			//	{
			//	}

			//	mqtt.processing();
			//}

			if (dutyCycle > MAX_DUTY_CYCLE)
			{
				dutyCycle = 0;
				Logger.Log(F("MAX DUTY CYCLE DETECTED"));

				//if (!mqtt.IsMqttStart())
				//{
				//	// We check each minute if SMS wasn't interrupt detected
				//	if (!Sim800.ReadSMSTreatment(smsTreatCommand))
				//	{
				//		if (errorCounter++ > MAX_SMS_READING_ERROR)
				//			bootState = FAULT_CYCLE;
				//	}
				//	else
				//		errorCounter = 0;
				//}
			}
		}
		else
			if (bootState == FAULT_CYCLE)
			{
				// Signal an error
				display.clear();
				display.drawString(DISPLAY_WIDTH / 2, 20, F("System fault\n"));
				display.drawString(DISPLAY_WIDTH / 2, 40, F("Restarting...\n"));
				display.display();

				Logger.Log(F("System fault"));

				delay(10000);
				ESP.restart();
			}
			else
				if (!Sim800.checkBootOk())
				{
					// Signal an error
					display.clear();
					display.drawString(DISPLAY_WIDTH / 2, 20, F("Sim800 fault\n"));
					display.drawString(DISPLAY_WIDTH / 2, 40, F("Restarting...\n"));
					display.display();

					Logger.Log(F("Sim800 fault"));

					delay(10000);
					ESP.restart();
				}
	}
}