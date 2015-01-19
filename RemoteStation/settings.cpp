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




static const char * const sHeader = "R1.0";
void ResetEEPROM()
{
        trace(F("Reseting EEPROM\n"));
}


bool IsFirstBoot()
{
        return false;
}

bool IsXBeeEnabled(void)
{
	return NETWORK_XBEE_DEFAULT_ENABLED;
}

uint8_t GetXBeePort(void)
{
	return NETWORK_XBEE_DEFAULT_PORT;
}

uint16_t GetXBeePortSpeed(void)
{
	return NETWORK_XBEE_DEFAULT_SPEED;
}

uint16_t GetXBeeAddr(void)
{
	return NETWORK_XBEE_ADDRESS;		// temporary, until EEPROM is hooked up
}

uint16_t GetXBeePANID(void)
{
	return NETWORK_XBEE_DEFAULT_PANID;
}

uint8_t GetXBeeChan(void)
{
	return NETWORK_XBEE_DEFAULT_CHAN;
}

uint8_t GetXBeeFlags(void)
{
	return NETWORK_FLAGS_ENABLED;
}

void SetXBeeFlags(uint8_t flags)
{
	;
}

uint8_t GetStationID(void)
{
	return DEFAULT_STATION_ID;
}

uint8_t GetMaxTtr(void)
{
	return DEFAULT_TTR;
}