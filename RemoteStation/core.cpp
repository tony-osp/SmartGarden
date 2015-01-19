// core.cpp
// This file provides necessary minimal core functions required for RemoteStation operation
// Author: Tony-osp
// Copyright (c) 2014 Tony-osp
//

#include "core.h"
#include "settings.h"
#include "port.h"
#include <stdlib.h>
#include "SGSensors.h"

#ifdef LOGGING
Logging sdlog;
#endif
runStateClass runState;
Sensors sensorsModule;

// A bitfield that defines which zones are currently on.
int ZoneState = 0;


void runStateClass::LogSchedule(int iValve, time_t eventTime, bool bManual, int iSchedule )
{
#ifdef LOGGING
		sdlog.LogZoneEvent(eventTime, iValve, now() - eventTime, bManual ? iSchedule:-1, 0, 0);
#endif
}

// main zones information. This is defined outside of the class for convenience

static zone_struct  zones[NUM_ZONES];	

uint8_t ZoneToIOMap[] = {31, 41, 40, 42, 43, 44, 45, 46, 47, 38, 37, 36, 35, 34, 33, 32};

static uint16_t outState = 0;
static uint16_t prevOutState = 0;

static void io_latch()
{
        // check if things have changed
        if (outState == prevOutState)
                return;

        for (int i = 0; i <= NUM_ZONES; i++)
        {
			digitalWrite(ZoneToIOMap[i], (outState&(0x01<<i))?1:0);
        }

        // Now store the new output state so we know if things have changed
        prevOutState = outState;
}

void io_setup()
{
        for (uint8_t i=0; i<sizeof(ZoneToIOMap); i++)
        {
               pinMode(ZoneToIOMap[i], OUTPUT);
               digitalWrite(ZoneToIOMap[i], 0);
        }

		outState = 0;
        prevOutState = 1;
        io_latch();
}


void TurnOffZones()
{
        trace(F("Turning Off All Zones\n"));
        outState = 0;

        io_latch();
}

bool isZoneOn(int8_t iNum)
{
        if ((iNum <= 0) || (iNum > NUM_ZONES))
                return false;

		if( zones[iNum-1].flags & ZONE_FLAGS_ACTIVE )	return true;
		else											return false;
}

int8_t ActiveZoneNum(void)
{
        if( outState == 0 ) return -1;      // nothing is running

        for( byte n=1; n<=NUM_ZONES; n++){          // note: zones are numbered from 1. Slot 0 is used for the common pump.

			if( zones[n-1].flags & ZONE_FLAGS_ACTIVE ) return n;
        }

        return -1;  // strange, we really should not get there - it could happen only when pump is On, but all zones are off. Treat this condition as Off.
}

bool TurnOnZone(uint8_t iValve)
{
        if( iValve >= NUM_ZONES )
		{
				trace(F("TurnOnZone - failed, incorrect zone number %d\n"), iValve);
                return false;
		}

        outState |= 0x01 << iValve;
        io_latch();
		
		return true;
}

bool TurnOffZone(uint8_t iValve)
{
        if( iValve >= NUM_ZONES )
		{
				trace(F("TurnOnZone - failed, incorrect zone number %d\n"), iValve);
                return false;
		}

        outState &= ~(0x01 << iValve);
        io_latch();

		return true;
}


bool runStateClass::StartZone(bool bManual, int iSchedule, int8_t iValve_in, uint8_t time2run )
{
// Sanity checks
	if( (iValve_in < 1) || (iValve_in > NUM_ZONES) ) 
	{
			trace(F("StartZone - failure, invalid zone requested.\n"));
			return false;  
	}
	
	uint8_t	iValve = iValve_in-1;		// convert to zero based 

//	trace(F("Request to start Zone %d (zero based), %d min. runtime\n"), (int)iValve, (int)time2run);

	if( !(zones[iValve].flags & ZONE_FLAGS_ENABLED) )
	{	
			trace(F("StartZone - failure, requested zone is not enabled.\n"));
			return false;   
	}

	if( time2run > max_time2run ) 
	{
			// safety constrain, limit maximum time to run
			time2run = max_time2run;
	}
	
// OK, we are good to go. 

// note, we are keeping start time as well as end millis for each zone. start time (regular time_t format) 
// is required for logging and reporting, while having end time in millis helps to avoid problems with
// already running zone(s) when system time changes.

	if( zones[iValve].flags & ZONE_FLAGS_ACTIVE )	// this zone is already active, just set new running time
	{
		// this zone was already running, record previous run
		LogSchedule(iValve, zones[iValve].tStartTime, (zones[iValve].flags & ZONE_FLAGS_MANUAL)? 1:0, iSchedule );
		
		zones[iValve].runTime = time2run;
		zones[iValve].tStartTime = now();	// reset start time
		zones[iValve].endMillis = millis() + ((unsigned long)time2run)*60000;  // compute end time in millis
		zones[iValve].iSchedule = iSchedule;
		if( bManual ) zones[iValve].flags |= ZONE_FLAGS_MANUAL;
		else 		  zones[iValve].flags &= ~ZONE_FLAGS_MANUAL;
	}
	else 	// start the zone now
	{
		zones[iValve].runTime = time2run;
		zones[iValve].tStartTime = now();	// set start time
		zones[iValve].endMillis = millis() + ((unsigned long)time2run)*60000;  // compute end time in millis
		zones[iValve].iSchedule = iSchedule;
		zones[iValve].flags = ZONE_FLAGS_ENABLED;		// reset flags
		if( bManual ) zones[iValve].flags |= ZONE_FLAGS_MANUAL;
		
		if( TurnOnZone(iValve) )  // actually turn On the zone
		{
			zones[iValve].flags |= ZONE_FLAGS_ACTIVE;	// mark it as active if successful
//			trace(F("Started zone %d, millis()=%lu, endMillis=%lu\n"), (int)iValve, millis(), zones[iValve].endMillis);
		}
		else 
		{
			trace(F("ScheduleZone - failed to turn On zone\n"));
		}
	}

	return true;
}

void	runStateClass::StopAllZones(void)
{
	trace(F("StopShedules - Stopping all zones\n"));

	for( int8_t i=0; i<NUM_ZONES; i++ )
	{
		if( zones[i].flags & ZONE_FLAGS_ACTIVE )		// this zone is currently running
		{	  
			runState.LogSchedule( i, zones[i].tStartTime, (zones[i].flags & ZONE_FLAGS_MANUAL)? 1:0, zones[i].iSchedule );

			TurnOffZone(i);
			zones[i].flags = ZONE_FLAGS_ENABLED;	
			zones[i].runTime = 0;
		}
	}
}


void	runStateClass::begin(void)
{
	for( int8_t i=0; i<NUM_ZONES; i++ )
	{
		memset(&zones[i], 0, sizeof(zone_struct));
		zones[i].flags = ZONE_FLAGS_ENABLED;
	}
	TurnOffZones();

	max_time2run = GetMaxTtr();
}



//
// This procedure is called relatively frequently (once a second) to check if any of the valves should be turned Off
// and/or to handle next zone auto-start
//
void ProcessEvents(void)
{
	for( int8_t i=0; i<NUM_ZONES; i++ )
	{
		if( zones[i].flags & ZONE_FLAGS_ACTIVE )		// this zone is currently running
		{
	  
			if( millis() > zones[i].endMillis ) // time to stop this zone
			{
//				trace(F("ProcessEvents - stopping zone %d\n"), (int)i);

				runState.LogSchedule( i, zones[i].tStartTime, (zones[i].flags & ZONE_FLAGS_MANUAL)? 1:0, zones[i].iSchedule );

				TurnOffZone(i);
				zones[i].flags = ZONE_FLAGS_ENABLED;	
				zones[i].runTime = 0;
			}
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
                io_setup();

                sensorsModule.begin();  // start sensors module

				runState.begin();

//                //Init the web server
//                if (!webServer.Init())
//                        exit(EXIT_FAILURE);

//                // Set the clock.
//                nntpTimeServer.checkTime();

//                ReloadEvents();
                //ShowSockStatus();
#ifdef LOGGING
                sdlog.begin(PSTR("System started."));
#endif

        }

// Optimization - following code needs to be executed regularly but high frequency is not required.
// To preserve system resources let's make it execute once per second.

       static unsigned long  old_millis = 0;
       unsigned long  new_millis = millis();    // Note: we are using built-in Arduino millis() function instead of now() or time-zone adjusted LocalNow(), because it is a lot faster
                                                               // and for detecting second change it does not make any difference.

       if( (new_millis - old_millis) >= 1000 ){

             old_millis = new_millis;

             // Check to see if we need to set the clock and do so if necessary.
//             nntpTimeServer.checkTime();

             const time_t timeNow = now();
             // One shot at midnight
             if ((hour(timeNow) == 0) && !bDoneMidnightReset)
             {
                     trace(F("Reloading Midnight\n"));
                     bDoneMidnightReset = true;
                     // TODO:  outstanding midnight events.  See other TODO for how.
//                     ReloadEvents(true);
             }
             else if (hour(timeNow) != 0)
                     bDoneMidnightReset = false;

             sensorsModule.loop();  // read and process sensors. Note: sensors module has its own scheduler.

        }  // one-second block

//        //  See if any web clients have connected
//        webServer.ProcessWebClients();

		// Process any pending events.
        ProcessEvents();

        // latch any output modifications
        io_latch();
}
