/*
  Settings handling for Local UI on Remote station on SmartGarden


Copyright 2014 tony-osp (http://tony-osp.dreamwidth.org/)

Licensed under the Apache License, Version 2.0 (the "License");
you may not use this file except in compliance with the License.
You may obtain a copy of the License at

    http://www.apache.org/licenses/LICENSE-2.0

Unless required by applicable law or agreed to in writing, software
distributed under the License is distributed on an "AS IS" BASIS,
WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
See the License for the specific language governing permissions and
limitations under the License.

*/

#include "settings.h"
#include "port.h"
#include <string.h>
#include <stdlib.h>
#include "sysreset.h"
#include "localUI.h"




void ResetEEPROM()
{
		const char * const sHeader = EEPROM_SHEADER;

        trace(F("Reseting EEPROM\n"));

        for (int i = 0; i <= 3; i++)			// write current signature
                EEPROM.write(i, sHeader[i]);

		SetXBeeFlags(NETWORK_FLAGS_ENABLED);
		SetXBeeAddr(NETWORK_XBEE_DEFAULTADDRESS);
		SetXBeePANID(NETWORK_XBEE_DEFAULT_PANID);
		SetXBeePortSpeed(NETWORK_XBEE_DEFAULT_SPEED);
		SetXBeePort(NETWORK_XBEE_DEFAULT_PORT);
		SetXBeeChan(NETWORK_XBEE_DEFAULT_CHAN);
		SetMyStationID(DEFAULT_STATION_ID);
		SetMaxTtr(DEFAULT_TTR);
		SetNumZones(MAX_ZONES);

		localUI.lcd_print_line_clear_pgm(PSTR("EEPROM reloaded"), 0);
		localUI.lcd_print_line_clear_pgm(PSTR("Rebooting..."), 1);
		delay(2000);

		sysreset();
}


bool IsFirstBoot()
{
		const char * const sHeader = EEPROM_SHEADER;

		if ((EEPROM.read(0) == sHeader[0]) && (EEPROM.read(1) == sHeader[1]) && (EEPROM.read(2) == sHeader[2]) && (EEPROM.read(3) == sHeader[3])){

			return false;
		}

        return true;
}

bool IsXBeeEnabled(void)
{
	return (EEPROM.read(ADDR_NETWORK_XBEE_FLAGS) & NETWORK_FLAGS_ENABLED) ? true:false;
}

uint8_t GetXBeeChan(void)
{
	return EEPROM.read(ADDR_NETWORK_XBEE_CHAN);
}

uint8_t GetXBeePort(void)
{
	return EEPROM.read(ADDR_NETWORK_XBEE_PORT);
}
uint16_t GetXBeePortSpeed(void)
{
	uint16_t speed;

	speed = EEPROM.read(ADDR_NETWORK_XBEE_SPEED+1) << 8;
	speed += EEPROM.read(ADDR_NETWORK_XBEE_SPEED);

	return speed;
}

uint16_t GetXBeePANID(void)
{
	uint16_t panID;

	panID = EEPROM.read(ADDR_NETWORK_XBEE_PANID+1) << 8;
	panID += EEPROM.read(ADDR_NETWORK_XBEE_PANID);

	return panID;
}

uint16_t GetXBeeAddr(void)
{
	uint16_t addr;

	addr = EEPROM.read(ADDR_NETWORK_XBEE_ADDR16+1) << 8;
	addr += EEPROM.read(ADDR_NETWORK_XBEE_ADDR16);

	return addr;
}

void SetXBeeFlags(uint8_t flags)
{
	EEPROM.write(ADDR_NETWORK_XBEE_FLAGS, flags);
}


void SetXBeeChan(uint8_t chan)
{
	EEPROM.write(ADDR_NETWORK_XBEE_CHAN, chan);
}

void SetXBeePort(uint8_t port)
{
	EEPROM.write(ADDR_NETWORK_XBEE_PORT, port);
}

void SetXBeePortSpeed(uint16_t speed)
{
	uint8_t speedh = (speed & 0x0FF00) >> 8;
	uint8_t speedl = speed & 0x0FF;

	EEPROM.write(ADDR_NETWORK_XBEE_SPEED, speedl);
	EEPROM.write(ADDR_NETWORK_XBEE_SPEED+1, speedh);
}

void SetXBeePANID(uint16_t panID)
{
	uint8_t panIDh = (panID & 0x0FF00) >> 8;
	uint8_t panIDl = panID & 0x0FF;

	EEPROM.write(ADDR_NETWORK_XBEE_PANID, panIDl);
	EEPROM.write(ADDR_NETWORK_XBEE_PANID+1, panIDh);
}

void SetXBeeAddr(uint16_t addr)
{
	uint8_t addrh = (addr & 0x0FF00) >> 8;
	uint8_t addrl = addr & 0x0FF;

	EEPROM.write(ADDR_NETWORK_XBEE_ADDR16, addrl);
	EEPROM.write(ADDR_NETWORK_XBEE_ADDR16+1, addrh);
}




uint8_t GetMyStationID(void)
{
	return EEPROM.read(ADDR_STATION_ID);
}

void SetMyStationID(uint8_t stationID)
{
	EEPROM.write(ADDR_STATION_ID, stationID);
}


uint8_t GetMaxTtr(void)
{
	return EEPROM.read(ADDR_MAX_TTR);
}

void SetMaxTtr(uint8_t ttr)
{
	EEPROM.write(ADDR_MAX_TTR, ttr);
}

uint8_t GetNumZones(void)
{
	return EEPROM.read(ADDR_NUM_ZONES);
}


uint8_t GetPumpChannel(void)
{
	return EEPROM.read(ADDR_PUMP_CHANNEL);
}


void SetNumZones(uint8_t numZones)
{
	EEPROM.write(ADDR_NUM_ZONES, numZones);
}


void SetPumpChannel(uint8_t pumpChannel)
{
	EEPROM.write(ADDR_PUMP_CHANNEL, pumpChannel);
}

uint8_t GetXBeeFlags(void)
{
	return EEPROM.read(ADDR_NETWORK_XBEE_FLAGS);
}
