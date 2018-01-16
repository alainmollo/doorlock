#include "CommandManager.h"

// Create a one day planning entry for 1 reservation card 45 5A 32 AA
// Or Delete a one day planning entry for 1 reservation
// ENTRY@14,2,1,1:455A32AAEND#111
// DELTE@14,2,1,1END#111
bool CommandManagerClass::ENTRY(String * Message, String * Who, uint8_t From)
{
	bool delFlag = false;
	if (Message->startsWith(F("ENTRY")))
		AnalyseSms(Message, F("ENTRY"));
	else
	{
		delFlag = true;
		AnalyseSms(Message, F("DELTE"));
	}
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
		ix++;
		String * s2 = new String();
		while (ix < Message->length() && Message->charAt(ix) != ',')
		{
			*s2 += Message->charAt(ix);
			ix++;
		}
		ix++;
		String * s3 = new String();
		while (ix < Message->length() && Message->charAt(ix) != ',')
		{
			*s3 += Message->charAt(ix);
			ix++;
		}
		ix++;
		String * s4 = new String();
		while (ix < Message->length() && Message->charAt(ix) != ':')
		{
			*s4 += Message->charAt(ix);
			ix++;
		}

		byte deciDay = strtol(s1->c_str(), 0, 10);
		byte deciMonth = strtol(s2->c_str(), 0, 10);
		byte deciBoat = strtol(s3->c_str(), 0, 10);
		byte deciTime = strtol(s4->c_str(), 0, 10);
		delete s1;
		delete s2;
		delete s3;
		delete s4;

		bool ret;

		if (!delFlag)
		{
			String * s5 = new String();

			ix++;
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
			delete Qstr;
			delete x;
			delete s5;

			Logger.Log(String(Q1, HEX), false);
			Logger.Log(F(","), false);
			Logger.Log(String(Q2, HEX), false);
			Logger.Log(F(","), false);
			Logger.Log(String(Q3, HEX), false);
			Logger.Log(F(","), false);
			Logger.Log(String(Q4, HEX));

			ret = planning->createPlanningEntry(deciDay, deciMonth, deciBoat, deciTime, Q1, Q2, Q3, Q4);
		}
		else
		{
			ret = planning->deletePlanningEntry(deciDay, deciMonth, deciBoat, deciTime);
		}

		if (ret)
			return ReplyToSender(F("OK"), Who, From);
		else
			return ReplyToSender(F("KO"), Who, From);
	}
	else
		return false;
}