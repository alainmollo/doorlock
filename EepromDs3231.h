// EepromDs3231.h
#ifndef _EEPROMDS3231_h
#define _EEPROMDS3231_h

#include "Arduino.h"

//I2C Slave Address  
const uint8_t DS3231_EEPROM_ADDRESS = 0x57;

class EepromDs3231Class
{
 protected:

 public:
	 static void i2c_eeprom_write_byte(int deviceaddress, unsigned int eeaddress, byte data);

	 // WARNING: address is a page address, 6-bit end will wrap around
	 // also, data can be maximum of about 30 bytes, because the Wire library has a buffer of 32 bytes
	 static void i2c_eeprom_write_page(int deviceaddress, unsigned int eeaddresspage, byte* data, byte length);

	 static byte i2c_eeprom_read_byte(int deviceaddress, unsigned int eeaddress);

	 // maybe let's not read more than 30 or 32 bytes at a time!
	 static void i2c_eeprom_read_buffer(int deviceaddress, unsigned int eeaddress, byte *buffer, int length);
};
#endif