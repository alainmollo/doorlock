/*
Name:		PlanningManager.h
Created:	07/11/2016 17:18:26
Author:	Alain MOLLO
*/

#ifndef _PLANNINGMANAGER_h
#define _PLANNINGMANAGER_h

#include <Wire.h>
#include <RtcDS3231.h>
#include "EepromDs3231.h"
#include <EEPROM.h>
#include "Logger.h"

class PlanningManager
{
 protected:
	 // Retrieve memory location that match with date and boat and timeextend
	 uint  adressPlanningCal(byte day, byte month, byte boat = 1, byte timeextend = 1);

	 // Retrieve or create a card index witch represent a quad bytes identifier
	 byte checkCard(byte Q1, byte Q2, byte Q3, byte Q4, bool create = true);
 public:
	 // Retrieve a Card Quartet identifier from a Card number
	 bool retrieveCard(byte Card, byte & Q1, byte & Q2, byte & Q3, byte & Q4);

	 // Delete a Card froml Quartet identifier
	 bool tryDeleteCard(byte day, byte month, byte Q1, byte Q2, byte Q3, byte Q4);

	 // Delete a Card from card identifier
	 bool tryDeleteCard(byte day, byte month, byte card);

	 // Create a planning entry
	 bool createPlanningEntry(byte day, byte month, byte boat, byte timeextend,byte Q1, byte Q2, byte Q3, byte Q4);

	 // Delete a planning entry
	 bool deletePlanningEntry(byte day, byte month, byte boat, byte timeextend);

	 // Get a planning entry
	 void getPlanningEntry(byte day, byte month, byte * buffer);

	 // Clean a planning entry
	 void cleanPlanningEntry();

	 // Delete 7 days before today of planning entry
	 void deleteLastWeek(byte day, byte month);

	 // Read one month planning
	 void discoverPlanning(byte day, byte month, String * buffer);

	 // Check planning and unlock a door if valid card at the right time
	 bool checkPlanning(RtcDateTime now, byte Q1, byte Q2, byte Q3, byte Q4, byte & door);
};
#endif