/*

Settings management implementation for SmartGarden system. This module handles EEPROM-based settings, including settings Get/Set
and settings/hardware config import from an INI file.

Settings management implementation for SmartGarden system. This module handles EEPROM-based settings, including settings Get/Set
and settings/hardware config import from an INI file.

Most of the code is written by Tony-osp (http://tony-osp.dreamwidth.org/)

Portions came from sprinklers_pi code by Richard Zimmerman (schedule handling is lifted from sprinklers_avr code)


*/


#include <inttypes.h>
#include "core.h"
#include "web.h"
#include "port.h"
#include "Defines.h"


#ifndef _SETTINGS_h
#define _SETTINGS_h


// XBee RF network

#define NETWORK_FLAGS_ENABLED		1	// 1 - indicates that the network is enabled (config)
#define NETWORK_FLAGS_ON			2	// 1 - indicates that the network is running (runtime state)

class Schedule
{
private:
	uint8_t m_type;
public:
	union
	{
		uint8_t day;
		uint8_t interval;
	};
	char name[20];
	short time[4];
	uint8_t zone_duration[MAX_ZONES];
	Schedule();
	bool IsEnabled() const { return m_type & 0x01; }
	bool IsInterval() const { return m_type & 0x02; }
	bool IsWAdj() const { return m_type & 0x04; }
	void SetEnabled(bool val) { m_type = val ? (m_type | 0x01) : (m_type & ~0x01); }
	void SetInterval(bool val) { m_type = val ? (m_type | 0x02) : (m_type & ~0x02); }
	void SetWAdj(bool val) { m_type = val ? (m_type | 0x04) : (m_type & ~0x04); }
};

struct FullZone
{
	bool bEnabled :1;
	bool bPump :1;
	uint8_t	stationID;
	uint8_t channel;

	char name[20];
};

struct ShortZone
{
	bool bEnabled :1;
	bool bPump :1;
	uint8_t	stationID;
	uint8_t channel;
};

//	Sensor definition structure. This structure reflects sensor definition in EEPROM.
//
//	Note:	Sensors are identified by SensorID, which is essentially the slot in the Sensors list. SensorIDs are numbered from zero. 
//			Maximum SensorID is defined by the hardware,
//			and for Arduino Mega - based hardware it is 9, giving us total of 10 Sensors.
struct FullSensor
{
	uint8_t		sensorType;				// Sensor type - Temp, Humidity etc
	uint8_t		flags;					// Sensor flags. Unused flag bits should be set to zero.
	uint8_t		sensorStationID;		// StationID
	uint8_t		sensorChannel;			// Sensor Channel this particular sensor is connected to on that Station.

	char name[MAX_SENSOR_NAME_LENGTH];
};

struct ShortSensor
{
	uint8_t		sensorType;				// Sensor type - Temp, Humidity etc
	uint8_t		flags;					// Sensor flags. Unused flag bits should be set to zero.
	uint8_t		sensorStationID;		// StationID
	uint8_t		sensorChannel;			// Sensor Channel this particular sensor is connected to on that Station.
};


//	Station definition structure. This structure reflects station definition in EEPROM.
//
//	Note:	Stations are identified by StationID, which is essentially the slot in the Stations list. StationID are numbered from zero,
//			with StationID of zero being the Master controller itself (by convention). Maximum StationID is defined by the hardware,
//			and for Arduino Mega - based hardware it is 15, giving us total of 16 Stations, including Master controller.
struct FullStation
{
	uint8_t		stationFlags;			// Station flags. Things like enabled/disabled etc.
	uint8_t		networkID;				// ID of the network this station is connected to - local, XBee, nRFL, other
	uint8_t		numZoneChannels;		// number of watering channels connected to the station
	uint8_t		startZone;				// Start zone# mapped to this station
										// Note: this assumes that zones are sequential within station mapping!

	uint16_t	networkAddress;			// XBee (or other) network address, 16 bit

	uint8_t		numTempSensors;			// number of temperature sensors
	uint8_t		numHumiditySensors;		// number of humidity sensors
	uint8_t		numPressureSensors;		// number of pressure sensors
	uint8_t		numWaterflowSensors;	// number of waterflow sensors

	char		name[20];				// station name
};


// Short version of the Station structure. Useful for quick load/lookup of the key Station fields.
struct ShortStation
{
	uint8_t		stationFlags;			// Station flags. Things like enabled/disabled etc.
	uint8_t		networkID;				// ID of the network this station is connected to - local, XBee, nRFL, other
	uint8_t		numZoneChannels;		// number of watering channels connected to the station
	uint8_t		startZone;				// Start zone# mapped to this station
										// Note: this assumes that zones are sequential within station mapping!

	uint16_t	networkAddress;			// XBee (or other) network address, 16 bit
};

#define NETWORK_ID_LOCAL_PARALLEL	0	// hardware connection to the master controller - direct Positive or Negative, or OpenSprinkler
#define NETWORK_ID_LOCAL_SERIAL		1	// hardware connection to the master controller - OpenSprinkler
#define NETWORK_ID_XBEE				10	// XBee RF

#define STATION_FLAGS_VALID  		1	// 1 - indicates that this Station structure is filled in and valid
#define STATION_FLAGS_ENABLED		2	// 1 - indicates that this Station is enabled
#define STATION_FLAGS_RSTATUS		4	// 1 - indicates that this station status could be queried remotely (via RF network)
#define STATION_FLAGS_RCONTROL		8	// 1 - indicates that this station status could be queried remotely (via RF network)

#define DEFAULT_STATION_NETWORK_TYPE	NETWORK_ID_LOCAL_PARALLEL
#define DEFAULT_STATION_NETWORK_ADDRESS	0
#define DEFAULT_STATION_NUM_ZONES		8


// Serial IO (OpenSprinkler style) pin assignment
struct SrIOMapStruct
{
	uint8_t SrClkPin;
	uint8_t	SrNoePin;
	uint8_t	SrDatPin;
	uint8_t	SrLatPin;
};

////////////////////
//  EEPROM Getter/Setters
void SetNumSchedules(uint8_t iNum);
uint8_t GetNumSchedules();
void SetNTPOffset(const int8_t value);
int8_t GetNTPOffset();
IPAddress GetNTPIP();
void SetNTPIP(const IPAddress & value);
IPAddress GetIP();
void SetIP(const IPAddress & value);
IPAddress GetNetmask();
void SetNetmask(const IPAddress & value);
IPAddress GetGateway();
void SetGateway(const IPAddress & value);
IPAddress GetWUIP();
void SetWUIP(const IPAddress & value);
uint32_t GetZip();
void SetZip(const uint32_t zip);
void GetApiKey(char * key);
void SetApiKey(const char * key);
bool GetRunSchedules();
void SetRunSchedules(bool value);
bool GetDHCP();
void SetDHCP(const bool value);
enum EOT {OT_NONE, OT_DIRECT_POS, OT_DIRECT_NEG, OT_OPEN_SPRINKLER};
EOT GetOT();
void SetOT(EOT oType);
uint16_t GetWebPort();
void SetWebPort(uint16_t);
uint8_t GetSeasonalAdjust();
void SetSeasonalAdjust(uint8_t);
void GetPWS(char * key);
void SetPWS(const char * key);
bool GetUsePWS();
void SetUsePWS(bool value);
void LoadSchedule(uint8_t num, Schedule * pSched);
void LoadZone(uint8_t num, FullZone * pZone);
void LoadShortZone(uint8_t index, ShortZone * pZone);

uint16_t GetEvtMasterFlags(void);
uint8_t  GetEvtMasterStationID(void);
void SetEvtMasterFlags(uint16_t flags);
void SetEvtMasterStationID(uint8_t stationID);
uint8_t GetMyStationID(void);
void SetMyStationID(uint8_t stationID);


// IO maps and hardware channels

void LoadZoneIOMap(uint8_t *ptr);
void SaveZoneIOMap(uint8_t *ptr);
uint8_t GetDirectIOPin(uint8_t n);
void SetNumIOChannels(uint8_t nchannels);
uint8_t GetNumIOChannels(void);
void SetNumOSChannels(uint8_t nchannels);
uint8_t GetNumOSChannels(void);
void LoadSrIOMap(SrIOMapStruct *ptr);
void SaveSrIOMap(SrIOMapStruct *ptr);

// Zones access

uint8_t GetNumZones(void);
uint8_t GetPumpStation(void);
uint8_t GetPumpChannel(void);
void SetNumZones(uint8_t numZones);
void SetPumpStation(uint8_t pumpStation);
void SetPumpChannel(uint8_t pumpChannel);
bool IsPumpEnabled(void);
int GetNumEnabledZones();

// Stations

void LoadStation(uint8_t num, FullStation *pStation);
void LoadShortStation(uint8_t num, ShortStation *pStation);
void SaveStation(uint8_t num, FullStation *pStation);
void SaveShortStation(uint8_t num, ShortStation *pStation);
uint8_t GetNumStations(void);

// Sensors
void LoadSensor(uint8_t num, FullSensor *pSensor);
void LoadShortSensor(uint8_t num, ShortSensor *pSensor);
void SaveSensor(uint8_t num, FullSensor * pSensor);
void SaveShortSensor(uint8_t num, ShortSensor * pSensor);
uint8_t GetNumSensors(void);
void SetNumSensors(uint8_t numSensors);


// XBee RF

uint8_t GetXBeeFlags(void);
bool IsXBeeEnabled(void);
uint8_t GetXBeeChan(void);
uint8_t GetXBeePort(void);
uint16_t GetXBeePortSpeed(void);
uint16_t GetXBeePANID(void);
uint16_t GetXBeeAddr(void);
void SetXBeeFlags(uint8_t flags);
void SetXBeeChan(uint8_t chan);
void SetXBeePort(uint8_t port);
void SetXBeePortSpeed(uint16_t speed);
void SetXBeePANID(uint16_t panID);
void SetXBeeAddr(uint16_t addr);


// KV Pairs Setters
bool SetSchedule(const KVPairs & key_value_pairs);
bool SetZones(const KVPairs & key_value_pairs);
bool DeleteSchedule(const KVPairs & key_value_pairs);
bool SetSettings(const KVPairs & key_value_pairs);

// Misc
bool IsFirstBoot();
void ResetEEPROM();
void 	ResetEEPROM_NoSD(uint8_t  defStationID);

// For storing info related to the Quick Schedule
extern Schedule quickSchedule;

#endif

