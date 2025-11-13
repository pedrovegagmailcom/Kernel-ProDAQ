#include "Arduino.h"
#include "LS7366.h"
#include "SPI.h"

/*
   LS7366.cpp - Library for using the LSI/CSI LS7366 quadrature encoder counter with serial interface
   Created by David Jabon
   License: GPL 3.0
   
   The count values that are returned are always unsigned longs (32 bit unsigned integers) .
   The code is set up so that it is easy to change to longs instead.  To change to longs, you need to change all
   declarations of "unsigned long"  to "long" and return
   left_extend_MSB(value to exend) in both read_counter and read_OTR.  The reason for this is that the
   MSB needs to be left extended in 1, 2, and 3 bit modes and this helper function takes care of that.
*/

#define CS_pin PA_0C

LS7366::LS7366()
{
	
		
}

void LS7366::begin()
{
	
	
	pinMode(PA_0C,OUTPUT);
	digitalWrite(PA_0C, HIGH);
	datawidth = 4; //Datawidth of 4 is the default
	
	
	clear_status_register();    // STR
  clear_mode_register_0();    // MDR0
  clear_mode_register_1();    // MDR1
  clear_counter();            // CNTR

  // Configura MDR0: X4, free-run, index deshabilitado, filtro /2 (si usas cristal)
  byte mdr0 = QUADRX4 | FREE_RUN | DISABLE_INDX | FILTER_2;
  write_mode_register_0(mdr0);

  // Configura MDR1: habilitar conteo, 4 bytes, sin flags
  byte mdr1 = EN_CNTR | BYTE_4 | NO_FLAGS;
  write_mode_register_1(mdr1);

  // Cargar contador desde DTR = 0 (por si quieres asegurar arranque en 0)
  write_data_register(0);
  load_counter();
	
}


void LS7366::clear_mode_register_0()
{
	SPI.beginTransaction(SPISettings(2000000, MSBFIRST, SPI_MODE0));

	digitalWrite(CS_pin, LOW);
	SPI.transfer(CLR_MDR0);
	digitalWrite(CS_pin, HIGH);
	
	SPI.endTransaction();
}


void LS7366::clear_mode_register_1()
{
	SPI.beginTransaction(SPISettings(2000000, MSBFIRST, SPI_MODE0));

	digitalWrite(CS_pin, LOW);
	SPI.transfer(CLR_MDR1);
	digitalWrite(CS_pin, HIGH);

	SPI.endTransaction();
}


void LS7366::clear_counter()
{
	SPI.beginTransaction(SPISettings(2000000, MSBFIRST, SPI_MODE0));

	digitalWrite(CS_pin, LOW);
	SPI.transfer(CLR_CNTR);
	digitalWrite(CS_pin, HIGH);

	SPI.endTransaction();
}


void LS7366::clear_status_register()
{
	SPI.beginTransaction(SPISettings(2000000, MSBFIRST, SPI_MODE0));

	digitalWrite(CS_pin, LOW);
	SPI.transfer(CLR_STR);
	digitalWrite(CS_pin, HIGH);

	SPI.endTransaction();
}


byte LS7366::read_mode_register_0()
{
	SPI.beginTransaction(SPISettings(2000000, MSBFIRST, SPI_MODE0));

	digitalWrite(CS_pin, LOW);
	SPI.transfer(READ_MDR0);
	byte return_value = SPI.transfer(0x00);
	digitalWrite(CS_pin, HIGH);

	SPI.endTransaction();
	return return_value;
}

byte LS7366::read_mode_register_1()
{
	SPI.beginTransaction(SPISettings(2000000, MSBFIRST, SPI_MODE0));

	digitalWrite(CS_pin, LOW);
	SPI.transfer(READ_MDR1);
	byte return_value = SPI.transfer(0x00);
	digitalWrite(CS_pin, HIGH);

	SPI.endTransaction();
	return return_value;
}

unsigned long LS7366::read_counter()
{
	unsigned long return_value=0;
	byte bytes_to_read = datawidth;
	byte val;

	SPI.beginTransaction(SPISettings(2000000, MSBFIRST, SPI_MODE0));
	
	digitalWrite(CS_pin, LOW);
	SPI.transfer(READ_CNTR);
	while(bytes_to_read > 0)
	{
	val = SPI.transfer(0x00);
	bytes_to_read--;
	return_value = (return_value << 8) | val; 
	}
	digitalWrite(CS_pin, HIGH);

	SPI.endTransaction();
	return return_value;
}

unsigned long LS7366::read_OTR()
{
	unsigned long return_value=0;
	byte bytes_to_read = datawidth;
	byte val;

	SPI.beginTransaction(SPISettings(2000000, MSBFIRST, SPI_MODE0));
	
	digitalWrite(CS_pin, LOW);
	SPI.transfer(READ_OTR);
	while(bytes_to_read > 0)
	{
	val = SPI.transfer(0x00);
	bytes_to_read--;
	return_value = (return_value << 8) | val; 
	}
	digitalWrite(CS_pin, HIGH);

	SPI.endTransaction();
	return return_value;
}
	
	
	
byte LS7366::read_status_register()
{
	SPI.beginTransaction(SPISettings(2000000, MSBFIRST, SPI_MODE0));

	digitalWrite(CS_pin, LOW);
	SPI.transfer(READ_STR);
	byte return_value = SPI.transfer(0x00);
	digitalWrite(CS_pin, HIGH);

	SPI.endTransaction();
	return return_value;
}


void LS7366::write_mode_register_0(byte val)
{
	SPI.beginTransaction(SPISettings(2000000, MSBFIRST, SPI_MODE0));

	digitalWrite(CS_pin, LOW);
	SPI.transfer(WRITE_MDR0);
	SPI.transfer(val);
	digitalWrite(CS_pin, HIGH);

	SPI.endTransaction();
}


void LS7366::write_mode_register_1(byte val)
{
	SPI.beginTransaction(SPISettings(2000000, MSBFIRST, SPI_MODE0));

	digitalWrite(CS_pin, LOW);
	SPI.transfer(WRITE_MDR1);
	SPI.transfer(val);
	digitalWrite(CS_pin, HIGH);
	//set datawidth property
	datawidth = 0x04 - (0x03 & val);    //Note that 0x00 is 4 byte mode, 0x01 is 3 byte mode, etc, so we need to subtract from 4.

	SPI.endTransaction();
}

void LS7366::write_data_register(unsigned long val)
{
	unsigned long value_to_write = val;
	byte i = 0;
	
	SPI.beginTransaction(SPISettings(2000000, MSBFIRST, SPI_MODE0));

	digitalWrite(CS_pin, LOW);
	SPI.transfer(WRITE_DTR);
	for (i=0;i<datawidth;i++)
	{
		value_to_write = (byte)(val >> (8*(datawidth -1 - i)));  //IDEA: e.g. when datawidth = 4, shift 24  first, then 16, then 8 then 0.
		SPI.transfer(value_to_write);
	}
	digitalWrite(CS_pin, HIGH);

	SPI.endTransaction();
}

void LS7366::load_counter()
{
	SPI.beginTransaction(SPISettings(2000000, MSBFIRST, SPI_MODE0));

	digitalWrite(CS_pin, LOW);
	SPI.transfer(LOAD_CNTR);
	digitalWrite(CS_pin, HIGH);

	SPI.endTransaction();
}

void LS7366::load_OTR()
{
	SPI.beginTransaction(SPISettings(2000000, MSBFIRST, SPI_MODE0));

	digitalWrite(CS_pin, LOW);
	SPI.transfer(LOAD_OTR);
	digitalWrite(CS_pin, HIGH);

	SPI.endTransaction();
}


long LS7366::left_extend_MSB(long val)
{
	long value_to_return = 0;
	long MSB;
	if (datawidth == 4 )    //Nothing to do in 4 byte mode
	{
		value_to_return = val; 
	}
	else 
	{
		MSB = (val >> (datawidth*8 - 1)) & 0x0001 ;
		if (MSB == 0)       //If the MSB is 0, there is nothing to extend.
		{
			value_to_return = val;
		}
		else
		{
			switch (datawidth) {
				case 1:
				  value_to_return = 0xFFFFFF00 | val;
				  break;
				case 2:
				  value_to_return = 0xFFFF0000 | val;
				  break;
				case 3:
				  value_to_return = 0xFF000000 | val;
				  break;
				default: 
				  // do nothing
				break;
			  }
			  
		}
	}

  return value_to_return;
}
