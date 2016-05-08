/*

Logging sub-system implementation for SmartGarden system. This module handles both regular system logging
as well as temperature/humidity/waterflow/etc logging.

Note: Log operation signature is implemented to be compatible with the Sql-based logging in sprinklers_pi control program.

Log file format is described in log_format2.txt file.


Creative Commons Attribution-ShareAlike 3.0 license
Copyright 2014 tony-osp (http://tony-osp.dreamwidth.org/)

*/

#define __STDC_FORMAT_MACROS
#include "sdlog.h"
#include "settings.h"
#include "RProtocolMS.h"

//#define TRACE_LEVEL			7		// trace everything for this module
#include "port.h"


extern SdFat sd;

#define CL_TMPB_SIZE  256    // size of the local temporary buffer

static FILE _syslog_file;

#ifndef SG_STATION_MASTER
static uint8_t  _syslog_EvtBuffer[SYSEVENT_MAX_STRING_LENGTH];
static uint8_t  _syslog_EvtType;
static uint32_t  _syslog_EvtTimeStamp;
static uint8_t  _syslog_EvtByteCounter;
static uint8_t  _syslog_EvtContFlag;
#endif

// local logger helper
static int syslog_putchar(char c, FILE *stream)
{
	trace_char(c);				// directly output character into the trace channel

#ifndef SG_STATION_MASTER
	if( _syslog_EvtByteCounter < (SYSEVENT_MAX_STRING_LENGTH-1) )
	{
		_syslog_EvtBuffer[_syslog_EvtByteCounter] = c;
		_syslog_EvtByteCounter++;
	}
	else
	{
		rprotocol.NotifySysEvent(_syslog_EvtType, _syslog_EvtTimeStamp, 0, _syslog_EvtContFlag?SYSEVENT_FLAG_CONTINUE:0, _syslog_EvtByteCounter, _syslog_EvtBuffer);

		_syslog_EvtBuffer[0] = c;
		_syslog_EvtByteCounter = 1;		
		_syslog_EvtContFlag = true;
	}
#endif

#ifdef HW_ENABLE_SD	// local log on SD card
	return sdlog.lfile.write(c);
#endif //HW_ENABLE_SD

	return 1;
}


// Main Syslog event routine - PSTR format
void syslog_evt(uint8_t event_type, const __FlashStringHelper * fmt, ...)
{
	time_t t = now();

#ifdef HW_ENABLE_SD	// local log on SD card

	if( !sdlog.logger_ready )
		return;

	// limit the scope of local variables to conserve stack space
	{
   // temp buffer for log strings processing
		char tmp_buf[20];

		sprintf_P(tmp_buf, PSTR(SYSTEM_LOG_FNAME_FORMAT), month(t), year(t) );

		if( !sdlog.lfile.open(tmp_buf, O_WRITE | O_APPEND | O_CREAT) ){

            TRACE_ERROR(F("Cannot open system log file (%s)\n"), tmp_buf);

            sdlog.logger_ready = false;      // something is wrong with the log file, mark logger as "not ready"
            return;    // failed to open/create log file
		}

		sprintf_P(tmp_buf, PSTR("%u,%u:%u:%u,%d,"), day(t), hour(t), minute(t), second(t),int(event_type));
		sdlog.lfile.print(tmp_buf);
	}
#endif //HW_ENABLE_SD

	if(		 event_type <= SYSEVENT_CRIT )
	{
		TRACE_CRIT(F("SYSEVT-CRIT: "));
	}
	else if( event_type == SYSEVENT_ERROR )
	{
		TRACE_CRIT(F("SYSEVT-ERROR: "));
	}
	else if( event_type == SYSEVENT_WARNING )
	{
		TRACE_CRIT(F("SYSEVT-WARNING: "));
	}
	else if( event_type == SYSEVENT_NOTICE )
	{
		TRACE_CRIT(F("SYSEVT-NOTICE: "));
	}
	else 
	{
		TRACE_CRIT(F("SYSEVT-INFO: "));	// we are treating INFO and VERBOSE as identical
	}

// prepare to send the event to the Master if this functionality is enabled
#ifndef SG_STATION_MASTER
	_syslog_EvtByteCounter = 0;		
	_syslog_EvtContFlag = false;
	_syslog_EvtType = event_type;
	_syslog_EvtTimeStamp = t;
#endif

	va_list parms;
	va_start(parms, fmt);
    vfprintf_P(&_syslog_file, reinterpret_cast<const char *>(fmt), parms);
	va_end(parms);

#ifdef HW_ENABLE_SD	// local log on SD card
	sdlog.lfile.write('\n');	
	sdlog.lfile.close();
#endif //HW_ENABLE_SD

	trace_char('\n');	

// Send event to the Master if this functionality is enabled
#ifndef SG_STATION_MASTER
	if( _syslog_EvtByteCounter != 0 )
	{
		rprotocol.NotifySysEvent(event_type, _syslog_EvtTimeStamp, 0, _syslog_EvtContFlag?SYSEVENT_FLAG_CONTINUE:0, _syslog_EvtByteCounter, _syslog_EvtBuffer);
	}
#endif
}



Logging::Logging()
{
// initialize internal state

  logger_ready = false;

// prepare common Syslog event routine
  fdev_setup_stream(&_syslog_file, syslog_putchar, NULL, _FDEV_SETUP_WRITE);
}

Logging::~Logging()
{
;
}


// Start logging - open/create system log file, create initial "start" record. Note: uses time/date data, time/date should be available by now
// Takes input string that will be used in the first log record
// Returns true on success and false on failure
//


bool Logging::begin(void)
{
#ifndef HW_ENABLE_SD
	return true;
#else

  char    log_fname[20];
  time_t  curr_time = now();

// Ensure log folders are there, if not - create it.

  sprintf_P(log_fname, PSTR(SYSTEM_LOG_DIR));   // system log directory
  if( !lfile.open(log_fname, O_READ) ){

        TRACE_INFO(F("System log directory not found, creating it.\n"));

        if( !sd.mkdir(log_fname) ){

           TRACE_ERROR(F("Error creating System log directory.\n"));
        }
  }
  lfile.close();      // close the directory

  sprintf_P(log_fname, PSTR(WATERING_LOG_DIR));   // Watering log directory
  if( !lfile.open(log_fname, O_READ) ){

        TRACE_INFO(F("Watering log directory not found, creating it.\n"));

        if( !sd.mkdir(log_fname) ){

           TRACE_ERROR(F("Error creating Watering log directory.\n"));
        }
  }
  lfile.close();      // close the directory

  sprintf_P(log_fname, PSTR(WFLOW_LOG_DIR));   // Waterflow log directory
  if( !lfile.open(log_fname, O_READ) ){

        TRACE_INFO(F("Waterflow log directory not found, creating it.\n"));

        if( !sd.mkdir(log_fname) ){

           TRACE_ERROR(F("Error creating Waterflow log directory.\n"));
        }
  }
  lfile.close();      // close the directory

  sprintf_P(log_fname, PSTR(TEMPERATURE_LOG_DIR));   // Temperature log directory
  if( !lfile.open(log_fname, O_READ) ){

        TRACE_INFO(F("Temperature log directory not found, creating it.\n"));

        if( !sd.mkdir(log_fname) ){

           TRACE_ERROR(F("Error creating Temperature log directory.\n"));
        }
  }
  lfile.close();      // close the directory

  sprintf_P(log_fname, PSTR(HUMIDITY_LOG_DIR));   // Humidity log directory
  if( !lfile.open(log_fname, O_READ) ){

        TRACE_INFO(F("Humidity log directory not found, creating it.\n"));

        if( !sd.mkdir(log_fname) ){

           TRACE_ERROR(F("Error creating Humidity log directory.\n"));
        }
  }
  lfile.close();      // close the directory

  sprintf_P(log_fname, PSTR(PRESSURE_LOG_DIR));   // Atmospheric pressure log directory
  if( !lfile.open(log_fname, O_READ) ){

        TRACE_INFO(F("Pressure log directory not found, creating it.\n"));

        if( !sd.mkdir(log_fname) ){

           TRACE_ERROR(F("Error creating Pressure log directory.\n"));
        }
  }
  lfile.close();      // close the directory


//  generate system log file name
  sprintf_P(log_fname, PSTR(SYSTEM_LOG_FNAME_FORMAT), month(curr_time), year(curr_time) );
  if( !lfile.open(log_fname, O_WRITE | O_APPEND | O_CREAT) ){

        SYSEVT_ERROR(F("Cannot open system log file (%s)\n"), log_fname);
        logger_ready = false;

        return false;    // failed to open/create log file
  }

  lfile.close();
  logger_ready = true;      // we are good to go

  return true;
#endif //!HW_ENABLE_SD
}

void Logging::Close()
{
   logger_ready = false;
}


// Record schedule watering event
//
// Note: we open/close file on each event

bool Logging::LogSchedEvent(time_t start, int duration, uint16_t water_used, int schedule, int sadj, int wunderground)
{
	  TRACE_VERBOSE(F("LogSchedEvent called, start=%lu, duration=%d, schedule=%d, sadj=%d, wunderground=%d\n"), start, duration, schedule, sadj, wunderground);

#ifndef HW_ENABLE_SD
	  return true;
#else
      if( !logger_ready ) return false;  //check if the logger is ready
	
// temp buffer for log strings processing
      char tmp_buf[MAX_LOG_RECORD_SIZE];

      sprintf_P(tmp_buf, PSTR(WATERING_SCH_LOG_FNAME_FORMAT), year(now()));

      if( !lfile.open(tmp_buf, O_WRITE | O_APPEND) ){    // we are trying to open existing log file for write/append

// operation failed, usually because log file for this year does not exist yet. Let's create it and add column headers.
         if( !lfile.open(tmp_buf, O_WRITE | O_APPEND | O_CREAT) ){

               TRACE_ERROR(F("Cannot open watering log file (%s)\n"), tmp_buf);    // file create failed, return an error.
               return false;    // failed to open/create file
         }
         lfile.println(F("Month,Day,Time,Schedule run time(min),Water used(gal),ScheduleID,Adjustment,WUAdjustment"));
      }

      sprintf_P(tmp_buf, PSTR("%u,%u,%u:%u,%u,%u,%u,%i,%i"), month(start), day(start), hour(start), minute(start), duration, water_used, schedule, sadj, wunderground);

      lfile.println(tmp_buf);
      lfile.close();

      return true;
#endif //HW_ENABLE_SD
}

// Record zone watering event
//
// Note: for watering events we open/close file on each event

bool Logging::LogZoneEvent(time_t start, int zone, int duration, uint16_t water_used, int schedule, int sadj, int wunderground)
{
	  time_t t = now();
	  
// Update running counters (in flash)
// Running water counters are stored on per-day of week basis, and we also store the date when we last updated running water counter
// When switching to the new day, we reset running water counter for that day and start new accumulation
// Also when day change occurs, we are adding previously accumulated running water counter to the global lifetime water counter
//
	  {
			register uint8_t	dow = weekday(t)-1;

			SetWWCounter(dow, GetWWCounter(dow)+water_used);
			//TRACE_CRIT(F("Updating WWCounter, dow=%d, duration=%d, zone.wfRate=%d, increment=%d\n"), int(dow), duration, szone.waterFlowRate, int(tmp32) );
	  }

#ifndef HW_ENABLE_SD
	  return true;
#else
	  water_used = water_used/100; // water used is reported in 1/100 of a gallon. We use full precision value for WWCounters calculations, but round it to nearest gallon for logging.

	  if( !logger_ready ) return false;  //check if the logger is ready
	
// temp buffer for log strings processing
      char tmp_buf[MAX_LOG_RECORD_SIZE];

      sprintf_P(tmp_buf, PSTR(WATERING_LOG_FNAME_FORMAT), (int)month(t), (int)(year(t)%100) );

      if( !lfile.open(tmp_buf, O_WRITE | O_APPEND) ){    // we are trying to open existing log file for write/append

// operation failed, usually because log file for this month does not exist yet. Let's create it and add column headers.
         if( !lfile.open(tmp_buf, O_WRITE | O_APPEND | O_CREAT) ){

               TRACE_ERROR(F("Cannot open watering log file (%s)\n"), tmp_buf);    // file create failed, return an error.
               return false;    // failed to open/create file
         }
         lfile.println(F("Day,Time,Run time(min),Water used(gal),ScheduleID,Adjustment,WUAdjustment"));
      }

      sprintf_P(tmp_buf, PSTR("%u,%u,%u:%u,%u,%u,%u,%i,%i"), zone, day(start), hour(start), minute(start), duration, water_used, schedule, sadj, wunderground);

      lfile.println(tmp_buf);
      lfile.close();

      return true;
#endif //HW_ENABLE_SD
}

// Sensors logging - record sensor reading. 
// Covers all types of basic pressure sensors that provide momentarily (immediate) readings.
//
// sensor_type       -  could be SENSOR_TYPE_TEMPERATURE or any other valid defines
// sensor_id           -  numeric ID of the sensor, minimum 0, maximum 999
// sensor_reading  -  actual sensor reading
//
// Returns true if successful and false if failure.
//
bool Logging::LogSensorReading(uint8_t sensor_type, int sensor_id, int32_t sensor_reading)
{
//	TRACE_ERROR(F("LogSensorReading - enter, sensor_type=%i, sensor_id=%i, sensor_reading=%ld\n"), (int)sensor_type, sensor_id, sensor_reading);

#ifndef HW_ENABLE_SD
	  return true;
#else
	if( !logger_ready ) return false;  //check if the logger is ready

	time_t  t = now();

// temp buffer for log strings processing
      char	tmp_buf[MAX_LOG_RECORD_SIZE];					

	const char *sensorName;

      switch (sensor_type){
      
           case  SENSOR_TYPE_TEMPERATURE:
          
                     sprintf_P(tmp_buf, PSTR(TEMPERATURE_LOG_FNAME_FORMAT), (int)month(t), (int)(year(t)%100), sensor_id );
                     sensorName = PSTR("Temperature(F)");
                     break; 
      
           case  SENSOR_TYPE_PRESSURE:

                     sprintf_P(tmp_buf, PSTR(PRESSURE_LOG_FNAME_FORMAT), (int)month(t), (int)(year(t)%100), sensor_id );
                     sensorName = PSTR("AirPressure");
                     break; 
      
           case  SENSOR_TYPE_HUMIDITY:
          
                     sprintf_P(tmp_buf, PSTR(HUMIDITY_LOG_FNAME_FORMAT), (int)month(t), (int)(year(t)%100), sensor_id );
                     sensorName = PSTR("Humidity");
                     break; 
      
           case  SENSOR_TYPE_WATERFLOW:
          
                     sprintf_P(tmp_buf, PSTR(WFLOW_LOG_FNAME_FORMAT), (int)month(t), (int)(year(t)%100), sensor_id );
                     sensorName = PSTR("Waterflow");
                     break; 

            default:
                     return false;    // sensor_type not recognized
                     break;           
      }
      TRACE_VERBOSE(F("LogSensorReading - about to open file: %s, len=%d\n"), tmp_buf, strlen(tmp_buf));

      if( !lfile.open(tmp_buf, O_WRITE | O_APPEND) ){    // we are trying to open existing log file for write/append

// operation failed, usually because log file for this year does not exist yet. Let's create it and add column headers.
         if( !lfile.open(tmp_buf, O_WRITE | O_APPEND | O_CREAT) ){

               TRACE_ERROR(F("Cannot open or create sensor  log file %s\n"), tmp_buf);    // file create failed, return an error.
               return false;    // failed to open/create file
         }
		 // write header line
         sprintf_P(tmp_buf, PSTR("Day,Time,%S\n"), sensorName); 
		 lfile.write(tmp_buf, strlen(tmp_buf));
         
         TRACE_INFO(F("creating new log file for sensor:%S\n"), sensorName);
      }
//	  TRACE_VERBOSE(F("Opened log file %s, len=%i\n"), tmp_buf, strlen(tmp_buf));

      sprintf_P(tmp_buf, PSTR("%u,%u:%u,%ld\n"), day(t), hour(t), minute(t), sensor_reading);

//	  TRACE_VERBOSE(F("Writing log string %s, len=%d\n"), tmp_buf, strlen(tmp_buf));
	  lfile.write(tmp_buf, strlen(tmp_buf));

      lfile.close();

      return true;    // standard exit-success
#endif //HW_ENABLE_SD
}



bool Logging::TableZone(FILE* stream_file, time_t start, time_t end)
{
#ifndef HW_ENABLE_SD
	  return false;
#else
        char tmp_buf[MAX_LOG_RECORD_SIZE];

		fprintf_P(stream_file, PSTR("{\n\t\"logs\": [\n"));
        
		if (start == 0)
                start = now();

        unsigned int    nyear=year(start);

        end = max(start,end) + 24*3600;  // add 1 day to end time.

        int  nmend = month(end);
        int  ndayend = day(end);
		int  nmstart = month(start);
		int  xsched = -1;

//		TRACE_ERROR(F("TableZone - entering, start year=%u, month=%u, day=%u\n"), year(start), month(start), day(start));

        if( year(end) != year(start) ){     // currently we cannot handle queries that span multiple years. Truncate the query to the year end.
             nmend = 12;    ndayend = 31;
        }

		uint8_t	n_zones = GetNumZones();
		time_t  evt_time = 0;
		time_t  prev_evtEnd = 0;

        for( int nmonth = nmstart; nmonth <= nmend; nmonth++ ){  // iterate over months (watering logs are stored one file per month)

                sprintf_P(tmp_buf, PSTR(WATERING_LOG_FNAME_FORMAT), nmonth, (int)(nyear%100) );

                if( lfile.open(tmp_buf, O_READ) ){  

                     char bFirstRow = true;
   					TRACE_VERBOSE(F("TableZone - reading file %s\n"), tmp_buf);

                     lfile.fgets(tmp_buf, MAX_LOG_RECORD_SIZE-1);  // skip first line in the file - column headers


// OK, we opened required watering log file. Iterate over records, filtering out necessary dates range
                  
                     while( lfile.available() ){

                            int  nday = 0, nhour = 0, nminute = 0, nschedule = 0;
                            int  nsadj = 0, nwunderground = 0, nzone = 0;
							uint16_t  nduration = 0, nwater_used = 0;

                            int bytes = lfile.fgets(tmp_buf, MAX_LOG_RECORD_SIZE-1);
                            if (bytes <= 0)
                                       break;
   							TRACE_VERBOSE(F("TableZone - got string %s\n"), tmp_buf);

// Parse the string into fields. 

							sscanf_P( tmp_buf, PSTR("%u,%u,%u:%u,%u,%u,%i,%i,%i"),
                                                            &nzone, &nday, &nhour, &nminute, &nduration, &nwater_used, &nschedule, &nsadj, &nwunderground);

                            if( (nmonth == nmend) && (nday > ndayend) ){    // check for the end date
			   					
								TRACE_VERBOSE(F("TableZone - date is beyond requested range, stop processing file\n"));
								break;
							}

							{
								tmElements_t tm;   tm.Day = nday;  tm.Month = nmonth; tm.Year = nyear - 1970;  tm.Hour = nhour;  tm.Minute = nminute;  tm.Second = 0;
								evt_time = makeTime(tm);
							}
                            if( evt_time >= start ){        // the record is within required range.
// we have something to output.

                                    if( (nschedule != xsched) || (evt_time > prev_evtEnd) ){
                                      
                                         if( xsched != -1 ) 
                                                   fprintf_P(stream_file, PSTR("\n\t\t\t\t\t]\n\t\t\t\t},\n"));   // if this is not the first schedule, close previous one
                                         
										 Schedule sched;
										 memset(&sched.name, 0, sizeof(sched.name));
										 if( nschedule == 100 )
											 strcpy_P(sched.name, PSTR("Manual"));
										 else
											LoadSchedule(nschedule, &sched);
                                         fprintf_P(stream_file, PSTR("\n\t\t\t\t { \n\t\t\t\t \"scheduleID\": %i,\n\t\t\t\t \"scheduleName\": \"%s\",\n\t\t\t\t \"entries\": ["), nschedule, sched.name);   // JSON schedule header
                                         xsched = nschedule;
                                         bFirstRow = true;
                                    }
									
									prev_evtEnd = evt_time + uint32_t(nduration+1)*60ul;

                                    fprintf_P(stream_file, PSTR("%s \n\t\t\t\t\t { \"date\":%lu, \"zone\":%i, \"duration\":%u, \"water_used\":%u, \"seasonal\":%i, \"wunderground\":%i}"),
                                                                       bFirstRow ? "":",",
                                                                       evt_time, nzone, nduration, nwater_used, nsadj, nwunderground );

                                     bFirstRow = false;
                            }
							else
							{
   								TRACE_VERBOSE(F("TableZone - record date %u:%u before start date of %u:%u, skipping\n"), nmonth, nday, int(month(start)), int(day(start)));
							}
                     }   // while
                     lfile.close();
                }
				else
				{
					TRACE_ERROR(F("TableZone - cannot open log file %s\n"), tmp_buf);
				}
        }   // for( int nmonth = nmstart; nmonth <= nmend; nmonth++ )

        if( xsched != -1)
                     fprintf_P(stream_file, PSTR("\n\t\t\t\t\t ] \n\t\t\t\t } \n"));    // close the last zone if we emitted

		fprintf_P(stream_file, PSTR("\t]\n}"));
        return true;
#endif //HW_ENABLE_SD
}


bool Logging::TableSchedule(FILE* stream_file, time_t start, time_t end)
{
#ifndef HW_ENABLE_SD
	  return false;
#else 
        char tmp_buf[MAX_LOG_RECORD_SIZE];

        if (start == 0)
                start = now();

        unsigned int    nyear=year(start);

        end = max(start,end) + 24*3600;  // add 1 day to end time.

        int  nmend = month(end);
        int  ndayend = day(end);

//		TRACE_ERROR(F("TableSchedule - entering, start year=%u, month=%u, day=%u\n"), year(start), month(start), day(start));

        if( year(end) != year(start) ){     // currently we cannot handle queries that span multiple years. Truncate the query to the year end.
             nmend = 12;    ndayend = 31;
        }

        char bFirstRow = true;
                    
                sprintf_P(tmp_buf, PSTR(WATERING_SCH_LOG_FNAME_FORMAT), nyear );

                if( lfile.open(tmp_buf, O_READ) ){  // logs for each zone are stored in a separate file, with the file name based on the year and zone number. Try to open it.

                     lfile.fgets(tmp_buf, MAX_LOG_RECORD_SIZE-1);  // skip first line in the file - column headers

// OK, we opened required schedule watering log file. Iterate over records, filtering out necessary dates range
                  
                     while( lfile.available() ){

                            int  nmonth = 0, nday = 0, nhour = 0, nminute = 0, nschedule = 0;
                            int  nsadj = 0, nwunderground = 0;
							uint16_t  nwater_used = 0, nduration = 0;

                            int bytes = lfile.fgets(tmp_buf, MAX_LOG_RECORD_SIZE-1);
                            if (bytes <= 0)
                                       break;

// Parse the string into fields. First field (up to two digits) is the day of the month

                            sscanf_P( tmp_buf, PSTR("%u,%u,%u:%u,%u,%u,%i,%i,%i"),
                                                            &nmonth, &nday, &nhour, &nminute, &nduration, &nwater_used, &nschedule, &nsadj, &nwunderground);

                            if( (nmonth > nmend) || ((nmonth == nmend) && (nday > ndayend)) )    // check for the end date
                                         break;

                            if( (nmonth > month(start)) || ((nmonth == month(start)) && (nday >= day(start)) )  ){        // the record is within required range. nmonth is the month, nday is the day of the month, xzone is the zone we are currently emitting

// we have something to output.
                                    tmElements_t tm;   tm.Day = nday;  tm.Month = nmonth; tm.Year = nyear - 1970;  tm.Hour = nhour;  tm.Minute = nminute;  tm.Second = 0;
									Schedule	sched;

									if( nschedule == 100 )
									{
										strcpy_P(sched.name, PSTR("Quick Schedule"));
									}
									else 
									{
										memset(&(sched.name), 0, sizeof(sched.name));
										LoadSchedule(uint8_t(nschedule), &sched);
									}
                            
                                    fprintf_P(stream_file, PSTR("%s \n\t\t\t\t\t { \"date\":%lu, \"duration\":%u, \"water_used\":%u, \"scheduleID\":%i, \"scheduleName\":\"%s\", \"seasonal\":%i, \"wunderground\":%i}"),
                                                                       bFirstRow ? "":",",
                                                                       makeTime(tm), nduration, nwater_used, nschedule, sched.name, nsadj, nwunderground );

                                     bFirstRow = false;
                            }
                     }   // while
                     lfile.close();
                }
				else
				{
//					TRACE_ERROR(F("TableSchedule - cannot open log file %s\n"), tmp_buf);
				}

        return true;
#endif //HW_ENABLE_SD
}


// Local worker routine
// Emit directory listing
//
void emitDirectoryListing(SdFile &dir, char *folder, FILE *pFile)
{
#ifndef HW_ENABLE_SD
	  return;
#else
//	TRACE_ERROR(F("Serving directory listing of %s\n"), folder);
	
	fprintf_P( pFile, PSTR("<table><tr><td><b>File Name</b></td> <td>&nbsp&nbsp</td> <td><b>Size, bytes</b></td></tr>\n"));

	SdFile entry;
	char fname[20];

	while(true) 
	{
        if (!entry.openNext(&dir, O_READ) ){
		// no more files
			dir.close();
            fprintf_P( pFile, PSTR("</table>"));
            return;          // all done, exiting
        }

        entry.getFilename(fname);
        fprintf_P( pFile, PSTR("<tr> <td> <a href=\".%s/%s\">%s</a></td><td>&nbsp&nbsp</td><td>%lu</td></tr>"), folder, fname, fname, entry.fileSize() ); 

        entry.close();
   }
   return;
#endif //HW_ENABLE_SD
}

// "/logs*" URL handler.
// This handler provides access and WEB UI management for various logs.
// This includes directory listing, displaying individual log files, and deleting unwanted log files
//

void Logging::LogsHandler(char *sPage, FILE *pFile, EthernetClient client)
{
#ifndef HW_ENABLE_SD
	  return;
#else 
//   let's check what is it - log listing or a specific log file request

   if( sPage[4] == 0 || sPage[4] == ' ' || (sPage[4] == '/' && sPage[5] == 0)){    // this is log listing - the string is either /logs or /logs/

// this is log listing request
        SdFile logfile;

        if( !logfile.open(sPage, O_READ) ){

            TRACE_ERROR(F("Cannot open logs directory\n"));
            Serve404(pFile);
            return;    // failed to open logs directory
        }
        
		ServeHeader(pFile, 200, PSTR("OK"), false);  // note: no caching on logs directory rendering        
        fprintf_P( pFile, PSTR("<html>\n<head>\n<title>SmartGarden Logs</title></head>\n<body>\n<div style=\"text-align: center\"><h2>SmartGarden System</h2>\n<h3>Directory listing of /logs</h3></div>\n"));
		fprintf_P( pFile, PSTR("<p><b>System Logs:</b><p>\n")); 
        
		emitDirectoryListing(logfile, "/logs", pFile);
        logfile.close();

		fprintf_P( pFile, PSTR( "<p><p><table><tr><td><b>Other logs:</b></td></tr>"
								"<tr><td><a href=\"/logs" WATERING_LOG_DIR "\">Watering Logs</a></td></tr>\n"
								"<tr><td><a href=\"/logs" TEMPERATURE_LOG_DIR "\">Temperature Sensors</a></td></tr>\n"
								"<tr><td><a href=\"/logs" HUMIDITY_LOG_DIR "\">Humidity Sensors</a></td></tr>\n"
								"<tr><td><a href=\"/logs" PRESSURE_LOG_DIR "\">Air Pressure Sensors</a></td></tr>\n"
								"<tr><td><a href=\"/logs" WFLOW_LOG_DIR "\">Waterflow Counters</a></td></tr>\n"
								"</table>\n"
								"<p><br><div style=\"text-align: center\">(c) 2015 Tony-osp</div></p>\n</body>\n</html>")); 
   }
   else if( sPage[4] != 0 && sPage[5] != 0 )	// path is longer than /logs
   {
	   char *path = sPage+4;

	   if( (strncmp_P(path, PSTR(WATERING_LOG_DIR), WATERING_LOG_DIR_LEN)==0) ||
		   (strncmp_P(path, PSTR(TEMPERATURE_LOG_DIR), TEMPERATURE_LOG_DIR_LEN)==0) ||
	       (strncmp_P(path, PSTR(HUMIDITY_LOG_DIR), HUMIDITY_LOG_DIR_LEN)==0) ||
	       (strncmp_P(path, PSTR(PRESSURE_LOG_DIR), PRESSURE_LOG_DIR_LEN)==0) ||
		   (strncmp_P(path, PSTR(WFLOW_LOG_DIR), WFLOW_LOG_DIR_LEN)==0) 	  ||
		   (strncmp_P(path, PSTR(SYSTEM_LOG_DIR), SYSTEM_LOG_DIR_LEN)==0) )
		{
			;
	    }
	   else
	   {
			path = sPage;
	   }


		SdFile logfile;

		if( !logfile.open(path, O_READ) ){

			TRACE_ERROR(F("Cannot open %s file or directory\n"), path);
		    Serve404(pFile);
			return;    // failed to open logs directory
		}

		if( logfile.isDir() )
		{
			ServeHeader(pFile, 200, PSTR("OK"), false);  // note: no caching on logs directory rendering
			fprintf_P( pFile, PSTR("<html>\n<head>\n<title>SmartGarden Logs</title></head>\n<body>\n<div style=\"text-align: center\"><h2>SmartGarden System</h2>\n<h3>Directory listing of %s</h3></div>\n"), sPage);
			
			emitDirectoryListing(logfile, path, pFile);
			logfile.close();
			
			fprintf_P( pFile, PSTR( "<p><br><div style=\"text-align: center\">(c) 2015 Tony-osp</div></p>\n</body>\n</html>"));
	   }
	   else {         // this is a request to an individual log file

//			TRACE_ERROR(F("Serving log file: %s\n"), path);

			ServeFile(pFile, sPage, logfile, client);
			logfile.close();
	   }
   }
#endif //HW_ENABLE_SD
}

// emit sensor log as JSON
bool Logging::EmitSensorLog(FILE* stream_file, time_t start, time_t end, char sensor_type, int sensor_id, char summary_type)
{
#ifndef HW_ENABLE_SD
	  return false;
#else 
        char tmp_buf[MAX_LOG_RECORD_SIZE];
        char *sensor_name;

        if (start == 0)
                start = now();

        end = max(start,end) + 24*3600;  // add 1 day to end time.

        int    nyear, nyearend=year(end), nyearstart=year(start);
        int    nmonth, nmend = month(end), nmstart=month(start);
        int    ndayend = day(end), ndaystart=day(start);

        char bFirstRow = true, bHeader = true;

//  TRACE_ERROR(F("EmitSensorLog - entering, nyearstart=%d, nmstart=%d, ndaystart=%d, nyearend=%d, nmend=%d, ndayend=%d\n"), nyearstart, nmstart, ndaystart, nyearend, nmend, ndayend );

        fprintf_P(stream_file, PSTR("\"series\": ["));   // JSON opening header

        for( nyear=nyearstart; nyear<=nyearend; nyear++ )
        {
          for( nmonth=nmstart; nmonth<=nmend; nmonth++ )
            {

//  TRACE_ERROR(F("EmitSensorLog - processing month=%d\n"), nmonth );

                if( sensor_type == SENSOR_TYPE_TEMPERATURE )
                {
                       sprintf_P(tmp_buf, PSTR(TEMPERATURE_LOG_FNAME_FORMAT), nmonth, nyear%100, sensor_id );
                       sensor_name = PSTR("Temperature");
                }
                else if( sensor_type == SENSOR_TYPE_PRESSURE )
                {
                       sprintf_P(tmp_buf, PSTR(PRESSURE_LOG_FNAME_FORMAT), nmonth, nyear%100, sensor_id );
                       sensor_name = PSTR("Air Pressure");
                }
                else if( sensor_type == SENSOR_TYPE_HUMIDITY )
                {
                       sprintf_P(tmp_buf, PSTR(HUMIDITY_LOG_FNAME_FORMAT), nmonth, nyear%100, sensor_id );
                       sensor_name = PSTR("Humidity");
                }
                else  
                {
                     SYSEVT_ERROR(F("EmitSensorLog - requested sensor type not recognized\n"));
                     return false;
                }

                if( lfile.open(tmp_buf, O_READ) )  // logs for each zone are stored in a separate file, with the file name based on the year and sensor ID. Try to open it.
                {

// OK, we opened the data file.
                
                    long int  sensor_sum = 0;
                    long int  sensor_c = 0;
                    int          sensor_stamp = -1;
                    int          sensor_stamp_h, sensor_stamp_d, sensor_stamp_m, sensor_stamp_y;                    

                    sensor_stamp_h = -1, sensor_stamp_d = sensor_stamp_m = sensor_stamp_y = -1;

                    lfile.fgets(tmp_buf, MAX_LOG_RECORD_SIZE-1);  // skip first line in the file - column headers

// OK, we opened required watering log file. Iterate over records, filtering out necessary dates range
                  
                     while( lfile.available() ){

                            int  nday = 0, nhour = 0, nminute = 0;
                            int  sensor_reading = 0;

                            int bytes = lfile.fgets(tmp_buf, MAX_LOG_RECORD_SIZE);
                            if (bytes <= 0)
                                       break;

// Parse the string into fields. First field (up to two digits) is the day of the month

                            sscanf_P( tmp_buf, PSTR("%u,%u:%u,%d"),
                                                            &nday, &nhour, &nminute, &sensor_reading);

                            if( (nmonth > nmend) || ((nmonth == nmend) && (nday > ndayend)) )    // check for the end date
                                         break;

                            if( (nmonth > nmstart) || ((nmonth == nmstart) && (nday >= ndaystart) )  ){        // the record is within required range. nmonth is the month, nday is the day of the month, xzone is the zone we are currently emitting

// we have something to output.

                                    if( bHeader ){
                                      
                                         fprintf_P(stream_file, PSTR("{\n\t\t\t \"name\": \"%S readings, Sensor: %d\", \n\t\t\t\t \"data\": [\n"), sensor_name, sensor_id);   // JSON series header
                                         bHeader = false;
                                         bFirstRow = true;
                                    }

                                    if( summary_type == LOG_SUMMARY_HOUR )
                                    {
                                           if( sensor_stamp == -1 )    // this is the very first record, start generating summary
                                           {
                                                 sensor_sum = sensor_reading;
                                                 sensor_c      = 1;
                                                 sensor_stamp = nhour;
                                                 sensor_stamp_d = nday;  sensor_stamp_m = nmonth; sensor_stamp_y = nyear;
                                           }
                                           else if( (sensor_stamp == nhour) && (sensor_stamp_d == nday) && (sensor_stamp_m == nmonth) && (sensor_stamp_y == nyear) )   // continue accumulation current sum
                                           {                                             
                                                 sensor_sum += sensor_reading;
                                                 sensor_c++;
                                           }
                                           else   // close previous sum and start a new one
                                           {
                                                int sensor_average = int(sensor_sum/sensor_c);

                                                tmElements_t tm;   tm.Day = sensor_stamp_d;  tm.Month = sensor_stamp_m; tm.Year = sensor_stamp_y - 1970;  tm.Hour = sensor_stamp;  tm.Minute = 0;  tm.Second = 0;
                                                fprintf_P(stream_file, PSTR("%s \n\t\t\t\t\t [ %lu000, %d ]"),
                                                                                              bFirstRow ? "":",",
                                                                                              makeTime(tm), sensor_average );   // note: month should be in JavaScript format (starting from 0)
                                                bFirstRow = false;
   
                                                 sensor_sum = sensor_reading;   // start new sum
                                                 sensor_c      = 1;
                                                 sensor_stamp = nhour;
                                                 sensor_stamp_d = nday;  sensor_stamp_m = nmonth; sensor_stamp_y = nyear;
                                           }
                                    }
                                    else if( summary_type == LOG_SUMMARY_DAY )
                                    {
                                           if( sensor_stamp == -1 )    // this is the very first record, start generating summary
                                           {
                                                 sensor_sum = sensor_reading;
                                                 sensor_c      = 1;
                                                 sensor_stamp = nday;
                                                 sensor_stamp_m = nmonth; sensor_stamp_y = nyear;
                                           }
                                           else if( (sensor_stamp == nday) && (sensor_stamp_m == nmonth) && (sensor_stamp_y == nyear) )   // continue accumulation current sum
                                           {                                             
                                                 sensor_sum += sensor_reading;
                                                 sensor_c++;
                                           }
                                           else   // close previous sum and start a new one
                                           {
                                                int sensor_average = (int)(sensor_sum/sensor_c);
                                                
                                                tmElements_t tm;   tm.Day = sensor_stamp;  tm.Month = sensor_stamp_m; tm.Year = sensor_stamp_y - 1970;  tm.Hour = 0;  tm.Minute = 0;  tm.Second = 0;

                                                fprintf_P(stream_file, PSTR("%s \n\t\t\t\t\t [ %lu000, %d ]"),
                                                                                              bFirstRow ? "":",",
                                                                                              makeTime(tm), sensor_average );   // note: month should be in JavaScript format (starting from 0)

                                                bFirstRow = false;
   
                                                 sensor_sum = sensor_reading;   // start new sum
                                                 sensor_c      = 1;
                                                 sensor_stamp = nday;
                                                 sensor_stamp_m = nmonth; sensor_stamp_y = nyear;
                                           }
                                    }
                                    else if( summary_type == LOG_SUMMARY_MONTH )
                                    {
                                           if( sensor_stamp == -1 )    // this is the very first record, start generating summary
                                           {
                                                 sensor_sum = sensor_reading;
                                                 sensor_c      = 1;
                                                 sensor_stamp = nmonth;
                                                 sensor_stamp_y = nyear;
                                           }
                                           else if( (sensor_stamp == nmonth) && (sensor_stamp_y == nyear) )   // continue accumulation current sum
                                           {                                             
                                                 sensor_sum += sensor_reading;
                                                 sensor_c++;
                                           }
                                           else   // close previous sum and start a new one
                                           {
                                                int sensor_average = int(sensor_sum/sensor_c);

                                                tmElements_t tm;   tm.Day = 0;  tm.Month = sensor_stamp; tm.Year = sensor_stamp_y - 1970;  tm.Hour = 0;  tm.Minute = 0;  tm.Second = 0;
                                                fprintf_P(stream_file, PSTR("%s \n\t\t\t\t\t [ %lu000, %d ]"),
                                                                                              bFirstRow ? "":",",
                                                                                              makeTime(tm), sensor_average );  
                                                bFirstRow = false;
   
                                                 sensor_sum = sensor_reading;   // start new sum
                                                 sensor_c      = 1;
                                                 sensor_stamp = nmonth;
                                                 sensor_stamp_y = nyear;
                                           }
                                    }
                                    else  
                                    {  // no summarization, just output readings as-is

                                                tmElements_t tm;   tm.Day = nday;  tm.Month = nmonth; tm.Year = nyear - 1970;  tm.Hour = nhour;  tm.Minute = nminute;  tm.Second = 0;
                                                fprintf_P(stream_file, PSTR("%s \n\t\t\t\t\t [ %lu000, %d ]"),
                                                                                              bFirstRow ? "":",",
                                                                                              makeTime(tm), sensor_reading );  
                                    
                                                bFirstRow = false;
                                    }
                            }  
                     }   // while
                     lfile.close();
                }  // file open
            }  //for( nmonth=nmstart; nmonth<=nmend; nmonth++ )
        }  //for( nyear=nyearstart; nyear<=nyearend; nyear++ )

        if( !bHeader )   // header flag was reset, it means we output at least one line
        {
               fprintf_P(stream_file, PSTR("\n\t\t\t\t ] \n \t }]\n"));
        }

    return true; 
#endif //HW_ENABLE_SD
}


