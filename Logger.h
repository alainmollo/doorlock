/*
Logger.h

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

#ifndef _LOGGER_h
#define _LOGGER_h

// Set this flag to define active level
// 0 => No Log
// 1 => Error Log
// 2 => Verbose Log
#define ACTIVELOG 2

#if defined(ARDUINO) && ARDUINO >= 100
#include "Arduino.h"
#else
#include "WProgram.h"
#endif

class LoggerClass
{
protected:
	// Generic log output method
	void Output(char *  logMessage);

	// Generic log output method
	void Output(String  logMessage);

	// Carriage return
	void linefeed();
public:
	// Verbose log with char* input
	void Log(char *, bool);

	// Verbose log with char input
	void Log(char, bool);

	// Verbose log with String input
	void Log(String, bool);

	// Verbose log with char* input
	void Log(char *);

	// Verbose log with char input
	void Log(char);

	// Verbose log with String input
	void Log(String);

	// Error log with char* input
	void Error(char *, bool);

	// Error log with char input
	void Error(char, bool);

	// Error log with String input
	void Error(String, bool);

	// Error log with char* input
	void Error(char *);

	// Error log with char input
	void Error(char);

	// Error log with String input
	void Error(String);

	// logging mode state
	bool IsLogging();
};

extern LoggerClass Logger;
#endif