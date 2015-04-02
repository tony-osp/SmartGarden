/*
  Settings handling for Local UI on Remote station on SmartGarden


 Creative Commons Attribution-ShareAlike 3.0 license
 Copyright 2014 tony-osp (http://tony-osp.dreamwidth.org/)


*/

#ifndef _SETTINGS_h
#define _SETTINGS_h

// This simple Remote station may have up to 8 zones
#define MAX_ZONES 8
// This simple Remote station has only one Station defined in the config
#define MAX_STATIONS 1
// This Remote station may have up to 8 sensors
#define MAX_SENSORS	 8

// max number of stations on RF network
#define NETWORK_MAX_STATIONS 11


#include <inttypes.h>

#include "core.h"
#include "port.h"

#include "eepromMap.h"

#define EEPROM_SHEADER "R2.3"


#define NETWORK_XBEE_DEFAULT_ENABLED	true

#define NETWORK_XBEE_PANID_HIGH		0x15	// high byte of PAN ID is fixed.

#define NETWORK_XBEE_DEFAULT_PANID	0x90	// Low byte of PAN ID is configurable, this is the default.

#define NETWORK_XBEE_DEFAULT_CHAN	7
#define NETWORK_XBEE_DEFAULT_PORT	1
#define NETWORK_XBEE_DEFAULT_SPEED	57600

#define NETWORK_FLAGS_ENABLED		1	// 1 - indicates that the network is enabled (config)
#define NETWORK_FLAGS_ON			2	// 1 - indicates that the network is running (runtime state)

#define NETWORK_XBEE_DEFAULTADDRESS	2
#define DEFAULT_STATION_ID			1

#define DEFAULT_TTR					99
#define DEFAULT_MAX_OFFLINE_TDELTA  300000UL		// time (in milliseconds) since last time sync before indicator starts to show that the station is offline
													// 300,000ms is 5minutes.

#define	LOCAL_NUM_DIRECT_CHANNELS	4

////////////////////
//  EEPROM Getter/Setters


// Misc
bool IsFirstBoot();
void ResetEEPROM();

// XBee RF network
bool IsXBeeEnabled(void);
uint8_t GetXBeePort(void);
uint16_t GetXBeePortSpeed(void);
uint16_t GetXBeeAddr(void);
uint8_t GetXBeePANID(void);
uint8_t GetXBeeChan(void);
uint8_t GetXBeeFlags(void);
uint8_t GetMyStationID(void);
uint8_t GetMaxTtr(void);
uint8_t GetNumZones(void);
uint8_t GetPumpChannel(void);
uint8_t GetDirectIOPin(uint8_t n);
uint16_t GetEvtMasterFlags(void);
uint8_t  GetEvtMasterStationID(void);
uint16_t GetEvtMasterStationAddress(void);


void SetXBeeFlags(uint8_t flags);
void SetXBeeAddr(uint16_t addr);
void SetXBeePANID(uint8_t panID);
void SetXBeePortSpeed(uint16_t speed);
void SetXBeePort(uint8_t port);
void SetXBeeChan(uint8_t chan);
void SetXBeeFlags(uint8_t flags);
void SetMyStationID(uint8_t stationID);
void SetMaxTtr(uint8_t ttr);
void SetNumZones(uint8_t numZones);
void SetPumpChannel(uint8_t pumpChannel);
void SaveZoneIOMap(uint8_t *ptr);
void SetEvtMasterStationAddress(uint16_t addr);
void SetEvtMasterFlags(uint16_t flags);
void SetEvtMasterStationID(uint8_t stationID);


#endif

