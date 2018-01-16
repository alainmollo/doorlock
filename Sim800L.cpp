/*
  Sim800L.cpp

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

#include "Arduino.h"
#include "Logger.h"
#include "Sim800L.h"

//================================================================================

bool Sim800L::smsReceived = false;

// Base constructor
Sim800L::Sim800L(void)
	: SoftwareSerial(SIM800_RX_PIN_DEFAULT, SIM800_TX_PIN_DEFAULT)
{
	_ResetPin = RESET_PIN;
	_InterruptPin = SIM800_INT_PIN_DEFAULT;
	SetUp(SIM800_BAUD_RATE_DEFAULT, SIM800_RX_PIN_DEFAULT);
}

// Constructor with capabilities to set rx and tx pin's
Sim800L::Sim800L(uint8_t receivePin, uint8_t transmitPin)
	: SoftwareSerial(receivePin, transmitPin)
{
	_ResetPin = RESET_PIN;
	_InterruptPin = SIM800_INT_PIN_DEFAULT;
	SetUp(SIM800_BAUD_RATE_DEFAULT, receivePin);
}

// Constructor with capabilities to set rx and tx pin's
Sim800L::Sim800L(uint8_t receivePin, uint8_t transmitPin, uint8_t resetPin)
	: SoftwareSerial(receivePin, transmitPin)
{
	_ResetPin = resetPin;
	_InterruptPin = SIM800_INT_PIN_DEFAULT;
	SetUp(SIM800_BAUD_RATE_DEFAULT, receivePin);
}

// Constructor with capabilities to set rx and tx pin's
Sim800L::Sim800L(uint8_t receivePin, uint8_t transmitPin, uint8_t resetPin, uint8_t interruptPin)
	: SoftwareSerial(receivePin, transmitPin)
{
	_ResetPin = resetPin;
	_InterruptPin = interruptPin;
	SetUp(SIM800_BAUD_RATE_DEFAULT, receivePin);
}

// Constructor with capabilities to set rx and tx pin's and also baud rate
Sim800L::Sim800L(uint8_t receivePin, uint8_t transmitPin, uint8_t resetPin, uint8_t interruptPin, long baudrate)
	: SoftwareSerial(receivePin, transmitPin)
{
	_ResetPin = resetPin;
	_InterruptPin = interruptPin;
	SetUp(baudrate, receivePin);
}

// Private factoring method SetUp
void Sim800L::SetUp(long baudrate, uint8_t receivePin)
{
	this->moduleBoot = false;
	_ReceivePin = receivePin;

	this->begin(baudrate);

	// reserve memory to prevent intern fragmention
	_buffer.reserve(255);

	// set reset pin as output and low level
	pinMode(_ResetPin, OUTPUT);
	pinMode(_InterruptPin, INPUT);
	digitalWrite(_ResetPin, 0);
}

// Module initialization
bool Sim800L::Init(void)
{
	bool result = true;

	Logger.Log(F("Sim800 reset..."));

	// Hardware reset Sim800L module
	result &= this->reset();

	// Wait 1 s after reseting module
	delay(1000);

	// Set Sim800 led off for low consumption
	if (result)
		Logger.Log(F("Sim800 set baudrate."));

	// Set sim800 serial interface baud rate
	if (result)
	{
		this->sendAtPlusCommand("IPR=9600", true);
		result &= this->waitOK();
	}

	Logger.Log(F("Sim800 autoCellRTC..."));

	if (result)
	{
		this->sendAtCommand("E0", true);
		result &= this->waitOK();
	}

	// Set SIM800L internal clock to adjust to network time
	if (result)
		result &= this->setAutoCellRTC();

	Logger.Log(F("Sim800 save settings..."));

	// Save sim800 eeprom settings
	if (result)
		result &= this->saveAllSettings();

	if (result)
	{
		// Attach ring hardware interruption for SIM800L notification
		attachInterrupt(digitalPinToInterrupt(_InterruptPin), ringinterrupt, CHANGE);

		// Read gsm level signal
		this->levelSignal = this->waitSignal();

		// Set moduleReady flag to true
		this->moduleBoot = true;
	}

	return result;
}

// Send command to module (optimized)
void Sim800L::sendCommand(String & command, bool cr = true)
{
	ESP.wdtFeed();

	Logger.Log(F("sendCommand : "), false);
	Logger.Log(command);

	this->print(command);
	delay(50);
	carriageReturn(cr);
}

// Send command to module (optimized)
void Sim800L::sendCommand(char command, bool cr = true)
{
	String Command(command);
	sendCommand(Command, cr);
}

// Send command to module (optimized)
void Sim800L::sendCommand(char * command, bool cr = true)
{
	String Command(command);
	sendCommand(Command, cr);
}

// Send command to module (optimized)
void Sim800L::sendCommand(const char * command, bool cr = true)
{
	String Command(command);
	sendCommand(Command, cr);
}

// Send command to module (optimized)
String Sim800L::sendWaitCommand(String & command)
{
	ESP.wdtFeed();

	Logger.Log(F("sendCommand : "), false);
	Logger.Log(command);

	this->print(command);
	delay(50);
	carriageReturn(true);

	return _readSerial();
}

// Send command to module (optimized)
String Sim800L::sendWaitCommand(char command)
{
	String Command(command);
	return sendWaitCommand(Command);
}

// Send command to module (optimized)
String Sim800L::sendWaitCommand(char * command)
{
	String Command(command);
	return sendWaitCommand(Command);
}

// Send command to module (optimized)
String Sim800L::sendWaitCommand(const char * command)
{
	String Command(command);
	return sendWaitCommand(Command);
}

// Send carriage return on demande
void Sim800L::carriageReturn(bool cr)
{
	if (cr)
	{
		this->print(F("\r\n"));
		delay(50);
	}
}

// Send AT command to module (optimized)
void Sim800L::sendAtCommand(const char * command, bool cr = true)
{
	this->print(F("AT"));
	delay(50);
	sendCommand(command, cr);
}

// Send AT+ command to module (optimized)
void Sim800L::sendAtPlusCommand(const char * command, bool cr = true)
{
	this->print(F("AT+"));
	delay(50);
	sendCommand(command, cr);
}

// Private method for reading serial data incomming from Sim800L after a command
String Sim800L::_readSerial(void)
{
	int timeout = 0;
	while (!this->available() && timeout++ < MAX_TIME_OUT)
	{
		delay(10);
	}
	if (this->available())
	{
		_buffer = this->readString();

		Logger.Log(F("Sim800 answer:"), false);
		Logger.Log(_buffer);

		return _buffer;
	}
	else
	{
		Logger.Log(F("reading timeout !"));

		_buffer = F("");
		return _buffer;
	}
}

// Return serial buffer after command
String Sim800L::readSerialbuffer()
{
	return _buffer;
}

// Return serial buffer after command
const char * Sim800L::readSerial()
{
	return _buffer.c_str();
}

// no Wait for communication with reset buffer
void Sim800L::noWait(void)
{
	int timeout = 0;
	while (!this->available() && timeout++ < MAX_TIME_OUT)
	{
		delay(10);
	}
	_buffer = F("");
}

// Wait OK response
bool Sim800L::waitOK(void)
{
	return waitResponse("OK");
}

// Wait xxx response
bool Sim800L::waitResponse(const char * response)
{
	Logger.Log(F("Waiting for response:"), false);
	Logger.Log(response);

	int counter = 0;
	while (_readSerial().indexOf(response) == -1 && counter++ < RESPONSE_COUNT_OUT)
	{
		delay(10);

		Logger.Log(F("loop wait response:"), false);
		Logger.Log(String(counter, DEC));
	}

	if (_buffer.indexOf(response) != -1)
	{
		Logger.Log(F("Response is OK"));

		return true;
	}
	else
	{
		Logger.Log(F("Response time out"));

		return false;
	}
}

// Waiting for good quality signal received
uint8_t Sim800L::waitSignal(void)
{
	uint8_t counter;
	uint8_t sig;
	while ((sig = this->signalQuality()) == 0)
	{
		Logger.Log(F("Wait for signal ..."));

		delay(1000);
		if (counter++ > 30)
		{
			this->reset();
			counter = 0;
		}
	}
	return sig;
}

// Public initialize method
bool Sim800L::reset(void)
{
	digitalWrite(_ResetPin, 1);
	delay(1000);
	digitalWrite(_ResetPin, 0);
	delay(1000);

	Logger.Log(F("Wait Sim800  ready ..."));

	// wait for ready answer ...
	// return waitResponse(F("+CIEV")); work with orange but not with SFR
	return waitResponse("SMS Ready");
}

// Put module into sleep mode
bool Sim800L::sleepMode(void)
{
	sendAtPlusCommand("CSCLK=1");
	return waitOK();
}

// Put module into power down mode
bool Sim800L::powerDownMode(void)
{
	sendAtPlusCommand("CPOWD=1");
	return waitOK();
}

// Put device in phone functionality mode
bool Sim800L::setPhoneFunctionality(uint8_t mode)
{
	sendAtPlusCommand("CFUN=", false);
	String * tmp = new String(mode, DEC);
	sendCommand(*tmp);
	delete tmp;
	return waitOK();
}

// Check signal quality
uint8_t Sim800L::signalQuality(void)
{
	sendAtPlusCommand("CSQ");

	String signal = _readSerial();
	return signal.substring(_buffer.indexOf(F("+CSQ: ")) + 6, _buffer.indexOf(F("+CSQ: ")) + 8).toInt();
}

// Send a Sms method
bool Sim800L::sendSms(char * number, char * text)
{
	String Number(number);
	String Text(text);

	this->sendSms(Number, Text);
}

// Send a Sms method
bool Sim800L::sendSms(String & number, String & text)
{
	sendAtPlusCommand("CMGF=1");
	noWait();

	sendAtPlusCommand("CMGS=\"", false);
	sendCommand(number, false);
	sendCommand("\"");

	noWait();

	sendCommand(text);

	noWait();
	sendCommand((char)26);

	noWait();
	return true;
}

// Private method for jump in buffer posititon
void Sim800L::nextBuffer(void)
{
	_buffer = _buffer.substring(_buffer.indexOf(F("\",\"")) + 2);
}

// Get an indexed Sms
Sim800L::smsReader * Sim800L::readSms(uint8_t index)
{
	// Put module in SMS text mode
	sendAtPlusCommand("CMGF=1");

	// If no error ...
	if ((_readSerial().indexOf(F("ER"))) == -1)
	{
		// Read the indexed SMS
		sendAtPlusCommand("CMGR=", false);
		String * tmp = new String(index, DEC);
		sendCommand(*tmp);
		delete tmp;

		// Read module response
		_readSerial();

		// If there were an sms
		if (_buffer.indexOf(F("CMGR:")) != -1)
		{
			nextBuffer();
			String whoSend = _buffer.substring(1, _buffer.indexOf(F("\",\"")));
			nextBuffer();
			nextBuffer();
			nextBuffer();
			String whenSend = _buffer.substring(0, _buffer.indexOf(F("\"")));
			nextBuffer();
			if (_buffer.length() > 10) //avoid empty sms
			{
				_buffer = _buffer.substring(_buffer.indexOf(F("\"")) + 3);
				smsReader * result = new smsReader();
				result->WhoSend = whoSend;
				result->WhenSend = whenSend;
				result->Message = _buffer;

				// Return the sms
				Logger.Log("who = " + whoSend);
				Logger.Log("when = " + whenSend);
				Logger.Log(F("sms reading:"), false);
				Logger.Log(_buffer);

				return result;
			}
			else
				return nullptr;
		}
		else
			return nullptr;
	}
	else
		return nullptr;
}

// Delete Indexed Sms method
bool Sim800L::delSms(uint8_t index)
{
	sendAtPlusCommand("CMGD=", false);
	String * tmp = new String(index, DEC);
	sendCommand(*tmp, false);
	delete tmp;
	sendCommand(",0");
	return waitOK();
}

// Delete all Sms method
bool Sim800L::delAllSms(void)
{
	sendAtPlusCommand("CMGDA=\"DEL ALL\"");
	return waitOK();
}

// Get Rtc internal Timer in decimal and string format
void Sim800L::RTCtime(String * retstr, dateTime * result = nullptr)
{
	sendAtPlusCommand("CCLK?");
	_readSerial();

	if ((_buffer.indexOf(F("ERR"))) == -1)
	{
		_buffer = _buffer.substring(_buffer.indexOf(F("\"")) + 1, _buffer.lastIndexOf(F("\"")) - 1);

		uint8_t year = _buffer.substring(0, 2).toInt();
		uint8_t month = _buffer.substring(3, 5).toInt();
		uint8_t day = _buffer.substring(6, 8).toInt();
		uint8_t hour = _buffer.substring(9, 11).toInt();
		uint8_t minute = _buffer.substring(12, 14).toInt();
		uint8_t second = _buffer.substring(15, 17).toInt();

		if (result != nullptr)
		{
			result->year = year;
			result->month = month;
			result->day = day;
			result->hour = hour;
			result->minute = minute;
			result->second = second;
		}

		*retstr = String(day, DEC) + "/" + String(month, DEC) + "/" + String(year, DEC) + "," +
			String(hour, DEC) + ":" + String(minute, DEC) + ":" + String(second, DEC);
	}
}

// Setup Sim800L to automatic rtc setup from cellular network
bool Sim800L::setAutoCellRTC(void)
{
	return AutoCellRTC("+CLTS: 0", "CLTS=1", "+CLTS: 1");
}

// Setup Sim800L to automatic rtc setup from cellular network
bool Sim800L::resetAutoCellRTC(void)
{
	return AutoCellRTC("+CLTS: 1", "CLTS=0", "+CLTS: 0");
}

// Set or Reset Sim800L to automatic rtc setup from cellular network
bool Sim800L::AutoCellRTC(const char * command1, const char * command2, const char * command3)
{
	sendAtPlusCommand("CLTS?");

	_readSerial();
	if (_buffer.indexOf(command1) != -1)
	{
		sendAtPlusCommand(command2);
		_readSerial();

		sendAtPlusCommand("CLTS?");
		_readSerial();

		if (_buffer.indexOf(command3) != -1)
		{
			saveAllSettings();
			reset();
			return true;
		}
		else
			return false;
	}
	else
		return true;
}

// Save All settings in memory
bool Sim800L::saveAllSettings(void)
{
	sendAtCommand("&W");
	return waitOK();
}

// Change state of module led flash
bool Sim800L::setOnLedFlash(void)
{
	sendAtPlusCommand("CNETLIGHT=1");
	return waitOK();
}

// Change state of module led flash
bool Sim800L::setOffLedFlash(void)
{
	sendAtPlusCommand("CNETLIGHT=0");
	return waitOK();
}

uint8_t Sim800L::getReceivePin(void)
{
	return _ReceivePin;
}

// Read SMS arrived and launch SMS Treatment
bool Sim800L::ReadSMSTreatment(const smsTreatFunction & smsTreatCommand)
{
	// Check Sim800L not sleeping !
	this->sendAtCommand("");
	if (!this->waitOK())
	{
		// Reset Sim800L and make setup cycle to refresh Sim800L
		this->reset();
		return false;
	}
	else
	{
		int i = 1;
		smsReader * SMS;
		while ((SMS = this->readSms(i)) != nullptr)
		{
			this->delSms(i++);
			(*smsTreatCommand)(SMS);
			delete SMS;
		}
	}
	return true;
}

// Return rf level signal
uint8_t Sim800L::getLevelSignal(bool readForce = false)
{
	if (readForce)
		this->levelSignal = this->waitSignal();
	return this->levelSignal;
}


// Return if module is ready
bool Sim800L::checkBootOk(void)
{
	return this->moduleBoot;
}

// Check if SMS was arrived and set flag to false
bool Sim800L::checkSMS(void)
{
	if (this->available())
	{
		String sim800data = this->readString();

		Logger.Log(F("checkSMS:"), false);
		Logger.Log(sim800data);



		if (sim800data.indexOf(F("+CMTI:")) != -1 || smsReceived)
		{
			smsReceived = false;
			return true;
		}
	}
	return false;
}

// Sim800L hardwware ring signal interruption, just to wake up arduino when SMS received
void Sim800L::ringinterrupt(void)
{
	smsReceived = true;
}