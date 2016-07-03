/*
  core.h 

  This module is intended to be used with my multi-station environment monitoring and sprinklers control system (SmartGarden).

Creative Commons Attribution-ShareAlike 3.0 license
Copyright 2015 tony-osp (http://tony-osp.dreamwidth.org/)

*/

#ifndef _CORE_h
#define _CORE_h

#ifdef ARDUINO
#include "nntp.h"
#endif
#include <inttypes.h>
#include "port.h"
#include "settings.h"
#include "sdlog.h"
extern Logging sdlog;

// Zone state cache.
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
void ReloadEvents(bool bAllEvents = false);
uint8_t GetZoneState(uint8_t iNum);
//void TurnOnZone(uint8_t zone);
//void TurnOffZones();
void io_setup();
int ActiveZoneNum(void);

bool GetNextEvent(uint8_t *pSchedID, uint8_t *pZoneID, short *pTime);


// Core runState class. This class is handling schedules, starting/stopping zones etc.
//

class runStateClass
{
public:
	runStateClass();
	void		StartSchedule(bool fQuickSched, int8_t iSchedNum = 100);
	void		StopSchedule(void);
	void		ProcessScheduledEvents();

	int8_t getZone()
	{
		if( m_iZone == -2 ) return -2;
		else				return m_iZone+1;
	}
	short getRemainingTime()
	{
		if( m_iZone == -2 ) return max(int(SG_DELAY_BETWEEN_ZONES/1000ul) - int((millis()-m_startZoneMillis)/1000ul), 0);
		else				return max(m_zoneMins*60 - int((millis()-m_startZoneMillis)/1000ul), 0);
	}
	int8_t getSchedule()
	{
		return m_iSchedule;
	}
	bool isSchedule()
	{
		if( m_iSchedule != -1 ) return true;
		else					return false;
	}
	short getRemainingPauseTime()
	{
		if( m_endPauseMillis == 0 ) return 0;

		int  rt = max(int((m_endPauseMillis-millis())/60000ul), 0);	
		if( rt == 0 )							// inline recalculation and correction of the paused state
			m_endPauseMillis = 0;

		return rt;
	}
	bool isPaused()
	{
		if( getRemainingPauseTime() != 0 )	return true;
		else								return false;
	}

	void TurnOnZone(uint8_t nZone, uint8_t ttr);
	void TurnOffZone(uint8_t nZone);
	void TurnOffZones();
	void RemoteStopAllZones(void);
	bool RemoteStartZone(int iSchedule, uint8_t stationID, uint8_t channel, uint8_t time2run );
	bool StartZone(int iSchedule, uint8_t stationID, uint8_t channel, uint8_t time2run );
	void TurnOffZonesWorker();
	bool StartZoneWorker(int iSchedule, uint8_t stationID, uint8_t channel, uint8_t time2run );

	void SetPause(int time2pause);

	void ReportZoneStatus(uint8_t stationID, uint8_t channel, uint8_t z_status);
	void ReportStationZonesStatus(uint8_t stationID, uint8_t z_status);

	time_t	sLastContactTime[MAX_STATIONS];
	int16_t	iLastReceivedRSSI[MAX_STATIONS];

private:
	void		LogSchedule();
	void		LogEvent();
	uint8_t		sAdj(uint8_t val);

	int8_t		m_iSchedule;		// Currently running schedule ID, or -1 if no schedules are running
	int8_t		m_iZone;			// Currently running zone ID, or -1 if no zones are active
									// Note: Another special state value is -2, this state means (few seconds) delay between zones in a schedule currently in progress
									//		 When m_iZone==-2, m_iNextZone will have the ID of the next zone that will be started after the pause

	int8_t		m_iNextZone;		// When m_iZone==-2 (short delay between zones in a schedule), m_iNextZone will have the ID of the next zone to run

	uint32_t	m_startZoneMillis;	// millis() reading when zone started, or when delay between zones in a schedule started
	uint8_t		m_zoneMins;			// number of minutes to run
	uint32_t	m_startSchedMillis;

	int			m_wuScale;			// weather forecast correction factor, 100% by default

	uint32_t	m_endPauseMillis;	// if non-zero, indicates we are in pause mode, and this field has millis value for the end of the pause period

	uint32_t	m_iWaterUsed;
};

extern runStateClass runState;
extern nntp nntpTimeServer;

#endif

