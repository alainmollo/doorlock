#include "CommandManager.h"

// Send AT Command to Sim800
// SIM800@AT+CSQ?END#xxx
bool CommandManagerClass::SENDATCOMMAND(String * Message, String * Who, uint8_t From)
{
	int ix = 0;
	if (Message->charAt(ix) == '@')
	{
		Logger.Log(F("Send AT Command to Sim800:"), false);

		ix++;
		String * addr = new String();
		while (ix < Message->length())
		{
			*addr += Message->charAt(ix);
			ix++;
		}

		Logger.Log(*addr);

		String result = Sim800->sendWaitCommand(addr->c_str());
		return ReplyToSender(result, Who, From);
	}
	else
		return false;
}