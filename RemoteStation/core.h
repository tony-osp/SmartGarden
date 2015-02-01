// core.cpp
// This file provides necessary minimal core functions required for RemoteStation operation
// Author: Tony-osp
// Copyright (c) 2014 Tony-osp
//

// enable logging support
#define LOGGING 1

#ifndef _CORE_h
#define _CORE_h

#include <inttypes.h>
#include "port.h"
#ifdef LOGGING
#include "sdlog.h"
extern Logging sdlog;
#endif

#ifndef VERSION
#define VERSION "1.0.1"
#endif

#define	DEFAULT_HARDWARE_VERSION	1
#define 	DEFAULT_FIRMWARE_VERSION		1

// default maximum watering run duration (in minutes)
#define	DEFAULT_MAX_DURATION				99	

void mainLoop();
bool isZoneOn(int8_t iNum);
bool TurnOnZone(uint8_t iValve);
bool TurnOffZone(uint8_t iValve);
void TurnOffZones();
void io_setup();
int8_t ActiveZoneNum(void);

#define ZONE_FLAGS_ENABLED		1	// this zone is enabled
#define ZONE_FLAGS_INDEPENDENT	2	// this zone is scheduled independently (e.g. it is a pump or something like that
									// zones that are independent are ignored when deciding whether to start next scheduled zone
#define ZONE_FLAGS_ACTIVE		4	// this zone is currently On	
#define ZONE_FLAGS_MANUAL		8	// flag indicating whether the zone was started manually or remotely/via schedule

#define ZONE_FLAGS_RUNTIME	(ZONE_FLAGS_ACTIVE | ZONE_FLAGS_ENABLED)

struct zone_struct
{
    uint8_t			flags;		
	int				iSchedule;		// schedule number, valid when the valve was started remotely (schedule# is useful for logs correlation)
	
	time_t			tStartTime;		// zone start time (if already started)
	uint8_t			runTime;		// requested time to run, minutes
	unsigned long 	endMillis;
};

class runStateClass
{
public:

	bool	StartZone(bool bManual, int iSchedule, int8_t iValve, uint8_t time2run );
	void	StopAllZones(void);
	bool	RemoteStartZone(bool bManual, int iSchedule, int8_t iValve_in, uint8_t time2run );
	void	RemoteStopAllZones(void);

	void	begin(void);


	void LogSchedule(int iValve, time_t eventTime, bool bManual, int iSchedule );


private:
	void LogSchedule(void);
	
	int			 max_time2run;	// maximum time to run a zone, seconds
};

extern uint8_t		LastReceivedStationID;
extern uint8_t		LastReceivedRssi;
extern uint32_t		LastReceivedTime;


inline uint8_t GetLastReceivedStationID(void)
{
	return LastReceivedStationID;
}

inline uint8_t GetLastReceivedRssi(uint8_t stationID)
{
	return LastReceivedRssi;
}

inline uint32_t GetLastReceivedTime(uint8_t stationID)
{
	return LastReceivedTime;
}

inline void SetLastReceivedRssi(uint8_t stationID, uint8_t rssi)
{
	LastReceivedStationID = stationID;
	LastReceivedRssi = rssi;
	LastReceivedTime = millis();
}


extern runStateClass runState;

#endif

