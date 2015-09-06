// core.h
// This file constitutes the core functions that run the scheduling for the SmartGarden system.
// Initial code by: Richard Zimmerman
// Copyright (c) 2013 Richard Zimmerman
//
// Modifications for multi-station and SmartGarden system by Tony-osp
//

// enable logging support
#define LOGGING 1

#ifndef _CORE_h
#define _CORE_h

#ifdef ARDUINO
#include "nntp.h"
#endif
#include <inttypes.h>
#include "port.h"
#include "settings.h"
#ifdef LOGGING
#include "sdlog.h"
extern Logging sdlog;
#endif

#ifndef VERSION
#define VERSION "SG 1.5"
#endif

// We are using upper nibble for status, and lower nibble as a timer for transitional states

#define ZONE_STATE_OFF			0
#define ZONE_STATE_STARTING		0x010
#define ZONE_STATE_STOPPING		0x020
#define ZONE_STATE_RUNNING		0x0F0

// This is the default remote communication timeout, seconds
// We use this value for "starting"/"stopping" timer stored in the lower nibble of the zone state
#define	ZONE_STATE_TIMEOUT		5	

void mainLoop();
void ClearEvents();
void LoadSchedTimeEvents(int8_t sched_num, bool bQuickSchedule = false);
void ReloadEvents(bool bAllEvents = false);
uint8_t GetZoneState(uint8_t iNum);
void TurnOnZone(uint8_t zone);
void TurnOffZones();
void io_setup();
int ActiveZoneNum(void);

bool GetNextEvent(uint8_t *pSchedID, uint8_t *pZoneID, short *pTime);


class runStateClass
{
public:
	class DurationAdjustments {
	public:
		DurationAdjustments() : seasonal(-1), wunderground(-1) {}
		DurationAdjustments(int16_t val) : seasonal(val), wunderground(val) {}
		int16_t seasonal;
		int16_t wunderground;
	};
public:
	runStateClass();
	void SetSchedule(bool val, int8_t iSchedNum = -1, const runStateClass::DurationAdjustments * adj = 0);
	void ContinueSchedule(int8_t zone, short endTime);
	void SetManual(bool val, int8_t zone = -1);
	bool isSchedule()
	{
		return m_bSchedule;
	}
	bool isManual()
	{
		return m_bManual;
	}
	int8_t getZone()
	{
		return m_zone;
	}
	short getEndTime()
	{
		return m_endTime;
	}
	int8_t getSchedule()
	{
		return m_iSchedule;
	}

	void TurnOnZone(uint8_t nZone, uint8_t ttr);
	void TurnOffZone(uint8_t nZone);
	void TurnOffZones();
	void RemoteStopAllZones(void);
	bool RemoteStartZone(int iSchedule, uint8_t stationID, uint8_t channel, uint8_t time2run );
	bool StartZone(int iSchedule, uint8_t stationID, uint8_t channel, uint8_t time2run );
	void TurnOffZonesWorker();
	bool StartZoneWorker(int iSchedule, uint8_t stationID, uint8_t channel, uint8_t time2run );

	void ReportZoneStatus(uint8_t stationID, uint8_t channel, uint8_t z_status);
	void ReportStationZonesStatus(uint8_t stationID, uint8_t z_status);

	time_t	sLastContactTime[MAX_STATIONS];

private:
	void LogSchedule();
	bool m_bSchedule;
	bool m_bManual;
	int8_t m_iSchedule;
	int8_t m_zone;
	short m_endTime;
	time_t m_eventTime;
	DurationAdjustments m_adj;

};

extern runStateClass runState;
extern nntp nntpTimeServer;

#endif

