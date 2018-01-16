/*
Name:		PlanningManager.cpp
Created:	07/11/2016 17:18:26
Author:	Alain MOLLO
*/

#include "PlanningManager.h"
#define tolerance 2

// Permet de retrouver l'adresse en mémoire pour une valeur de planning en fonction de la date, du bateau et de la plage
unsigned int PlanningManager::adressPlanningCal(byte day, byte month, byte boat, byte timeextend)
{
	int intermediaire = ((day - 1) + ((month - 1) % 2) * 31) * 36 + ((boat - 1) * 6) + timeextend - 1;

	Logger.Log(F("adressPlanningCal:"), false);
	Logger.Log(String(intermediaire, DEC));

	return intermediaire;
}

// Retrieve a Card Quartet identifier from a Card number
// Q1 .. Q4 contain Quartet and return true if found the entry else return false
bool PlanningManager::retrieveCard(byte Card, byte & Q1, byte & Q2, byte & Q3, byte & Q4)
{
	if (Card <= 252)
	{
		int memad = Card * 4 + 12;
		Q1 = EEPROM.read(memad);
		delay(10);
		Q2 = EEPROM.read(memad + 1);
		delay(10);
		Q3 = EEPROM.read(memad + 2);
		delay(10);
		Q4 = EEPROM.read(memad + 3);
		delay(10);
		if (Q1 == 0 && Q2 == 0 && Q3 == 0 && Q4 == 0)
			return false;
		else
		{
			Logger.Log(F("retrieveCard : "), false);
			Logger.Log(String(Q1, HEX) + F(",") + String(Q2, HEX) + F(",") + String(Q3, HEX) + F(",") + String(Q4, HEX) + F(","));

			return true;
		}
	}
	else
		return false;
}

// Retrieve a Card number from Quartet or create an entry on the table
// Card number returned is between 0 and 252, memory overflow if 0xFF is returned
byte PlanningManager::checkCard(byte Q1, byte Q2, byte Q3, byte Q4, bool create)
{
	Logger.Log(F("checkCard : "), false);
	Logger.Log(String(Q1, HEX) + F(",") + String(Q2, HEX) + F(",") + String(Q3, HEX) + F(",") + String(Q4, HEX) + F(","));

	int j = 0;
	byte chr;
	byte chp;
	for (int i = 12; i < 1021; i = i + 4)
	{
		chr = EEPROM.read(i);
		delay(10);
		// On enregistre le premier élément libre dans j
		if (chr == 0 && j == 0)
		{
			chp = EEPROM.read(i + 1);
			delay(10);
			if (chp == 0)
			{
				chp = EEPROM.read(i + 2);
				delay(10);
				if (chp == 0)
				{
					chp = EEPROM.read(i + 3);
					delay(10);
					if (chp == 0) j = i;
				}
			}
		}
		// On cherche la correspondance
		if (chr == Q1)
		{
			chp = EEPROM.read(i + 1);
			delay(10);
			if (chp == Q2)
			{
				chp = EEPROM.read(i + 2);
				delay(10);
				if (chp == Q3)
				{
					chp = EEPROM.read(i + 3);
					delay(10);
					if (chp == Q4)
					{
						byte cardResult = (i - 12) / 4;

						Logger.Log(F("found card : "), false);
						Logger.Log(String(cardResult, DEC));

						return cardResult;
					}
				}
			}
		}

		if (i % 50 == 0)
			wdt_reset();
	}

	if (j > 0 && create)
	{
		EEPROM.write(j, Q1);
		delay(10);
		EEPROM.write(j + 1, Q2);
		delay(10);
		EEPROM.write(j + 2, Q3);
		delay(10);
		EEPROM.write(j + 3, Q4);
		delay(10);

		byte cardResult = (j - 12) / 4;

		Logger.Log(F("create card : "), false);
		Logger.Log(String(cardResult, DEC));


		return cardResult;
	}
	// Si l'on retourne 0xFF, c'est qu'il n'est plus possible de créer une entrée dans la table de cartes
	Logger.Log(F("no card."));

	return 0xFF;
}

// Delete a Card from card identifier
bool PlanningManager::tryDeleteCard(byte day, byte month, byte card)
{
	if (card != 0xFF)
	{
		unsigned int  memory = adressPlanningCal(day, month, 1, 1);
		bool flag = false;
		byte res = 0;
		// check if card is not used during a month
		for (int i = 0; i < 1008; i++)
		{
			res = EepromDs3231Class::i2c_eeprom_read_byte(DS3231_EEPROM_ADDRESS, memory);
			delay(10);

			if (++memory > 2231)
				memory = 0;

			if (i % 50 == 0)
				wdt_reset();

			if (res == card)
			{
				flag = true;
				break;
			}
		}

		if (!flag)
		{
			// We can delete the card
			int memcard = card * 4 + 12;
			for (byte k = 0; k < 4; k++)
			{
				EEPROM.write(memcard++, 0x00);
				delay(10);
			}
			return true;
		}
		else
			return false;
	}
	else
		return false;
}

// Delete a Card from l Quartet identifier
bool PlanningManager::tryDeleteCard(byte day, byte month, byte Q1, byte Q2, byte Q3, byte Q4)
{
	byte result = checkCard(Q1, Q2, Q3, Q4, false);
	tryDeleteCard(day, month, result);
}

// Create a planning entry
bool PlanningManager::createPlanningEntry(byte day, byte month, byte boat, byte timeextend, byte Q1, byte Q2, byte Q3, byte Q4)
{
	Logger.Log(F("createPlanningEntry"));

	unsigned int memory = adressPlanningCal(day, month, boat, timeextend);
	if (memory < 4096)
	{
		byte id = checkCard(Q1, Q2, Q3, Q4);
		if (id != 0xFF)
		{
			EepromDs3231Class::i2c_eeprom_write_byte(DS3231_EEPROM_ADDRESS, memory, id);
			delay(10);
			return true;
		}
		else
			return false;
	}
	else
		return false;
}

// Delete a planning entry
bool PlanningManager::deletePlanningEntry(byte day, byte month, byte boat, byte timeextend)
{
	Logger.Log(F("deletePlanningEntry"));

	unsigned int memory = adressPlanningCal(day, month, boat, timeextend);
	if (memory < 4096)
	{
		byte result = EepromDs3231Class::i2c_eeprom_read_byte(DS3231_EEPROM_ADDRESS, memory);
		delay(10);
		// check if card could be delete
		if (result != 0xFF)
			tryDeleteCard(day, month, result);
		EepromDs3231Class::i2c_eeprom_write_byte(DS3231_EEPROM_ADDRESS, memory, 0xFF);
		delay(10);
		return true;
	}
	else
		return false;
}

// Get a planning entry for a day
void PlanningManager::getPlanningEntry(byte day, byte month, byte * buffer)
{
	Logger.Log(F("getPlanningEntry"));

	unsigned int memory = adressPlanningCal(day, month, 1, 1);
	byte *reader = new byte[9];
	byte card;
	for (int j = 0; j < 4; j++)
	{
		EepromDs3231Class::i2c_eeprom_read_buffer(DS3231_EEPROM_ADDRESS, memory + (9 * j), reader, 9);
		delay(10);
		
		for (byte i = 0; i < 9; i++)
		{
			if ((card = *(reader + i)) != 0xFF)
			{
				byte Q1, Q2, Q3, Q4;
				if (retrieveCard(card, Q1, Q2, Q3, Q4))
				{
					buffer[i * 4 + 36 * j] = Q1;
					buffer[i * 4 + 1 + 36 * j] = Q2;
					buffer[i * 4 + 2 + 36 * j] = Q3;
					buffer[i * 4 + 3 + 36 * j] = Q4;
				}
				else
				{
					buffer[i * 4 + 36 * j] = 0;
					buffer[i * 4 + 1 + 36 * j] = 0;
					buffer[i * 4 + 2 + 36 * j] = 0;
					buffer[i * 4 + 3 + 36 * j] = 0;
				}
			}
			else
			{
				buffer[i * 4 + 36 * j] = 0;
				buffer[i * 4 + 1 + 36 * j] = 0;
				buffer[i * 4 + 2 + 36 * j] = 0;
				buffer[i * 4 + 3 + 36 * j] = 0;
			}
		}
	}
	delete reader;
}

// Clean a planning entry for a day
void PlanningManager::cleanPlanningEntry()
{
	Logger.Log(F("clean Card eeprom..."));

	for (int i = 12; i < 1024; i++)
	{
		EEPROM.write(i, 0);
		delay(10);
		if (i % 50 == 0)
			wdt_reset();
	}
	for (int i = 0; i < 2232; i++)
	{
		EepromDs3231Class::i2c_eeprom_write_byte(DS3231_EEPROM_ADDRESS, i, 0xFF);
		delay(10);
		if (i % 50 == 0)
			wdt_reset();
	}
	for (int i = 2232; i < 4096; i++)
	{
		EepromDs3231Class::i2c_eeprom_write_byte(DS3231_EEPROM_ADDRESS, i, 0x00);
		delay(10);
		if (i % 50 == 0)
			wdt_reset();
	}
}

// Delete 7 days before today of planning entry
void PlanningManager::deleteLastWeek(byte day, byte month)
{
	Logger.Log(F("delete 7 days before"));

	day--;
	if (day == 0)
	{
		day = 31;
		month--;
		if (month == 0)
			month = 12;
	}

	unsigned int memory = adressPlanningCal(day, month, 6, 6);
	int i = memory;
	byte res;
	for (byte c = 0; c < 252; c++)
	{
		res = EepromDs3231Class::i2c_eeprom_read_byte(DS3231_EEPROM_ADDRESS, i);
		delay(10);
		if (res != 0xFF)
		{
			tryDeleteCard(day, month, res);
			EepromDs3231Class::i2c_eeprom_write_byte(DS3231_EEPROM_ADDRESS, i, 0xFF);
			delay(10);
		}
		if (i-- == 0)
			i = 2231;
		if (i % 50 == 0)
			wdt_reset();
	}
}

// Read one month planning
void PlanningManager::discoverPlanning(byte day, byte month, String * buffer)
{
	Logger.Log(F("discover planning"));

	unsigned int memory = adressPlanningCal(day, month, 1, 1);
	byte res;
	uint8_t stk = 0;
	byte cnt;
	char car;
	for (int i = 0; i < 1008; i++)
	{
		cnt = i % 6;
		res = EepromDs3231Class::i2c_eeprom_read_byte(DS3231_EEPROM_ADDRESS, memory);

		if (res != 0xFF)
			stk += pow(2, cnt);
		delay(10);

		if (++memory > 2231)
			memory = 0;

		if (i % 50 == 0)
			wdt_reset();

		if (cnt == 5)
		{
			car = char(stk + 33);
			buffer->concat(car);

			Logger.Log(String(stk + 33, 10) + "=" + car + ",");

			stk = 0;
		}
	}
}

// Check planning and unlock a door if valid card at the right time
bool PlanningManager::checkPlanning(RtcDateTime now, byte Q1, byte Q2, byte Q3, byte Q4, byte & door)
{
	Logger.Log(F("check planning"));

	unsigned int memory = adressPlanningCal(now.Day(), now.Month(), 1, 1);
	byte card = checkCard(Q1, Q2, Q3, Q4, false);
	door = 0;
	// if card equal 0xFF, don't recongnise else ok
	if (card != 0xFF)
	{
		// We are searching one day before today if card have reservation
		// We must compare range of time for each timextend (stored in top of eeprom)

		byte * buffer = new byte[24];
		int * rangein = new int[6];
		int * rangeout = new int[6];
		byte count = 0;
		EepromDs3231Class::i2c_eeprom_read_buffer(DS3231_EEPROM_ADDRESS, 4072, buffer, 24);
		delay(10);
		for (int p = 0; p < 24; p = p + 4)
		{
			int * tmp;
			tmp = (int *)(buffer + p);
			if (*tmp != 0xFFFF)
				rangein[count] = *tmp - tolerance;
			else
				rangein[count] = *tmp;
			if (rangein[count] < 0)
				rangein[count] = 0;
			tmp = (int *)(buffer + p + 2);
			if (*tmp != 0xFFFF)
				rangeout[count++] = *tmp + tolerance;
			else
				rangeout[count++] = *tmp;
		}
		delete buffer;
		
		byte range = 0xFF;
		// Detect the range we are
		for (byte z = 0; z < 6; z++)
		{
			if (rangein[z] != 0xFFFF && rangeout[z] != 0xFFFF)
				if (now.Hour() >= rangein[z] && now.Hour() < rangeout[z])
				{
					range = z;
					// We must check here the validity for each corresponding range

					break;
				}
		}
		delete rangein;
		delete rangeout;

		Logger.Log(F("range = "), false);
		Logger.Log(String(range, 10));

		byte res;
		memory += range;
		// range value 0xFF mean no range !
		if (range != 0xFF)
		{
			// door = 1; => door property represent the door to open with return true else return false
			for (range = 0; range < 6; range++)
			{
				res = EepromDs3231Class::i2c_eeprom_read_byte(DS3231_EEPROM_ADDRESS, memory);
				delay(10);
				Logger.Log(F("check boat "), false);
				Logger.Log(String(range + 1, 10), false);
				Logger.Log(F(","), false);
				Logger.Log(F("read mem = "), false);
				Logger.Log(String(res, 10));
				if (res == card)
					break;
				memory += 6;
			}
			if (res == card)
			{
				door = range + 1;
				return true;
			}
			else
				return true;
		}
		else
			return false;
	}
	else
		return false;
}