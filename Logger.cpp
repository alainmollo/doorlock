// Logging class redirecting to serial port with log level

#include "Logger.h"

// Verbose log with char* input
void LoggerClass::Log(char * logMessage)
{
	Log(* logMessage, true);
}

void LoggerClass::Log(char * logMessage, bool cr)
{
#if defined(ARDUINO) && ACTIVELOG == 2
	Output(logMessage);
	if (cr)
		linefeed();
#endif
}

// Verbose log with char input
void LoggerClass::Log(char logMessage)
{
	Log(logMessage, true);
}

void LoggerClass::Log(char logMessage, bool cr)
{
#if defined(ARDUINO) && ACTIVELOG == 2
	Output(&logMessage);
	if (cr)
		linefeed();
#endif
}

// Verbose log with String input
void LoggerClass::Log(String logMessage)
{
	Log(logMessage, true);
}

void LoggerClass::Log(String logMessage, bool cr)
{
#if defined(ARDUINO) && ACTIVELOG == 2
	Output(logMessage);
	if (cr)
		linefeed();
#endif
}

// Error log with char* input
void LoggerClass::Error(char * logMessage)
{
	Error(* logMessage, true);
}

void LoggerClass::Error(char * logMessage, bool cr)
{
#if defined(ARDUINO) && ACTIVELOG >= 1 
	Output(F("!!"));
	Output(logMessage);
	if (cr)
		linefeed();
#endif
}

// Error log with char input
void LoggerClass::Error(char logMessage)
{
	Error(logMessage, true);
}

void LoggerClass::Error(char logMessage, bool cr)
{
#if defined(ARDUINO) && ACTIVELOG >= 1 
	Output(F("!!"));
	Output(&logMessage);
	if (cr)
		linefeed();
#endif
}

// Error log with String input
void LoggerClass::Error(String logMessage)
{
	Error(logMessage, true);
}

void LoggerClass::Error(String logMessage, bool cr)
{
#if defined(ARDUINO) && ACTIVELOG >= 1
	Output(F("!!"));
	Output(logMessage);
	if (cr)
		linefeed();
#endif
}

// Generic log output method
void LoggerClass::Output(char *  logMessage)
{
	Serial.print(logMessage);
	delay(10);
}

// Generic log output method
void LoggerClass::Output(String logMessage)
{
	Serial.print(logMessage);
	delay(10);
}

// Carriage return
void LoggerClass::linefeed()
{
	Serial.println();
	delay(10);
}

bool LoggerClass::IsLogging()
{
#if defined(ARDUINO) && ACTIVELOG >= 1
	return true;
#else
	return false;
#endif
}

LoggerClass Logger;