/*
  Sim800L.h

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

#ifndef Sim800L_h
#define Sim800L_h

#include <SoftwareSerial.h>

//================================================================================
//================================================================================
//  Sim800L

// SIM800 RX and TX default pins
#define SIM800_RX_PIN_DEFAULT 3
#define SIM800_TX_PIN_DEFAULT 4
#define SIM800_INT_PIN_DEFAULT 2

// SIM800 Reset pin
#define RESET_PIN 5

// SIM800 Default baud rate
#define SIM800_BAUD_RATE_DEFAULT 9600

// SIM800 Max time out for response
#define MAX_TIME_OUT 1000

// Max response await
#define RESPONSE_COUNT_OUT 10

// Set debug mode
//#define DEBUG_MODE_SET

// Sim800L module class driver
class Sim800L : public SoftwareSerial
{
private:
	// Keep last gsm level signal
	uint8_t levelSignal;

	// Flag for detect if SMS was received
	static bool smsReceived;

	// Ready state for sim module
	bool moduleBoot;

	// Interrupt function for ring signal detection
	static void ringinterrupt(void);

	// Private Data buffer
	String _buffer;

	// Inner class receive pin datastore
	uint8_t _ReceivePin;

	// Inner class reset pin datastore
	uint8_t _ResetPin;

	// Inner class reset pin datastore
	uint8_t _InterruptPin;

	// Private method for reading serial data incomming from Sim800L after a command
	String _readSerial(void);

	// no Wait for communication with reset buffer
	void noWait(void);

	// Private factoring method SetUp
	void SetUp(long, uint8_t);

	// Send carriage return on demande
	void carriageReturn(bool);

	// Private method for jump in buffer posititon
	void nextBuffer(void);

	// Set or Reset Sim800L to automatic rtc setup from cellular network
	bool AutoCellRTC(const char *, const char *, const char *);
public:
	struct smsReader
	{
		String WhoSend;
		String WhenSend;
		String Message;
	};

	struct dateTime
	{
		uint8_t day;
		uint8_t month;
		uint8_t year;
		uint8_t hour;
		uint8_t minute;
		uint8_t second;
	};

	// sms received and treatment function Prototype
	typedef void(*smsTreatFunction)(smsReader * smsCommand); // type for conciseness
	
	// Base constructor
	Sim800L(void);

	// Constructor with capabilities to set rx and tx pin's
	Sim800L(uint8_t, uint8_t);

	// Constructor with capabilities to set rx and tx pin's
	Sim800L(uint8_t, uint8_t, uint8_t);

	// Constructor with capabilities to set rx and tx pin's
	Sim800L(uint8_t, uint8_t, uint8_t, uint8_t);

	// Constructor with capabilities to set rx and tx pin's and also baud rate
	Sim800L(uint8_t, uint8_t, uint8_t, uint8_t, long);

	// Send command to module (optimized)
	void sendCommand(String &, bool);

	// Send command to module (optimized)
	void sendCommand(char, bool);

	// Send command to module (optimized)
	void sendCommand(char *, bool);

	// Send command to module (optimized)
	void sendCommand(const char *, bool);

	// Send command to module (optimized)
	String sendWaitCommand(String &);

	// Send command to module (optimized)
	String sendWaitCommand(char);

	// Send command to module (optimized)
	String sendWaitCommand(char *);

	// Send command to module (optimized)
	String sendWaitCommand(const char *);

	// Send AT command to module (optimized)
	void sendAtCommand(const char *, bool);

	// Send AT+ command to module (optimized)
	void sendAtPlusCommand(const char *, bool);

	// Wait OK response
	bool waitOK(void);

	// Wait xxx response
	bool waitResponse(const char *);

	// Waiting for good quality signal received
	uint8_t waitSignal(void);

	// Public initialize method
	bool reset(void);

	// Put module into power down mode
	bool powerDownMode(void);

	// Put module into sleep mode
	bool sleepMode(void);

	// Put device in phone functionality mode
	bool setPhoneFunctionality(uint8_t);

	// Check signal quality
	uint8_t signalQuality(void);

	// Send a Sms method
	bool sendSms(char *, char *);

	// Send a Sms method
	bool sendSms(String &, String &);

	// Get an index Sms
	smsReader * readSms(uint8_t);

	// Delete Indexed Sms method
	bool delSms(uint8_t index);

	// Delete all Sms method
	bool delAllSms(void);

	// Get Rtc internal Timer in decimal and string format
	void RTCtime(String *, dateTime *);

	// Setup Sim800L to automatic rtc setup from cellular network
	bool setAutoCellRTC(void);

	// Setup Sim800L to non automatic rtc setup from cellular network
	bool resetAutoCellRTC(void);

	// Save All settings in memory
	bool saveAllSettings(void);

	// Change state of module led flash
	bool setOnLedFlash(void);
	bool setOffLedFlash(void);

	// Return the receive (interrupt) pin choice for software communication
	uint8_t getReceivePin(void);

	// Module initialization
	bool Init(void);

	// Check if module is ready
	bool checkBootOk(void);

	// Return rf level signal
	uint8_t getLevelSignal(bool);

	// Check if SMS was arrived
	bool checkSMS(void);

	// Read SMS arrived and launch SMS Treatment
	bool ReadSMSTreatment(const smsTreatFunction &);

	// Receive SMS Datas from Sim800
	//void ReceiveSMSData(const smsReceiveData &);

	// Return serial buffer after command
	String readSerialbuffer();

	// Return serial char pointer to sim buffer
	const char * readSerial(void);
};
#endif