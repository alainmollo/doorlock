#include "CommandManager.h"

// Default constructor
CommandManagerClass::CommandManagerClass(Sim800L * _Sim800, RtcDS3231<TwoWire> * _Rtc, RfidManagerClass * _RfidManager, PlanningManager * _planning)
{
	Sim800 = _Sim800;
	Rtc = _Rtc;
	RfidManager = _RfidManager;
	planning = _planning;
}

bool CommandManagerClass::TreatCommand(String * Message, String * Who, uint8_t From)
{
	Logger.Log(F("Command was launched by "), false);
	Logger.Log(*Who);
	Logger.Log(*Message);
	bool result = LaunchCommand(Message, Who, From);
	if (!result)
		ReplyToSender(F("CMD ERROR"), Who, From);
	return result;
}

bool CommandManagerClass::TreatCommand(char * Message, String * Who, uint8_t From)
{
	String * tmpstr = new String(Message);
	bool result = TreatCommand(tmpstr, Who, From);
	delete tmpstr;
	return result;
}

bool CommandManagerClass::TreatCommand(String * Message, char * Who, uint8_t From)
{
	String * tmpstr = new String(Who);
	bool result = TreatCommand(Message, tmpstr, From);
	delete tmpstr;
	return result;
}

bool CommandManagerClass::TreatCommand(char * Message, char * Who, uint8_t From)
{
	String * tmpstr1 = new String(Message);
	String * tmpstr2 = new String(Who);
	bool result = TreatCommand(tmpstr1, tmpstr2, From);
	delete tmpstr1;
	delete tmpstr2;
	return result;
}

bool CommandManagerClass::LaunchCommand(String * Message, String * Who, uint8_t From)
{	
	if (AnalyseSms(Message, F("MQTT")))
	{
		Logger.Log(F("MQTT Initialisation"));

		return true;
	}

	// Reset the system
	// RSETEND#533
	if (AnalyseSms(Message, F("RSET")))
	{
		display.clear();
		display.drawString(DISPLAY_WIDTH / 2, DISPLAY_HEIGHT / 2, "Restart");
		display.display();
		delay(1000);
		ESP.restart();

		return true;
	}

	// Reset the system
	// OTAEND#
	if (AnalyseSms(Message, F("OTA")))
	{
		otaManager.Ota(true);

		return ReplyToSender(F("OTA was launched..."), Who, From);
	}

	// Unlock door
	// 0 -> 3 ex: UNLK1END#100
	if (AnalyseSms(Message, F("UNLK")))
	{
		double  x = Message->toFloat();
		uint8_t toLock = pow(2, x);
		// All PCF8574 ouptuts are low level
		WritePCF8574(!toLock);
		delay(50);

		return ReplyToSender(F("OK"), Who, From);
	}

	// Store planning information
	// DATA@0010$4A325E66@0020$55666A23END#1983
	if (AnalyseSms(Message, F("DATA")))
	{
		display.clear();
		display.drawString(DISPLAY_WIDTH / 2, 20, F("Datas Setting ..."));
		display.display();

		int ix = 0;
		while (ix < Message->length())
		{
			if (Message->charAt(ix) == '@')
			{
				ix++;
				String * addr = new String();
				while (ix < Message->length() && Message->charAt(ix) != '$')
				{
					*addr += Message->charAt(ix);
					ix++;
				}
				ix++;
				String * data = new String();
				while (ix < Message->length() && Message->charAt(ix) != '@')
				{
					*data += Message->charAt(ix);
					ix++;
				}

				int deciAdress = strtol(addr->c_str(), 0, 16);
				bool selector = true;

				// more than $2000 bank adress for internal eeprom (offset -> +12) else DS3231 eeprom
				if (deciAdress > 8191)
				{
					deciAdress = deciAdress - 8180;
					selector = false;
				}

				for (int j = 0; j < data->length(); j = j + 2)
				{
					uint8_t deciData = strtol((*data).substring(j, j + 2).c_str(), nullptr, 16);
					if (selector)
						EepromDs3231Class::i2c_eeprom_write_byte(0x57, deciAdress++, deciData); // datas less than 0x2000
					else
						EEPROM.write(deciAdress++, deciData); // datas adress upper or equal at 0x2000
					delay(10);
				}
				delete addr;
				delete data;
			}
		}
		return ReplyToSender(F("OK"), Who, From);
	}

	// Send AT Command to Sim800
	// SIM800@AT+CSQ?END#xxx
	if (AnalyseSms(Message, F("SIM800")))
		return SENDATCOMMAND(Message, Who, From);

	// Dump planning informations
	// DUMP@0010END#782
	if (AnalyseSms(Message, F("DUMP")))
	{
		display.clear();
		display.drawString(DISPLAY_WIDTH / 2, 20, F("Dump Asking ... "));
		display.display();

		int ix = 0;
		if (Message->charAt(ix) == '@')
		{
			ix++;
			String * addr = new String();
			while (ix < Message->length())
			{
				*addr += Message->charAt(ix);
				ix++;
			}

			int deciAdress = strtol(addr->c_str(), 0, 16);
			bool selector = true;

			// more than $2000 bank adress for internal eeprom (offset -> +12) else DS3231 eeprom
			if (deciAdress > 8191)
			{
				deciAdress = deciAdress - 8180;
				selector = false;
			}

			String * output = new String();
			for (int j = 0; j < 48; j++)
			{
				uint8_t result;
				if (selector)
					result = EepromDs3231Class::i2c_eeprom_read_byte(0x57, deciAdress + j); // dump less than 0x2000
				else
					result = EEPROM.read(deciAdress + j); // dump adress upper or equal at 0x2000
				delay(10);
				if (result < 16)
					*output += F("0");
				*output += String(result, HEX);;
			}
			output->toUpperCase();

			int crc = 0;
			for (int i = 0; i < output->length(); i++)
			{
				crc += output->charAt(i);
			}
			*output = "@" + *addr + ":" + *output + "#" + String(crc);

			Logger.Log(F("Send Dump :"), false);
			Logger.Log(*output);

			bool result = ReplyToSender(*output, Who, From);

			delete addr;
			delete output;
			return result;
		}
		else
			return false;
	}

	// Set phone number for Callback
	// CLBK+33614490515END#1111
	if (AnalyseSms(Message, F("CLBK")))
	{
		if (Message->length() == 12)
		{
			// Set callback number eeprom storage adress
			int adress = 0x0000;
			for (int i = 0; i < 12; i++)
			{
				EEPROM.write(adress + i, Message->charAt(i));
				delay(10);
			}

			display.clear();
			display.drawString(DISPLAY_WIDTH / 2, 20, F("Set CallBack Num"));
			display.display();
			display.lockDisplay();

			return ReplyToSender(F("OK"), Who, From);
		}
		else
			return false;
	}

	// Create a one day planning entry for 1 reservation card 45 5A 32 AA
	// Or Delete a one day planning entry for 1 reservation
	// ENTRY@14,2,1,1:455A32AAEND#111
	// DELTE@14,2,1,1END#111
	if (Message->startsWith(F("ENTRY")) || Message->startsWith(F("DELTE")))
		return ENTRY(Message, Who, From);

	// delete a card 45 5A 32 AA
	// DELKD@455A32AAEND#111
	if (AnalyseSms(Message, F("DELKD")))
	{
		int ix = 0;
		if (Message->charAt(ix) == '@')
		{
			ix++;
			String * s5 = new String();
			while (ix < Message->length())
			{
				*s5 += Message->charAt(ix);
				ix++;
			}

			String * x = new String();
			char * Qstr = new char[3];
			*(Qstr + 2) = 0;
			*x = s5->substring(0, 2);
			strncpy(Qstr, x->c_str(), 2);
			byte Q1 = strtol(Qstr, 0, 16);
			*x = s5->substring(2, 4);
			strncpy(Qstr, x->c_str(), 2);
			byte Q2 = strtol(Qstr, 0, 16);
			*x = s5->substring(4, 6);
			strncpy(Qstr, x->c_str(), 2);
			byte Q3 = strtol(Qstr, 0, 16);
			*x = s5->substring(6);
			strncpy(Qstr, x->c_str(), 2);
			byte Q4 = strtol(Qstr, 0, 16);
			delete s5;
			delete Qstr;
			delete x;

			Logger.Log(String(Q1, HEX), false);
			Logger.Log(F(","), false);
			Logger.Log(String(Q2, HEX), false);
			Logger.Log(F(","), false);
			Logger.Log(String(Q3, HEX), false);
			Logger.Log(F(","), false);
			Logger.Log(String(Q4, HEX));

			RtcDateTime time = Rtc->GetDateTime();
			uint8_t day = time.Day();
			uint8_t month = time.Month();

			bool ret = planning->tryDeleteCard(day, month, Q1, Q2, Q3, Q4);
			if (ret)
				return ReplyToSender(F("OK"), Who, From);
			else
				return ReplyToSender(F("KO"), Who, From);
		}
		else
			return false;
	}

	// emulate rfid scan for card 45 5A 32 AA
	// RFID@455A32AAEND#111
	if (AnalyseSms(Message, F("RFID")))
	{
		int ix = 0;
		if (Message->charAt(ix) == '@')
		{
			ix++;
			String * s5 = new String();
			while (ix < Message->length())
			{
				*s5 += Message->charAt(ix);
				ix++;
			}

			String * x = new String();
			char * Qstr = new char[3];
			*(Qstr + 2) = 0;
			*x = s5->substring(0, 2);
			strncpy(Qstr, x->c_str(), 2);
			byte Q1 = strtol(Qstr, 0, 16);
			*x = s5->substring(2, 4);
			strncpy(Qstr, x->c_str(), 2);
			byte Q2 = strtol(Qstr, 0, 16);
			*x = s5->substring(4, 6);
			strncpy(Qstr, x->c_str(), 2);
			byte Q3 = strtol(Qstr, 0, 16);
			*x = s5->substring(6);
			strncpy(Qstr, x->c_str(), 2);
			byte Q4 = strtol(Qstr, 0, 16);
			delete s5;
			delete Qstr;
			delete x;

			Logger.Log(String(Q1, HEX), false);
			Logger.Log(F(","), false);
			Logger.Log(String(Q2, HEX), false);
			Logger.Log(F(","), false);
			Logger.Log(String(Q3, HEX), false);
			Logger.Log(F(","), false);
			Logger.Log(String(Q4, HEX));

			uint8 * tag = new uint8[4];
			tag[0] = Q1;
			tag[1] = Q2;
			tag[2] = Q3;
			tag[3] = Q4;
			rfidTreatCommand(tag, Who, From);
			delete tag;

			return ReplyToSender(F("OK"), Who, From);
		}
		else
			return false;
	}

	// Retrieve a one day planning entry (ex : 1 junary)
	// PLAN@1,1END#720
	if (AnalyseSms(Message, F("PLAN")))
	{
		int ix = 0;
		if (Message->charAt(ix) == '@')
		{
			ix++;
			String * s1 = new String();
			while (ix < Message->length() && Message->charAt(ix) != ',')
			{
				*s1 += Message->charAt(ix);
				ix++;
			}
			
			yield();

			ix++;
			String * s2 = new String();
			while (ix < Message->length() && Message->charAt(ix) != '@')
			{
				*s2 += Message->charAt(ix);
				ix++;
			}

			byte deciDay = strtol(s1->c_str(), 0, 10);
			byte deciMonth = strtol(s2->c_str(), 0, 10);
			delete s1;
			delete s2;

			byte * buffer = new byte[144];

			Logger.Log(String(deciDay), false);
			Logger.Log(F(","), false);
			Logger.Log(String(deciMonth));

			planning->getPlanningEntry(deciDay, deciMonth, buffer);

			String * bloc = new String();

			byte * calc = buffer;

			for (int k = 0; k < 144; k++)
			{
				String * output = new String();
				if (*calc < 16)
					*output += F("0");
				*output += String((*calc), HEX);
				calc++;

				*bloc += *output;

				delete output;
			}
			delete buffer;

			yield();

			bloc->toUpperCase();

			int crc = 0;
			for (int i = 0; i < 144; i++)
			{
				crc += bloc->charAt(i);
			}

			String * subs = new String(bloc->substring(0, 144));
			*subs += "@0#";
			*subs += String(crc);

			Logger.Log(*subs);

			bool result = ReplyToSender(*subs, Who, From);

			if (From == COMMAND_FROM_SIM800)
			{
				Sim800->sendAtCommand("", true);
				Sim800->waitOK();
			}
			delete subs;

			crc = 0;
			for (int i = 144; i < 288; i++)
			{
				crc += bloc->charAt(i);
			}

			String * subd = new String(bloc->substring(144));
			*subd += "@1#";
			*subd += String(crc);

			Logger.Log(*subd);

			yield();

			result &= ReplyToSender(*subd, Who, From);

			if (From == COMMAND_FROM_SIM800)
			{
				Sim800->sendAtCommand("", true);
				Sim800->waitOK();
			}
			delete subd;
			delete bloc;

			return result;
		}
		else
			return false;
	}

	// Discover planning for one month
	// DISCVEND#0
	if (AnalyseSms(Message, F("DISCV")))
	{
		RtcDateTime time = Rtc->GetDateTime();
		uint8_t day = time.Day();
		uint8_t month = time.Month();

		String * buffer = new String();

		planning->discoverPlanning(day, month, buffer);

		int crc = 0;
		for (int i = 0; i < 144; i++)
		{
			crc += buffer->charAt(i);
		}

		*buffer += "#";
		*buffer += String(crc);
		bool result = ReplyToSender(*buffer, Who, From);
		delete buffer;

		return result;
	}

	// Clean all planning tables
	// CLEANEND#0
	if (AnalyseSms(Message, F("CLEAN")))
	{
		display.clear();
		display.drawString(DISPLAY_WIDTH / 2, 20, F("Please Wait ..."));
		display.display();
		display.lockDisplay();

		planning->cleanPlanningEntry();

		return ReplyToSender(F("OK"), Who, From);
	}

	return false;
}

// Decode SMS and check validity, return valid string or null
// La forme correcte est :
// PLAN301116END#814 par exemple
// RSETEND#533
bool CommandManagerClass::AnalyseSms(String * message, String code)
{
	if (message->startsWith(code))
	{
		int diezeindex = message->indexOf(F("#"));
		String dieze = message->substring(diezeindex + 1);
		int crcmessage = dieze.toInt();
		int crc = 0;
		for (int i = 0; i < diezeindex; i++)
		{
			crc += message->charAt(i);
		}

		Logger.Log(F("Analyze Sms:"), false);
		Logger.Log(String(crc));

		if (crc == crcmessage || Logger.IsLogging())
		{
			int endindex = message->indexOf(F("END"));
			if (endindex != -1)
			{
				int atindex = message->indexOf(F("@"));
				if (atindex != -1)
					*message = message->substring(atindex, endindex);
				else
					*message = message->substring(1, endindex);
				return true;
			}
		}
		return false;
	}
	return false;
}

// Command treatment function when Tag was detected
void CommandManagerClass::rfidTreatCommand(uint8 * tagid)
{
	rfidTreatCommand(tagid, nullptr, COMMAND_FROM_INTERNAL);
}

// Command treatment function when Tag was detected
void CommandManagerClass::rfidTreatCommand(uint8 * tagid, String * Who, uint8_t From)
{
	Logger.Log(F("rfidTreatCommand"));

	RtcDateTime now = Rtc->GetDateTime();
	byte door;
	if (planning->checkPlanning(now, tagid[0], tagid[1], tagid[2], tagid[3], door))
	{
		if (door != 0)
		{
			uint8_t toLock = pow(2, door - 1);
			// All PCF8574 ouptuts are low level
			WritePCF8574(!toLock);
			delay(50);

			display.clear();
			display.drawString(DISPLAY_WIDTH / 2, 20, F("Porte debloquee."));
			display.display();
			display.lockDisplay();

			String * toSend = new String(F("TRACE:"));
			toSend->concat(String(tagid[0], HEX));
			toSend->concat(String(tagid[1], HEX));
			toSend->concat(String(tagid[2], HEX));
			toSend->concat(String(tagid[3], HEX));
			toSend->concat(F("-"));
			toSend->concat(String(door, DEC));
			toSend->toUpperCase();

			ReplyToSender(*toSend, Who, From);

			delete toSend;

			Logger.Log(F("Porte debloquee."));
		}
		else
		{
			display.clear();
			display.drawString(DISPLAY_WIDTH / 2, 20, F(" Rien trouve ..."));
			display.display();
			display.lockDisplay();

			Logger.Log(F("Rien trouve ..."));
		}
	}
	else
	{
		display.clear();
		display.drawString(DISPLAY_WIDTH / 2, 20, F("Carte rejetee..."));
		display.display();
		display.lockDisplay();

		Logger.Log(F("Carte rejetee..."));
	}
}

// Set PCF8574 IO outputs to value
void CommandManagerClass::WritePCF8574(byte value)
{
	Wire.beginTransmission(PCF8574_ADDRESS);
	Wire.write(value);
	Wire.endTransmission();
}

// Answer to incoming request
bool CommandManagerClass::ReplyToSender(String reply, String * Who, uint8_t From)
{
	switch (From)
	{
	case COMMAND_FROM_SERIAL:
		Serial.println(reply);
		delay(10);
		return true;
	case COMMAND_FROM_SIM800:
		return sendSms(Who, &reply);
	case COMMAND_FROM_MQTT:
		return true;
	default:
		return true;
	}
}

// Send message to callback number
bool CommandManagerClass::sendSms(String * callbackNumber, String * message)
{
	Logger.Log(F("sending Sms to "), false);
	Logger.Log(*callbackNumber, false);
	Logger.Log(F(" : "), false);
	Logger.Log(*message);

	return Sim800->sendSms(*callbackNumber, *message);
}