/*
  core scheduling and run state class.


  Some of the design patterns came from Sprinklers_pi core by Richard Zimmerman.

Creative Commons Attribution-ShareAlike 3.0 license
Copyright 2014 tony-osp (http://tony-osp.dreamwidth.org/)

*/
#include "core.h"
#include "settings.h"
#include "Weather.h"
#include "web.h"
//#define TRACE_LEVEL			7		// trace everything for this module
#include "port.h"
#include "LocalBoard.h"
#include <stdlib.h>
#include "sensors.h"
#include "XBeeRF.h"
#include "localUI.h"
#include "RProtocolMS.h"
#ifdef ARDUINO
#include "tftp.h"
static tftp tftpServer;
#else
#include <wiringPi.h>
#include <unistd.h>
#endif



// Core modules

Logging sdlog;
static web webServer;
nntp nntpTimeServer;
runStateClass runState;

LocalBoardParallel	lBoardParallel;		// local hardware handler for Parallel-connected stations
LocalBoardSerial	lBoardSerial;		// local hardware handler for Serially-connected stations

static uint8_t	zoneStateCache[MAX_ZONES] = {0};

// Zone handler loop, it is called once a second

void zoneHandlerLoop(void)
{
	uint8_t  numZones = GetNumZones();
	register uint8_t  z;

	for( uint8_t i=0; i<numZones; i++ )
	{
		z = zoneStateCache[i];
		if( ( z != ZONE_STATE_OFF) && (z != ZONE_STATE_RUNNING) )
		{
			// this zone is in either "starting" or "stopping" state. Process the timer.

			if( z & ZONE_STATE_STARTING ) 
			{
				register uint8_t  t = z & 0x0F;
				if( t > 1 )
				{
					t--; 
					zoneStateCache[i] = t + ZONE_STATE_STARTING;
				}
				else
				{
					// we reached zero but have not received confirmation, assume that zone did not start.
					// stop the timer and change the state.
					zoneStateCache[i] = ZONE_STATE_OFF;
				}
			}
			else if( z & ZONE_STATE_STOPPING ) 
			{
				register uint8_t  t = z & 0x0F;
				if( t > 1 )
				{
					t--; 
					zoneStateCache[i] = t + ZONE_STATE_STOPPING;
				}
				else
				{
					// we reached zero but have not received confirmation, assume that zone stopped.
					// stop the timer and change the state.
					zoneStateCache[i] = ZONE_STATE_OFF;
				}
			}
		}
	}
}


uint8_t GetZoneState(uint8_t iNum)
{
	iNum--;		// adjust zone number (1 based vs 0 based)

	if( iNum >= GetNumZones() )
	{
		SYSEVT_ERROR(F("GetZoneState - wrong zone number %d"), (uint16_t)iNum);
		return ZONE_STATE_OFF;
	}

	return (zoneStateCache[iNum] & 0x0F0);		// return state only, suppress timer (lower nibble)
}

//	Returns active zone number.
//
//	Note: This funciton performs conversion. lBoard numbers channels from 0, with 255 meaning - nothing is running.
//		  However zones are numbered from 1, with -1 means - nothing is running.
//
int ActiveZoneNum(void)
{
		for( uint8_t i=0; i<GetNumZones(); i++ )
		{
			if( zoneStateCache[i] != ZONE_STATE_OFF )
				return i+1;
		}

		return -1;
}



void runStateClass::TurnOnZone(uint8_t nZone, uint8_t ttr)
{
        SYSEVT_CRIT(F("Turning On Zone %d"), nZone);

		nZone--;		// adjust zone number (1 based vs 0 based)

        if( nZone >= GetNumZones() )
		{
			SYSEVT_ERROR(F("TurnOnZone - invalid zone %d"), (uint16_t)nZone);
            return;
		}

        ShortZone zone;
        LoadShortZone(nZone, &zone);

		// sanity checks
		if( !zone.bEnabled ){

			SYSEVT_ERROR(F("TurnOnZone - error, zone %d is not enabled"), (uint16_t)nZone);
			return;
		}
		if( zone.stationID > MAX_STATIONS ){

			SYSEVT_ERROR(F("TurnOnZone - data corruption, StationID for zone %d is too high (%d)"), (uint16_t)nZone, (uint16_t)(zone.stationID));
			return;
		}

		// get zone Station and NetworkID (station type) for routing

		ShortStation	sStation;
		LoadShortStation(zone.stationID, &sStation);
		if( !(sStation.stationFlags & STATION_FLAGS_ENABLED) )
			return;													// Station is not enabled

		if( sStation.networkID == NETWORK_ID_LOCAL_PARALLEL )
		{
			if( lBoardParallel.ChannelOn(sStation.networkAddress+zone.channel) )
				zoneStateCache[nZone] = ZONE_STATE_RUNNING;	// parallel stations go directly to running state
			else
			{
				SYSEVT_ERROR(F("TurnOnZone - lBoardParallel returned failure for zone %d"), (uint16_t)nZone);
				return;
			}

			// Turn on the pump if necessary
//			lBoard.PumpControl(zone.bPump);
		}
		else if( sStation.networkID == NETWORK_ID_LOCAL_SERIAL )
		{
			if( lBoardSerial.ChannelOn(sStation.networkAddress+zone.channel) )
				zoneStateCache[nZone] = ZONE_STATE_RUNNING;	// serial stations go directly to running state
			else
			{
				SYSEVT_ERROR(F("TurnOnZone - lBoardSerial returned failure for zone %d"), (uint16_t)nZone);
				return;
			}

			// Turn on the pump if necessary
//			lBoard.PumpControl(zone.bPump);
		}
		else if( (sStation.networkID == NETWORK_ID_XBEE) || (sStation.networkID == NETWORK_ID_MOTEINORF) )
		{
			if( rprotocol.ChannelOn(zone.stationID, zone.channel, ttr) )
			{
				zoneStateCache[nZone] = ZONE_STATE_STARTING + ZONE_STATE_TIMEOUT;	// remote stations go to "starting" state first, and will transition to "running" state when response arrives
			}
			else
			{
				SYSEVT_ERROR(F("TurnOnZone - rprotocol.ChannelOn returned failure for zone %d"), (uint16_t)nZone);
				return;
			}

			// Turn on the pump if necessary
//			lBoard.PumpControl(zone.bPump);
		}
		else
		{
			SYSEVT_ERROR(F("TurnOnZone - unknown NetworkID for station %d, cannot turn On zone"), (uint16_t)nZone);
			return;
		}
}

void runStateClass::TurnOffZone(uint8_t nZone)
{
        SYSEVT_CRIT(F("Turning Off Zone %d"), nZone);

		nZone--;		// adjust zone number (1 based vs 0 based)

        if( nZone >= GetNumZones() )
		{
			SYSEVT_ERROR(F("TurnOffZone - invalid zone %d"), (uint16_t)nZone);
            return;
		}

        ShortZone zone;
        LoadShortZone(nZone, &zone);

		// sanity checks
		if( !zone.bEnabled ){

			SYSEVT_ERROR(F("TurnOffZone - error, zone %d is not enabled"), (uint16_t)nZone);
			return;
		}
		if( zone.stationID > MAX_STATIONS ){

			SYSEVT_ERROR(F("TurnOffZone - data corruption, StationID for zone %d is too high (%d)"), (uint16_t)nZone, (uint16_t)(zone.stationID));
			return;
		}

		// get zone Station and NetworkID (station type) for routing

		ShortStation	sStation;
		LoadShortStation(zone.stationID, &sStation);
		if( !(sStation.stationFlags & STATION_FLAGS_ENABLED) )
			return;													// Station is not enabled

		if( sStation.networkID == NETWORK_ID_LOCAL_PARALLEL )
		{
			lBoardParallel.ChannelOff( sStation.networkAddress+zone.channel );

			zoneStateCache[nZone] = ZONE_STATE_OFF;	

        // Turn on the pump if necessary
//			lBoard.PumpControl(zone.bPump);
		}
		else if( sStation.networkID == NETWORK_ID_LOCAL_SERIAL )
		{
			lBoardSerial.ChannelOff( sStation.networkAddress+zone.channel );

			zoneStateCache[nZone] = ZONE_STATE_OFF;	

			// Turn on the pump if necessary
//			lBoard.PumpControl(zone.bPump);
		}
		else if( (sStation.networkID == NETWORK_ID_XBEE) || (sStation.networkID == NETWORK_ID_MOTEINORF) )
		{
			if( rprotocol.ChannelOff(zone.stationID, zone.channel) )
			{
				zoneStateCache[nZone] = ZONE_STATE_STOPPING + ZONE_STATE_TIMEOUT;	// remote stations go to "stopping" state first, and will transition to "running" state when response arrives
			}
			else
			{
				SYSEVT_ERROR(F("TurnOffZone - rprotocol.ChannelOff returned failure for zone %d"), (uint16_t)nZone);
				return;
			}

			// Turn on the pump if necessary
//			lBoard.PumpControl(zone.bPump);
		}
		else
		{
			SYSEVT_ERROR(F("TurnOffZone - unknown NetworkID for station %d, cannot turn On zone"), (uint16_t)nZone);
			return;
		}
}

//
// Start individual zone. Technically it is done by creating quick schedule.
//
// Note: remote Start/Stop don't send report to EvtMaster (for now we are assuming remote control is coming form the master)
//
bool runStateClass::RemoteStartZone(int iSchedule, uint8_t stationID, uint8_t channel, uint8_t time2run )
{
// When starting zone remotely, we expect the initator (Master) to turn Off the zone,
// and the time2run specified in the request is used as a backstop, to ensure zone will not stay On if there is an interruption in comms,
// or if Master died etc
//
// To support this, we increase time2run just a bit (by 1 minute) and use this increased time as a backstop.
	return StartZoneWorker( iSchedule, stationID, channel, time2run+1 );
}

//
// Start individual zone. Technically it is done by creating quick schedule.
//
bool runStateClass::StartZone(int iSchedule, uint8_t stationID, uint8_t channel, uint8_t time2run )
{
	if( StartZoneWorker(iSchedule, stationID, channel, time2run) )
	{
#ifndef SG_STATION_MASTER	// we send remote master notifications only if this station is not a master by itself
		if( GetEvtMasterFlags() & EVTMASTER_FLAGS_REPORT_ZONES )
			rprotocol.SendZonesReport(0, stationID, GetEvtMasterStationID(), 0, GetNumZones());	// Note: this would work correctly only on Remote station, with only one station defined
#endif //SG_STATION_MASTER
		return true;
	}
	else
		return false;
}
//
// Start individual zone. Technically it is done by creating quick schedule.
//
// Note: this is a local worker for this operation. remote Start/Stop don't send report to EvtMaster (for now we are assuming remote control is coming form the master)
//
bool runStateClass::StartZoneWorker(int iSchedule, uint8_t stationID, uint8_t channel, uint8_t time2run )
{
		uint8_t		ch;
		{
			ShortStation	sStation;

			LoadShortStation(stationID, &sStation);

			if( !(sStation.stationFlags & STATION_FLAGS_ENABLED) || !(sStation.stationFlags & STATION_FLAGS_VALID) )
				return false;

			if( channel > sStation.numZoneChannels )
				return false;

			ch = sStation.startZone+channel;

			SYSEVT_VERBOSE(F("StartZoneWorker - st:%u,ch:%u,zone=%u\n"), uint16_t(stationID), uint16_t(channel), uint16_t(ch));
		}

		uint8_t	n_zones = GetNumZones();
        if( ch >= n_zones ) return	false;    // basic protection, ensure that required zone number is within acceptable range

        // So, we first end any schedule that's currently running 
		runState.StopSchedule();

        if( ActiveZoneNum() != -1 ){    // something is currently running, turn it off

                runState.TurnOffZonesWorker();
        }

        for( uint8_t n=0; n<n_zones; n++ ){

                quickSchedule.zone_duration[n] = 0;  // clear up QuickSchedule to zero out run time for all zones
        }

// set run time for required zone.
//
        quickSchedule.zone_duration[ch] = time2run;
        StartSchedule(true);
		ProcessScheduledEvents();

		return true;
}

void runStateClass::RemoteStopAllZones(void)
{
		TurnOffZonesWorker();
}

void runStateClass::TurnOffZones()
{
		TurnOffZonesWorker();
#ifndef SG_STATION_MASTER	// we send remote master notifications only if this station is not a master by itself
		if( GetEvtMasterFlags() & EVTMASTER_FLAGS_REPORT_ZONES )
		{
			rprotocol.SendZonesReport(0, GetMyStationID(), GetEvtMasterStationID(), 0, GetNumZones());	// Note: this would work correctly only on Remote station, with only one station defined
			TRACE_INFO(F("TurnOffZones - reporting event to Master\n"));
		}
#endif
}

void runStateClass::TurnOffZonesWorker()
{
        TRACE_INFO(F("Turning Off All Zones\n"));

		for( uint8_t i=0; i<GetNumZones(); i++ )
		{
			if( zoneStateCache[i] != ZONE_STATE_OFF )
				TurnOffZone(i+1);
		}
		m_iZone = -1;
		if( m_iSchedule != -1 )
			m_iSchedule = -1;
}

void runStateClass::ReportZoneStatus(uint8_t stationID, uint8_t channel, uint8_t z_status)
{
	ShortStation  sStation;

	if( stationID >= MAX_STATIONS )		// stationID is too high
		return;							

	LoadShortStation(stationID, &sStation);
	if( !(sStation.stationFlags & STATION_FLAGS_VALID) || !(sStation.stationFlags & STATION_FLAGS_ENABLED) )
		return;							// Station is not enabled or not valid

	if( channel >= sStation.numZoneChannels )
		return;							// channel out of range for this zone

	if( z_status != 0 )
		zoneStateCache[sStation.startZone+channel] = ZONE_STATE_RUNNING;
	else
		zoneStateCache[sStation.startZone+channel] = ZONE_STATE_OFF;
}

void runStateClass::ReportStationZonesStatus(uint8_t stationID, uint8_t z_status)
{
	ShortStation  sStation;

	if( stationID >= MAX_STATIONS )		// basic protection
		return;							

	LoadShortStation(stationID, &sStation);
	if( !(sStation.stationFlags & STATION_FLAGS_VALID) || !(sStation.stationFlags & STATION_FLAGS_ENABLED) )
		return;							// Station is not enabled or not valid

	for( uint8_t i=0; i<sStation.numZoneChannels; i++ )
	{
		if( z_status & (1<<i) )
			zoneStateCache[sStation.startZone+i] = ZONE_STATE_RUNNING;
		else
			zoneStateCache[sStation.startZone+i] = ZONE_STATE_OFF;
	}
}

runStateClass::runStateClass() : m_iSchedule(-1), m_iZone(-1), m_wuScale(100), m_endPauseMillis(0)
{
	for( int i=0; i<MAX_STATIONS; i++ ){
		sLastContactTime[i] = 0;
		iLastReceivedRSSI[i] = -999; // placeholder
	}
}

void runStateClass::LogEvent()
{
	if( m_iZone >= 0 )
	{
		int duration = int((millis()-m_startZoneMillis)/60000ul);
		uint16_t water_used;
		{
			ShortZone   szone;
			LoadShortZone(m_iZone, &szone);
		
			water_used = uint16_t( uint32_t(duration) * uint32_t(szone.waterFlowRate) );	// calculate this zone water usage
			m_iWaterUsed += water_used;												// increment all-up water usage for this schedule
		}
        sdlog.LogZoneEvent(now()-(uint32_t)m_zoneMins*60ul, m_iZone, duration, water_used, m_iSchedule, GetSeasonalAdjust(), m_wuScale);
	}
}

void runStateClass::LogSchedule()
{
	if( m_iSchedule != -1 )
	{
        sdlog.LogSchedEvent(now()-(millis()-m_startSchedMillis)/1000ul, int((millis()-m_startSchedMillis)/60000ul), uint16_t(m_iWaterUsed/100ul), m_iSchedule, GetSeasonalAdjust(), m_wuScale);
	}
}

void runStateClass::StopSchedule(void)
{
        if( m_iSchedule != -1 )		// a schedule is already running, stop it
		{
			if( m_iZone >= 0 )
				TurnOffZone(m_iZone+1);
			
			m_iZone = -1; // no zone is running
			LogSchedule(); // log previous schedule since we are stopping it
			m_iSchedule = -1;
			m_iWaterUsed = 0;
		}
}

void runStateClass::SetPause(int time2pause)
{
	if( time2pause == 0 )
	{
		if( m_endPauseMillis != 0 ) 
		{
			m_endPauseMillis = 0;	// resume operation
			ProcessScheduledEvents();
		}
	}
	else
	{
		if (GetRunSchedules())	// if there are any currently running schedules - stop it.
		{ 
			StopSchedule();
		}
		m_endPauseMillis = millis() + uint32_t(time2pause)*60000ul;
		if( m_endPauseMillis == 0 ) m_endPauseMillis = 1;	// account for rare condition when due to overflow new end millis time equals 0 (but we use 0 as a flag here)
		ProcessScheduledEvents();
	}
}

uint8_t runStateClass::sAdj(uint8_t val)
{
		static	uint32_t	lastWUUpdate = 0;		// timestamp (in millis) of the last WU update

		if( (lastWUUpdate==0) || ((lastWUUpdate+WU_VALID_TIME*60000)<millis()) )			
        {
                Weather w;
                char key[17];
                GetApiKey(key);
                char pws[12] = {0};
                GetPWS(pws);
                m_wuScale = w.GetScale(GetWUIP(), key, GetZip(), pws, GetUsePWS());   // factor to adjust times by.  100 = 100% (i.e. no adjustment)
				lastWUUpdate = millis();
        }
        uint8_t seasonal = GetSeasonalAdjust();
        long scale = ((long)seasonal * (long)m_wuScale) / 100;
        
		return min(((long)val * scale + 50) / 100, 254);
}


void runStateClass::StartSchedule(bool fQuickSched, int8_t iSched)
{
		StopSchedule();	// stop currently running schedule if any
		m_iWaterUsed = 0;	// zero out water usage counter

		if( fQuickSched )
		{
			uint8_t	mZones = GetNumZones();

			for( uint8_t i=0; i<mZones; i++ )
			{
				if( quickSchedule.zone_duration[i] != 0 )  // OK, we found first zone in this schedule to start
				{
					//TRACE_CRIT(F("StartSchedule - starting quick schedule wiht zone=%d, duration=%d\n"),int(i), int(quickSchedule.zone_duration[i]));

					m_iSchedule = 100;	// quick schedule goes under standard number 100.
					m_startSchedMillis = millis();
					m_iZone = i;
					m_startZoneMillis = millis();
					m_zoneMins = quickSchedule.zone_duration[i];
					
					TurnOnZone(i+1, m_zoneMins);
					return;	// done
				}
			}
			return;		// we have not found any enabled zones in quickSchedule			
		}
		else
		{
			uint8_t	mZones = GetNumZones();
			Schedule	sched;

			LoadSchedule(iSched, &sched);
			if( !sched.IsEnabled() )		// basic protection, if schedule is not enabled - exit.
				return;

			for( uint8_t i=0; i<mZones; i++ )
			{
				if( sched.zone_duration[i] != 0 )  // OK, we found first zone in this schedule to start
				{
					m_iSchedule = iSched;	// quick schedule goes under standard number 100.
					m_startSchedMillis = millis();
					m_iZone = i;
					m_startZoneMillis = millis();
					if (sched.IsWAdj())
						m_zoneMins = sAdj(sched.zone_duration[i]);
					else
						m_zoneMins = sched.zone_duration[i];
					
					TurnOnZone(i+1, m_zoneMins);
					return;	// done
				}
			}
		}
}

// process scheduled events

void runStateClass::ProcessScheduledEvents(void)
{
	if( !GetRunSchedules() )	// schedules are currently disabled
	{
		if( m_iSchedule != -1 )		// if a schedule is running, stop it
			StopSchedule();
		return;
	}

	if( m_iSchedule != -1 )		// a schedule is currently running
	{
		if( m_iZone == -2 )		// delay between zones is currently in progress
		{
			register uint32_t  rt = millis()-m_startZoneMillis;
			if( rt < SG_DELAY_BETWEEN_ZONES )
				return;			// currently executing delay still has time

			// OK, delay time finished, start next zone in the same schedule. m_iNextZone should have zone ID to start
			
			if( m_iNextZone < 0 )  // state corruption check
			{
				SYSEVT_ERROR(F("ProcessEvents - state corruption, end of delay with m_iNextZone not set\n"));
				StopSchedule();
				return;
			}

			Schedule	sched;
			if( m_iSchedule == 100 )	// quick schedule
			{
				memcpy(&sched, &quickSchedule, sizeof(quickSchedule));
			}
			else
			{
				LoadSchedule(m_iSchedule, &sched);
				if( !sched.IsEnabled() )		// basic protection, if schedule is not enabled - exit.
				{
					TRACE_CRIT(F("ProcessScheduledEvents - current schedule %d is not enabled, exiting\n"));
					m_iZone = -1;
					StopSchedule();
					return;
				}
			}
			m_iZone = m_iNextZone; m_iNextZone = -1;
			m_startZoneMillis = millis();
			if (sched.IsWAdj())
				m_zoneMins = sAdj(sched.zone_duration[m_iZone]);
			else
				m_zoneMins = sched.zone_duration[m_iZone];

			TurnOnZone(m_iZone+1, m_zoneMins);
			return;	// done
		} // inter-zone delay
		else if( m_iZone >= 0 )		// a zone is currently running
		{
			register uint32_t  rt = (millis()-m_startZoneMillis)/60000ul;
			if( rt < m_zoneMins )
				return;			// currently running zone still has time to run

			// OK, run time finished, stop this zone and start new one in the same schedule
			LogEvent();
			TurnOffZone(m_iZone+1);
			
			uint8_t		mZones = GetNumZones();
			Schedule	sched;
			if( m_iSchedule == 100 )	// quick schedule
			{
				memcpy(&sched, &quickSchedule, sizeof(quickSchedule));
			}
			else
			{
				LoadSchedule(m_iSchedule, &sched);
				if( !sched.IsEnabled() )		// basic protection, if schedule is not enabled - exit.
				{
					TRACE_CRIT(F("ProcessScheduledEvents - current schedule %d is not enabled, exiting\n"));
					m_iZone = -1;
					StopSchedule();
					return;
				}
			}

			for( uint8_t i=m_iZone+1; i<mZones; i++ )
			{
				if( sched.zone_duration[i] != 0 )  // OK, we found first zone in this schedule to start
				{
					// add delay between zones in a schedule

					m_iNextZone = i;	//next zone to run after delay
					m_iZone = -2;		// flag indicating we are in delay mode
					m_startZoneMillis = millis();

					return;	// done
				}
			}
			// we have not found a new zone in the current schedule, close current schedule
			m_iZone = -1;
			StopSchedule();
			return;
		}	//a zone is currently running
		else
		{
			TRACE_ERROR(F("ProcessEvents - state corruption, active schedule with no zones\n"));
			StopSchedule();
			return;
		}
	}	// a schedule is currently running
	else
	{
		uint8_t	schedID, zoneID;
		short	sTime;

		if( GetNextEvent(&schedID, &zoneID, &sTime) )
		{
			time_t	 t = now();
			register short  cTime = hour(t)*60 + minute(t);
			if( sTime == cTime )
				StartSchedule(false, schedID);
		}
	}
}

// return true if the schedule is enabled and runs today.
static inline bool IsRunToday(const Schedule & sched, time_t time_now)
{
        if ((sched.IsEnabled())
                        && (((sched.IsInterval()) && ((elapsedDays(time_now) % sched.interval) == 0))
                                        || (!(sched.IsInterval()) && (sched.day & (0x01 << (weekday(time_now) - 1))))))
                return true;
        return false;
}



// finds next watering event
// by scanning schedules
//
// Input - none
//
// Output - true if found and false otherwise
//			if true, will set schedule ID and zone ID of the next event, as well as time (in minutes since midnight) when it is supposed to run

bool GetNextEvent(uint8_t *pSchedID, uint8_t *pZoneID, short *pTime)
{
		bool	fRet = false;

        // Make sure we're running now
        if( !GetRunSchedules() )
                return false;		// schedules are disabled, so no next event

		*pTime = 32000;		// initial bogus number, any real value will be smaller than that (time is expressed in minutes since midnight)

        const time_t time_now = now();
        const uint8_t iNumSchedules = GetNumSchedules();
		short cTime = hour(time_now)*60 + minute(time_now) + runState.getRemainingPauseTime();

		for( uint8_t i = 0; i < iNumSchedules; i++ )
        {
                Schedule sched;
                LoadSchedule( i, &sched );
                if( IsRunToday(sched, time_now) )
                {
                        // now scan events for each of the start times.
                        for( uint8_t j = 0; j <= 3; j++ )
                        {
                                const short start_time = sched.time[j];	//Note: schedule may have up to 4 start times specified, stored as minutes (since midnight?)
                                if( start_time != -1 )
                                {
										if( (start_time<*pTime) && (start_time>=cTime) )
										{
											for( uint8_t iZone = 0; iZone < MAX_ZONES; iZone++ )
											{
												if( sched.zone_duration[iZone] != 0 )
												{
													*pTime = start_time;
													*pZoneID = iZone;
													*pSchedID = i;
													fRet = true;
													
													break;
												}
											}
										}
                                }
                        }
                }
        }

		return fRet;
}



void mainLoop()
{
        static bool firstLoop = true;
        static bool bDoneMidnightReset = false;
        if (firstLoop)
        {
                firstLoop = false;
                freeMemory();

				lBoardParallel.begin();			// start local Parallel board (IO) handler
				lBoardSerial.begin();			// start local Serial board (IO) handler

                sensorsModule.begin();  // start sensors module
                
#ifdef HW_ENABLE_ETHERNET
                //Init the web server
                if (!webServer.Init())
                        exit(EXIT_FAILURE);

#ifdef ARDUINO
                //Init the TFTP server
                tftpServer.Init();
#endif
                // Set the clock.
                nntpTimeServer.checkTime();
#endif //HW_ENABLE_ETHERNET

                sdlog.begin();
                SYSEVT_CRIT(F("System started."));
			    localUI.set_mode(OSUI_MODE_HOME);  // set to HOME mode, page 0
			    localUI.resume();
        }

// [Tony-osp] Optimization - following code needs to be executed regularly but high frequency is not required.
// To preserve system resources let's make it execute once per second.
//
// Also to help to spread various periodic tasks more evenly, I use the notion of 1/10 sec tick, 
//  and different types of load are executed at different ticks.
//
// I have to go into this complexity because the number of scheduled tasks increased a lot (sensors, RF network etc), 
//  while some of the loop() - style tasks are time-sensitive. Key task that is time-sensisive - local UI handling.

       static unsigned long  old_millis = 0;
	   static uint8_t		 tick_counter = 0;	// We use 1/10th of a second ticks to help running various scheduled tasks
       unsigned long  new_millis = millis();	// Note: we are using built-in Arduino millis() function instead of now() or time-zone adjusted LocalNow(), because it is a lot faster
                                                               // and for detecting second change it does not make any difference.

       if( (new_millis - old_millis) >= 100 )
	   {
            old_millis = new_millis;
			tick_counter++;
			if( tick_counter >= 10 ) // rollover
				tick_counter = 0;
			
			if( (tick_counter%10) == 0 )	// one-second block1
			{
				 // Check to see if we need to set the clock and do so if necessary.
#if defined(HW_ENABLE_ETHERNET) && defined(ARDUINO)
				nntpTimeServer.checkTime();
#endif //HW_ENABLE_ETHERNET

				const time_t timeNow = now();
				// One shot at midnight
				if ((hour(timeNow) == 0) && !bDoneMidnightReset)
				{
                     TRACE_INFO(F("Reloading Midnight\n"));
                     bDoneMidnightReset = true;
                     // TODO:  outstanding midnight events.  See other TODO for how.
				}
				else if (hour(timeNow) != 0)
                     bDoneMidnightReset = false;
			}  
			else if( (tick_counter%10) == 3 )	// one-second block2
			{
#ifdef SG_STATION_MASTER	// if this is Master station, send time broadcasts
				if( nntpTimeServer.GetNetworkStatus() )		// if we have reliable time data
					rprotocol.SendTimeBroadcast();				// broadcast time on RF network
#endif //SG_STATION_MASTER
			}
			else if( (tick_counter%10) == 4 )	// one-second block3
			{
				sensorsModule.loop();  // read and process sensors. Note: sensors module has its own scheduler.
			}
			else if( (tick_counter%10) == 5 )	// one-second block4
			{
             	zoneHandlerLoop();
			}
			else if( (tick_counter%10) == 7 )	// one-second block5
			{
				lBoardSerial.loop();			// refresh state of the serial (OS-style) outputs
			}
	   }
        
#ifdef HW_ENABLE_ETHERNET
        //  See if any web clients have connected
        webServer.ProcessWebClients();
#endif //HW_ENABLE_ETHERNET

        // Process any pending events.
        runState.ProcessScheduledEvents();

#if defined(ARDUINO) && defined(HW_ENABLE_ETHERNET)
        // Process the TFTP Server
        tftpServer.Poll();
#endif //HW_ENABLE_ETHERNET

}

