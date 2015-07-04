// core.cpp
// This file constitutes the core functions that run the scheduling for the Sprinkler system.
// Author: Richard Zimmerman
// Copyright (c) 2013 Richard Zimmerman
//
//
// Modifications for multi-station and SmartGarden system by Tony-osp
//
#include "core.h"
#include "settings.h"
#include "Weather.h"
#include "web.h"
#include "Event.h"
#include "port.h"
#include "LocalBoard.h"
#include <stdlib.h>
#include "sensors.h"
#include "XBeeRF.h"
#include "localUI.h"
#ifdef ARDUINO
#include "tftp.h"
static tftp tftpServer;
#else
#include <wiringPi.h>
#include <unistd.h>
#endif

// Local forward declarations
void ProcessEvents();


// Core modules

#ifdef LOGGING
Logging sdlog;
#endif
static web webServer;
nntp nntpTimeServer;
runStateClass runState;

LocalBoardParallel	lBoardParallel;		// local hardware handler for Parallel-connected stations
LocalBoardSerial	lBoardSerial;		// local hardware handler for Serially-connected stations

// maximum ulong value
#define MAX_ULONG       4294967295

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

void runStateClass::TurnOnZone(uint8_t nZone, uint8_t ttr)
{
        trace(F("Turning On Zone %d\n"), nZone);

		nZone--;		// adjust zone number (1 based vs 0 based)

        if( nZone >= GetNumZones() )
		{
			trace(F("TurnOnZone - invalid zone %d\n"), (uint16_t)nZone);
            return;
		}

        ShortZone zone;
        LoadShortZone(nZone, &zone);

		// sanity checks
		if( !zone.bEnabled ){

			trace(F("TurnOnZone - error, zone %d is not enabled\n"), (uint16_t)nZone);
			return;
		}
		if( zone.stationID > MAX_STATIONS ){

			trace(F("TurnOnZone - data corruption, StationID for zone %d is too high (%d)\n"), (uint16_t)nZone, (uint16_t)(zone.stationID));
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
				trace(F("TurnOnZone - lBoardParallel returned failure for zone %d\n"), (uint16_t)nZone);
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
				trace(F("TurnOnZone - lBoardSerial returned failure for zone %d\n"), (uint16_t)nZone);
				return;
			}

			// Turn on the pump if necessary
//			lBoard.PumpControl(zone.bPump);
		}
		else if( sStation.networkID == NETWORK_ID_XBEE )
		{
			if( XBeeRF.ChannelOn(zone.stationID, zone.channel, ttr) )
			{
				zoneStateCache[nZone] = ZONE_STATE_STARTING + ZONE_STATE_TIMEOUT;	// remote stations go to "starting" state first, and will transition to "running" state when response arrives
			}
			else
			{
				trace(F("TurnOnZone - XBeeRF.ChannelOn returned failure for zone %d\n"), (uint16_t)nZone);
				return;
			}

			// Turn on the pump if necessary
//			lBoard.PumpControl(zone.bPump);
		}
		else
		{
			trace(F("TurnOnZone - unknown NetworkID for station %d, cannot turn On zone.\n"), (uint16_t)nZone);
			return;
		}
}

void runStateClass::TurnOffZone(uint8_t nZone)
{
        trace(F("Turning Off Zone %d\n"), nZone);

		nZone--;		// adjust zone number (1 based vs 0 based)

        if( nZone >= GetNumZones() )
		{
			trace(F("TurnOffZone - invalid zone %d\n"), (uint16_t)nZone);
            return;
		}

        ShortZone zone;
        LoadShortZone(nZone, &zone);

		// sanity checks
		if( !zone.bEnabled ){

			trace(F("TurnOffZone - error, zone %d is not enabled\n"), (uint16_t)nZone);
			return;
		}
		if( zone.stationID > MAX_STATIONS ){

			trace(F("TurnOffZone - data corruption, StationID for zone %d is too high (%d)\n"), (uint16_t)nZone, (uint16_t)(zone.stationID));
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
		else if( sStation.networkID == NETWORK_ID_XBEE )
		{
			if( XBeeRF.ChannelOff(zone.stationID, zone.channel) )
			{
				zoneStateCache[nZone] = ZONE_STATE_STOPPING + ZONE_STATE_TIMEOUT;	// remote stations go to "stopping" state first, and will transition to "running" state when response arrives
			}
			else
			{
				trace(F("TurnOffZone - XBeeRF.ChannelOff returned failure for zone %d\n"), (uint16_t)nZone);
				return;
			}

			// Turn on the pump if necessary
//			lBoard.PumpControl(zone.bPump);
		}
		else
		{
			trace(F("TurnOffZone - unknown NetworkID for station %d, cannot turn On zone.\n"), (uint16_t)nZone);
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
		uint8_t		ch;
		{
			ShortStation	sStation;

			LoadShortStation(stationID, &sStation);

			if( !(sStation.stationFlags & STATION_FLAGS_ENABLED) || !(sStation.stationFlags & STATION_FLAGS_VALID) )
				return false;

			if( channel > sStation.numZoneChannels )
				return false;

			ch = sStation.startZone+channel;
		}

		uint8_t	n_zones = GetNumZones();
        if( ch >= n_zones ) return	false;    // basic protection, ensure that required zone number is within acceptable range

        // So, we first end any schedule that's currently running by turning things off then on again.
        ReloadEvents();

        if( ActiveZoneNum() != -1 ){    // something is currently running, turn it off

                runState.TurnOffZones();
                runState.SetManual(false);
        }

        for( uint8_t n=0; n<n_zones; n++ ){

                quickSchedule.zone_duration[n] = 0;  // clear up QuickSchedule to zero out run time for all zones
        }

// set run time for required zone.
//
        quickSchedule.zone_duration[ch] = time2run;
        LoadSchedTimeEvents(0, true);
		ProcessEvents();

		return true;
}

void runStateClass::RemoteStopAllZones(void)
{
		TurnOffZones();
        SetManual(false);
}

void runStateClass::TurnOffZones()
{
        trace(F("Turning Off All Zones\n"));

		for( uint8_t i=0; i<GetNumZones(); i++ )
		{
			if( zoneStateCache[i] != ZONE_STATE_OFF )
				TurnOffZone(i+1);
		}
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

runStateClass::runStateClass() : m_bSchedule(false), m_bManual(false), m_iSchedule(-1), m_zone(-1), m_endTime(0), m_eventTime(0)
{
	for( int i=0; i<MAX_STATIONS; i++ ) sLastContactTime[i] = 0;
}

void runStateClass::LogSchedule()
{
#ifdef LOGGING
        if ((m_eventTime > 0) && (m_zone >= 0))
                sdlog.LogZoneEvent(m_eventTime, m_zone, now() - m_eventTime, m_bSchedule ? m_iSchedule+1:-1, m_adj.seasonal, m_adj.wunderground);
#endif
}

void runStateClass::SetSchedule(bool val, int8_t iSched, const runStateClass::DurationAdjustments * adj)
{
        LogSchedule();
        m_bSchedule = val;
        m_bManual = false;
        m_zone = -1;
        m_endTime = 0;
        m_iSchedule = val?iSched:-1;
        m_eventTime = now();
        m_adj = adj?*adj:DurationAdjustments();
}

void runStateClass::ContinueSchedule(int8_t zone, short endTime)
{
        LogSchedule();
        m_bSchedule = true;
        m_bManual = false;
        m_zone = zone;
        m_endTime = endTime;
        m_eventTime = now();
}

void runStateClass::SetManual(bool val, int8_t zone)
{
        LogSchedule();
        m_bSchedule = false;
        m_bManual = val;
        m_zone = zone;
        m_endTime = 0;
        m_iSchedule = -1;
        m_eventTime = now();
        m_adj=DurationAdjustments();
}





uint8_t GetZoneState(uint8_t iNum)
{
	iNum--;		// adjust zone number (1 based vs 0 based)

	if( iNum >= GetNumZones() )
	{
		trace(F("GetZoneState - wrong zone number %d\n"), (uint16_t)iNum);
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




// Adjust the durations based on atmospheric conditions
static runStateClass::DurationAdjustments AdjustDurations(Schedule * sched)
{
        runStateClass::DurationAdjustments adj(100);
        if (sched->IsWAdj())
        {
                Weather w;
                char key[17];
                GetApiKey(key);
                char pws[12] = {0};
                GetPWS(pws);
                adj.wunderground = w.GetScale(GetWUIP(), key, GetZip(), pws, GetUsePWS());   // factor to adjust times by.  100 = 100% (i.e. no adjustment)
        }
        adj.seasonal = GetSeasonalAdjust();
        long scale = ((long)adj.seasonal * (long)adj.wunderground) / 100;
		uint8_t	n_zones = GetNumZones();
        for (uint8_t k = 0; k < n_zones; k++)
                sched->zone_duration[k] = min(((long)sched->zone_duration[k] * scale + 50) / 100, 254);
        return adj;
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

// Load the on/off events for a specific schedule/time or the quick schedule
void LoadSchedTimeEvents(int8_t sched_num, bool bQuickSchedule)
{
        Schedule sched;
        runStateClass::DurationAdjustments adj;
        if (!bQuickSchedule)
        {
                const uint8_t iNumSchedules = GetNumSchedules();
                if ((sched_num < 0) || (sched_num >= iNumSchedules))
                        return;
                LoadSchedule(sched_num, &sched);
                adj=AdjustDurations(&sched);
        }
        else
                sched = quickSchedule;

        const time_t local_now = now();
        short start_time = (local_now - previousMidnight(local_now)) / 60;

		uint8_t	n_zones = GetNumZones();
        for (uint8_t k = 0; k < n_zones; k++)
        {
                ShortZone zone;
                LoadShortZone(k, &zone);
                if (zone.bEnabled && (sched.zone_duration[k] > 0))
                {
                        if (iNumEvents >= MAX_EVENTS - 1)
                        {  // make sure we have room for the on && the off events.. hence the -1
                                trace(F("ERROR: Too Many Events!\n"));
                        }
                        else
                        {
                                events[iNumEvents].time = start_time;
                                events[iNumEvents].command = 0x01; // Turn on a zone
                                events[iNumEvents].data[0] = k+1; // Zone to turn on
                                events[iNumEvents].data[1] = (start_time + sched.zone_duration[k]) >> 8;
                                events[iNumEvents].data[2] = (start_time + sched.zone_duration[k]) & 0x00FF;
                                events[iNumEvents].data[3] = sched.zone_duration[k];
                                iNumEvents++;
                                start_time += sched.zone_duration[k];
                        }
                }
        }
        // Load up the last turn off event.
        events[iNumEvents].time = start_time;
        events[iNumEvents].command = 0x02; // Turn off all zones
        events[iNumEvents].data[0] = 0;
        events[iNumEvents].data[1] = 0;
        events[iNumEvents].data[2] = 0;
        iNumEvents++;
        runState.SetSchedule(true, bQuickSchedule?99:sched_num, &adj);

		ProcessEvents();
}

void ClearEvents()
{
        iNumEvents = 0;
        runState.SetSchedule(false);
}

// TODO:  Schedules that go past midnight!
//  Pretty simple.  When we one-shot at midnight, check to see if any outstanding events are at time >1400.  If so, move them
//  to the top of the event stack and subtract 1440 (24*60) from their times.

// Loads the events for the current day
void ReloadEvents(bool bAllEvents)
{
        ClearEvents();
        runState.TurnOffZones();

        // Make sure we're running now
        if (!GetRunSchedules())
                return;

        const time_t time_now = now();
        const uint8_t iNumSchedules = GetNumSchedules();
        for (uint8_t i = 0; i < iNumSchedules; i++)
        {
                Schedule sched;
                LoadSchedule(i, &sched);
                if (IsRunToday(sched, time_now))
                {
                        // now load up events for each of the start times.
                        for (uint8_t j = 0; j <= 3; j++)
                        {
                                const short start_time = sched.time[j];
                                if (start_time != -1)
                                {
                                        if (!bAllEvents && (start_time <= (long)(time_now - previousMidnight(time_now))/60 ))
                                                continue;
                                        if (iNumEvents >= MAX_EVENTS)
                                        {
                                                trace(F("ERROR: Too Many Events!\n"));
                                        }
                                        else
                                        {
                                                events[iNumEvents].time = start_time;
                                                events[iNumEvents].command = 0x03;  // load events for schedule i, time j
                                                events[iNumEvents].data[0] = i;
                                                events[iNumEvents].data[1] = j;
                                                events[iNumEvents].data[2] = 0;
                                                iNumEvents++;
                                        }
                                }
                        }
                }
        }
}

// Check to see if there are any events that need to be processed.
void ProcessEvents()
{
        const time_t local_now = now();
        const short time_check = (local_now - previousMidnight(local_now)) / 60;
        for (uint8_t i = 0; i < iNumEvents; i++)
        {
                if (events[i].time == -1)
                        continue;
                if (time_check >= events[i].time)
                {
                        switch (events[i].command)
                        {
                        case 0x01:  // turn on valves in data[0]
                                runState.TurnOffZones();			// [Tony-osp] - SmartGarden conversion
														//
														// Note:	Right now the scheduling logic assumes that turning On a Zone will automatically
														//			turn Off all other zones. However the new execution engine does not support
														//			this assumption - it requires the caller to explicitly turn zones On and Off.
														//			To emulate old behavior we will explicitly turn Off all zones before 
														//			turning On a new one.

                                runState.TurnOnZone(events[i].data[0],events[i].data[3]);
                                runState.ContinueSchedule(events[i].data[0], events[i].data[1] << 8 | events[i].data[2]);
                                events[i].time = -1;
                                break;
                        case 0x02:  // turn off all valves
                                runState.TurnOffZones();
                                runState.SetSchedule(false);
                                events[i].time = -1;
                                break;
                        case 0x03:  // load events for schedule(data[0]) time(data[1])
                                if (runState.isSchedule())  // If we're already running a schedule, push this off 1 minute
                                        events[i].time++;
                                else
                                {
                                        // Load all the individual events for the individual zones on/off
                                        LoadSchedTimeEvents(events[i].data[0]);
                                        events[i].time = -1;
                                }
                                break;
                        };
                }
        }
}

void mainLoop()
{
        static bool firstLoop = true;
        static bool bDoneMidnightReset = false;
        if (firstLoop)
        {
                firstLoop = false;
                freeMemory();

                if (IsFirstBoot())
                        ResetEEPROM();

				lBoardParallel.begin();			// start local Parallel board (IO) handler
				lBoardSerial.begin();			// start local Serial board (IO) handler

                sensorsModule.begin();  // start sensors module
                
                runState.TurnOffZones();
                ClearEvents();

#ifdef HW_ENABLE_ETHERNET
                //Init the web server
                if (!webServer.Init())
                        exit(EXIT_FAILURE);

#ifdef ARDUINO
                //Init the TFTP server
//				trace(F("Skipping TFPT\n"));
                tftpServer.Init();
#endif
                // Set the clock.
//				trace(F("Skipping NNTP time\n"));
                nntpTimeServer.checkTime();
#endif //HW_ENABLE_ETHERNET

                ReloadEvents();
                //ShowSockStatus();
#ifdef LOGGING
                sdlog.begin(PSTR("System started."));
#endif
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
//  while some of the loop() - style tasks are time-sensitive. Key tasks that is time-sensisive - local UI handling.

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
#ifdef HW_ENABLE_ETHERNET
				nntpTimeServer.checkTime();
#endif //HW_ENABLE_ETHERNET

				const time_t timeNow = now();
				// One shot at midnight
				if ((hour(timeNow) == 0) && !bDoneMidnightReset)
				{
                     trace(F("Reloading Midnight\n"));
                     bDoneMidnightReset = true;
                     // TODO:  outstanding midnight events.  See other TODO for how.
                     ReloadEvents(true);
				}
				else if (hour(timeNow) != 0)
                     bDoneMidnightReset = false;
			}  
			else if( (tick_counter%10) == 3 )	// one-second block2
			{
#ifdef SG_STATION_MASTER	// if this is Master station, send time broadcasts
				if( nntpTimeServer.GetNetworkStatus() )		// if we have reliable time data
					XBeeRF.SendTimeBroadcast();				// broadcast time on RF network
#endif //SG_STATION_MASTER
			}
			else if( (tick_counter%10) == 6 )	// one-second block3
			{
				sensorsModule.loop();  // read and process sensors. Note: sensors module has its own scheduler.
             	zoneHandlerLoop();
			}
	   }

        
#ifdef HW_ENABLE_ETHERNET
        //  See if any web clients have connected
        webServer.ProcessWebClients();
#endif //HW_ENABLE_ETHERNET

        // Process any pending events.
        ProcessEvents();

#ifdef ARDUINO
#ifdef HW_ENABLE_ETHERNET
        // Process the TFTP Server
        tftpServer.Poll();
#endif //HW_ENABLE_ETHERNET
#else
        // if we've changed the settings, store them to disk
        EEPROM.Store();
#endif

}

