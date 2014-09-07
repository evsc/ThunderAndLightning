/*
  AS3935.cpp - AS3935 Franklin Lightning Sensor™ IC by AMS library
  Copyright (c) 2012 Raivis Rengelis (raivis [at] rrkb.lv). All rights reserved.

  This library is free software; you can redistribute it and/or
  modify it under the terms of the GNU Lesser General Public
  License as published by the Free Software Foundation; either
  version 3 of the License, or (at your option) any later version.

  This library is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
  Lesser General Public License for more details.

  You should have received a copy of the GNU Lesser General Public
  License along with this library; if not, write to the Free Software
  Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA  02110-1301  USA
*/

#include "AS3935.h"

AS3935::AS3935(byte (*SPItransfer)(byte),int csPin, int irq)
{
	SPITransferFunc = SPItransfer;
	_CSPin = csPin;
	_IRQPin = irq;
	digitalWrite(_CSPin,HIGH);
	pinMode(_CSPin,OUTPUT);
	pinMode(_IRQPin,INPUT);
}

byte AS3935::_SPITransfer2(byte high, byte low)
{
	digitalWrite(_CSPin,LOW);
	SPITransferFunc(high);
	byte regval = SPITransferFunc(low);
	digitalWrite(_CSPin,HIGH);
	return regval;	
}

byte AS3935::_rawRegisterRead(byte reg)
{
	return _SPITransfer2((reg & 0x3F) | 0x40, 0);
}

byte AS3935::_ffsz(byte mask)
{
	byte i = 0;
	if (mask)
		for (i = 1; ~mask & 1; i++)
			mask >>= 1;
	return i;
}

void AS3935::registerWrite(byte reg, byte mask, byte data)
{
	byte regval = _rawRegisterRead(reg);
	regval &= ~(mask);
	if (mask)
		regval |= (data << (_ffsz(mask)-1));
	else
		regval |= data;
	_SPITransfer2(reg & 0x3F, regval);
}

byte AS3935::registerRead(byte reg, byte mask)
{
	byte regval = _rawRegisterRead(reg);
	regval = regval & mask;
	if (mask)
		regval >>= (_ffsz(mask)-1);
	return regval;
}

void AS3935::reset()
{
	_SPITransfer2(0x3C, 0x96);
	delay(2);
}


int AS3935::tuneAntenna(byte tuneCapacitor)
{
	unsigned long setUpTime;
	int currentcount = 0;
	int currIrq, prevIrq;

	registerWrite(AS3935_LCO_FDIV,0);
	registerWrite(AS3935_DISP_LCO,1);

	registerWrite(AS3935_TUN_CAP,tuneCapacitor);
	delay(2);
	prevIrq = digitalRead(_IRQPin);
	setUpTime = millis() + 100;
	while((long)(millis() - setUpTime) < 0)
	{
		currIrq = digitalRead(_IRQPin);
		if (currIrq > prevIrq)
		{
			currentcount++;	
		}
		prevIrq = currIrq;
	}

	registerWrite(AS3935_TUN_CAP,tuneCapacitor);
	delay(2);
	registerWrite(AS3935_DISP_LCO,0);
	// and now do RCO calibration
	powerUp();

	return currentcount;
}

int AS3935::getIRQ()
{
	return digitalRead(_IRQPin);
}

bool AS3935::calibrate()
{
	int target = 3125, currentcount = 0, bestdiff = INT_MAX, currdiff = 0;
	byte bestTune = 0, currTune = 0;
	unsigned long setUpTime;
	int currIrq, prevIrq;
	// set lco_fdiv divider to 0, which translates to 16
	// so we are looking for 31250Hz on irq pin
	// and since we are counting for 100ms that translates to number 3125
	// each capacitor changes second least significant digit
	// using this timing so this is probably the best way to go
	registerWrite(AS3935_LCO_FDIV,0);
	registerWrite(AS3935_DISP_LCO,1);
	// tuning is not linear, can't do any shortcuts here
	// going over all built-in cap values and finding the best
	for (currTune = 0; currTune <= 0x0F; currTune++) 
	{
		registerWrite(AS3935_TUN_CAP,currTune);
		// let it settle
		delay(2);
		currentcount = 0;
		prevIrq = digitalRead(_IRQPin);
		setUpTime = millis() + 100;
		while((long)(millis() - setUpTime) < 0)
		{
			currIrq = digitalRead(_IRQPin);
			if (currIrq > prevIrq)
			{
				currentcount++;	
			}
			prevIrq = currIrq;
		}
		currdiff = target - currentcount;
		// don't look at me, abs() misbehaves
		if(currdiff < 0)
			currdiff = -currdiff;
		if(bestdiff > currdiff)
		{
			bestdiff = currdiff;
			bestTune = currTune;
		}
	}
	registerWrite(AS3935_TUN_CAP,bestTune);
	delay(2);
	registerWrite(AS3935_DISP_LCO,0);
	// and now do RCO calibration
	powerUp();
	// if error is over 109, we are outside allowed tuning range of +/-3.5%
	return bestdiff > 109?false:true;
}	


int AS3935::getBestTune()
{
	int target = 3125, currentcount = 0, bestdiff = INT_MAX, currdiff = 0;
	byte bestTune = 0, currTune = 0;
	unsigned long setUpTime;
	int currIrq, prevIrq;
	// set lco_fdiv divider to 0, which translates to 16
	// so we are looking for 31250Hz on irq pin (square wave)
	// and since we are counting for 100ms that translates to number 3125
	// each capacitor changes second least significant digit
	// using this timing so this is probably the best way to go
	registerWrite(AS3935_LCO_FDIV,0);
	registerWrite(AS3935_DISP_LCO,1);
	// tuning is not linear, can't do any shortcuts here
	// going over all built-in cap values and finding the best
	for (currTune = 0; currTune <= 0x0F; currTune++) 
	{
		registerWrite(AS3935_TUN_CAP,currTune);
		// let it settle
		delay(10);
		currentcount = 0;
		prevIrq = digitalRead(_IRQPin);
		setUpTime = millis() + 100;
		while((long)(millis() - setUpTime) < 0)
		{
			currIrq = digitalRead(_IRQPin);
			if (currIrq > prevIrq)
			{
				currentcount++;	
			}
			prevIrq = currIrq;
		}
		currdiff = target - currentcount;
		// don't look at me, abs() misbehaves
		if(currdiff < 0)
			currdiff = -currdiff;
		if(bestdiff > currdiff)
		{
			bestdiff = currdiff;
			bestTune = currTune;
		}
	}
	registerWrite(AS3935_TUN_CAP,bestTune);
	delay(2);
	registerWrite(AS3935_DISP_LCO,0);
	// and now do RCO calibration
	powerUp();
	// if error is over 109, we are outside allowed tuning range of +/-3.5%
	return bestTune;
}

void AS3935::powerDown()
{
	registerWrite(AS3935_PWD,1);
}

void AS3935::powerUp()
{
	registerWrite(AS3935_PWD,0);
	_SPITransfer2(0x3D, 0x96);
	delay(3);

	// Modify REG0x08[5] = 1
	registerWrite(AS3935_DISP_TRCO,1);
	delay(2);
	// Modify REG0x08[5] = 0
	registerWrite(AS3935_DISP_TRCO,0);
}

int AS3935::interruptSource()
{
	return registerRead(AS3935_INT);
}

void AS3935::disableDisturbers()
{
	registerWrite(AS3935_MASK_DIST,1);
}

void AS3935::enableDisturbers()
{
	registerWrite(AS3935_MASK_DIST,0);
}

int AS3935::getMinimumLightnings()
{
	return registerRead(AS3935_MIN_NUM_LIGH);
}

int AS3935::setMinimumLightnings(int minlightning)
{
	registerWrite(AS3935_MIN_NUM_LIGH,minlightning);
	return getMinimumLightnings();
}

int AS3935::lightningDistanceKm()
{
	return registerRead(AS3935_DISTANCE);
}

long AS3935::lightningEnergy()
{
	long v = 0;
	char bits8[4];

	// Energy_u e;
	// REG_u reg4, reg5, reg6;

	char err;

	bits8[3] = 0;
	bits8[2] = registerRead(AS3935_ENERGY_3);
	bits8[1] = registerRead(AS3935_ENERGY_2);
	bits8[0] = registerRead(AS3935_ENERGY_1);

	v = bits8[2]*65536 + bits8[1]*256 + bits8[0];

	return v;
}

void AS3935::setIndoors()
{
	registerWrite(AS3935_AFE_GB,AS3935_AFE_INDOOR);
}

void AS3935::setOutdoors()
{
	registerWrite(AS3935_AFE_GB,AS3935_AFE_OUTDOOR);
}

int AS3935::getNoiseFloor()
{
	return registerRead(AS3935_NF_LEV);
}

int AS3935::setNoiseFloor(int noisefloor)
{
	registerWrite(AS3935_NF_LEV,noisefloor);
	return getNoiseFloor();
}

int AS3935::getSpikeRejection()
{
	return registerRead(AS3935_SREJ);
}

int AS3935::setSpikeRejection(int srej)
{
	registerWrite(AS3935_SREJ, srej);
	return getSpikeRejection();
}

int AS3935::getWatchdogThreshold()
{
	return registerRead(AS3935_WDTH);
}

int AS3935::setWatchdogThreshold(int wdth)
{
	registerWrite(AS3935_WDTH,wdth);
	return getWatchdogThreshold();
}

void AS3935::clearStats()
{
	registerWrite(AS3935_CL_STAT,1);
	registerWrite(AS3935_CL_STAT,0);
	registerWrite(AS3935_CL_STAT,1);
}
