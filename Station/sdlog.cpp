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
#include "port.h"
#include "settings.h"

extern SdFat sd;

#define CL_TMPB_SIZE  256    // size of the local temporary buffer

// Local forward declarations



Logging::Logging()
{
// initialize internal state

  logger_ready = false;

}

Logging::~Logging()
{
;
}


// Start logging - open/create system log file, create initial "start" record. Note: uses time/date data, time/date should be available by now
// Takes input string that will be used in the first log record
// Returns true on success and false on failure
//


bool Logging::begin(char *str)
{
#ifndef HW_ENABLE_SD
	return false;
#endif //!HW_ENABLE_SD

  char    log_fname[20];
  time_t  curr_time = now();

// Ensure log folders are there, if not - create it.

  sprintf_P(log_fname, PSTR(SYSTEM_LOG_DIR));   // system log directory
  if( !lfile.open(log_fname, O_READ) ){

        trace(F("System log directory not found, creating it.\n"));

        if( !sd.mkdir(log_fname) ){

           trace(F("Error creating System log directory.\n"));
        }
  }
  lfile.close();      // close the directory

  sprintf_P(log_fname, PSTR(WATERING_LOG_DIR));   // Watering log directory
  if( !lfile.open(log_fname, O_READ) ){

        trace(F("Watering log directory not found, creating it.\n"));

        if( !sd.mkdir(log_fname) ){

           trace(F("Error creating Watering log directory.\n"));
        }
  }
  lfile.close();      // close the directory

  sprintf_P(log_fname, PSTR(WFLOW_LOG_DIR));   // Waterflow log directory
  if( !lfile.open(log_fname, O_READ) ){

        trace(F("Waterflow log directory not found, creating it.\n"));

        if( !sd.mkdir(log_fname) ){

           trace(F("Error creating Waterflow log directory.\n"));
        }
  }
  lfile.close();      // close the directory

  sprintf_P(log_fname, PSTR(TEMPERATURE_LOG_DIR));   // Temperature log directory
  if( !lfile.open(log_fname, O_READ) ){

        trace(F("Temperature log directory not found, creating it.\n"));

        if( !sd.mkdir(log_fname) ){

           trace(F("Error creating Temperature log directory.\n"));
        }
  }
  lfile.close();      // close the directory

  sprintf_P(log_fname, PSTR(HUMIDITY_LOG_DIR));   // Humidity log directory
  if( !lfile.open(log_fname, O_READ) ){

        trace(F("Humidity log directory not found, creating it.\n"));

        if( !sd.mkdir(log_fname) ){

           trace(F("Error creating Humidity log directory.\n"));
        }
  }
  lfile.close();      // close the directory

  sprintf_P(log_fname, PSTR(PRESSURE_LOG_DIR));   // Atmospheric pressure log directory
  if( !lfile.open(log_fname, O_READ) ){

        trace(F("Humidity log directory not found, creating it.\n"));

        if( !sd.mkdir(log_fname) ){

           trace(F("Error creating Pressure log directory.\n"));
        }
  }
  lfile.close();      // close the directory


//  generate system log file name
  sprintf_P(log_fname, PSTR(SYSTEM_LOG_FNAME_FORMAT), month(curr_time), year(curr_time) );
  if( !lfile.open(log_fname, O_WRITE | O_APPEND | O_CREAT) ){

        trace(F("Cannot open system log file (%s)\n"), log_fname);
        logger_ready = false;

        return false;    // failed to open/create log file
  }

  lfile.close();
  logger_ready = true;      // we are good to go

  return syslog_str_P(SYSEVENT_INFO, str);    // add system log record using the string provided.
}

void Logging::Close()
{
   logger_ready = false;
}



//
// internal helpers
//

// Add new log record
// Takes input string, returns true on success and false on failure
//
byte Logging::syslog_str(char evt_type, char *str)
{
   return syslog_str_internal(evt_type, str, false);
}

// Add new log record
// Takes input string FROM PROGRAM MEMORY, returns true on success and false on failure
//
byte Logging::syslog_str_P(char evt_type, char *str)
{
   return syslog_str_internal(evt_type, str, true);
}


// Internal worker for syslog_str
// First parameter is the string, second parameter is a flag indicating string location
//     false == RAM
//     true   == PROGMEM
//

// Note: We open/write/close log file on each event. It is kind of expensive, but it allows to keep RAM
//               usage low. Also close operation flushes buffers (acts as sync()).
//
byte Logging::syslog_str_internal(char evt_type, char *str, char flag)
{
   time_t t = now();
// temp buffer for log strings processing
   char tmp_buf[20];

   if( !logger_ready ) return false;  //check if the logger is ready

   sprintf_P(tmp_buf, PSTR(SYSTEM_LOG_FNAME_FORMAT), month(t), year(t) );

   if( !lfile.open(tmp_buf, O_WRITE | O_APPEND | O_CREAT) ){

            trace(F("Cannot open system log file (%s)\n"), tmp_buf);

            logger_ready = false;      // something is wrong with the log file, mark logger as "not ready"
            return false;    // failed to open/create log file
   }

   if( flag ){   // progmem or regular string

      if( strlen_P(str) > (CL_TMPB_SIZE-20) ) return false;   // input string too long, reject it. Note: we need almost 20 bytes for the date/time etc
   }
   else {
      if( strlen(str) > (CL_TMPB_SIZE-20) ) return false;   // input string too long, reject it. Note: we need almost 20 bytes for the date/time etc
   }

   if( flag ){

      sprintf_P(tmp_buf, PSTR("%u,%u:%u:%u,"), day(t), hour(t), minute(t), second(t) );
      lfile.print(tmp_buf);

// Because this is PROGMEM string we have to output it manually, one char at a time because we don't want to pre-allocate space for the whole string (RAM is a scarse resource!). But SdFat library will buffer output anyway, so it is OK.
// SdFat will flush the buffer on close().
      char c;
      while((c = pgm_read_byte(str++)))
         lfile.write(c);

	  lfile.write('\n');
   }
   else {

      sprintf_P(tmp_buf, PSTR("%u,%u:%u:%u,"), day(t), hour(t), minute(t), second(t) );
      lfile.print(tmp_buf);
      lfile.println(str);
   }

   lfile.close();
   return true;
}


// Record watering event
//
// Note: for watering events we open/close file on each event

bool Logging::LogZoneEvent(time_t start, int zone, int duration, int schedule, int sadj, int wunderground)
{
   if( !logger_ready ) return false;  //check if the logger is ready
	
	  time_t t = now();
// temp buffer for log strings processing
      char tmp_buf[MAX_LOG_RECORD_SIZE];

      sprintf_P(tmp_buf, PSTR(WATERING_LOG_FNAME_FORMAT), year(t), zone );

      if( !lfile.open(tmp_buf, O_WRITE | O_APPEND) ){    // we are trying to open existing log file for write/append

// operation failed, usually because log file for this year does not exist yet. Let's create it and add column headers.
         if( !lfile.open(tmp_buf, O_WRITE | O_APPEND | O_CREAT) ){

               trace(F("Cannot open watering log file (%s)\n"), tmp_buf);    // file create failed, return an error.
               return false;    // failed to open/create file
         }
         lfile.println(F("Month,Day,Time,Run time(min),ScheduleID,Adjustment,WUAdjustment"));
      }

      sprintf_P(tmp_buf, PSTR("%u,%u,%u:%u,%u,%u,%i,%i"), month(start), day(start), hour(start), minute(start), duration, schedule, sadj, wunderground);

      lfile.println(tmp_buf);
      lfile.close();

      return true;
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
bool Logging::LogSensorReading(uint8_t sensor_type, int sensor_id, int sensor_reading)
{
//	trace(F("LogSensorReading - enter, sensor_type=%i, sensor_id=%i, sensor_reading=%i\n"), (int)sensor_type, sensor_id, sensor_reading);

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
      
            default:
                     return false;    // sensor_type not recognized
                     break;           
      }
//      trace(F("LogSensorReading - about to open file: %s, len=%d\n"), tmp_buf, strlen(tmp_buf));

      if( !lfile.open(tmp_buf, O_WRITE | O_APPEND) ){    // we are trying to open existing log file for write/append

// operation failed, usually because log file for this year does not exist yet. Let's create it and add column headers.
         if( !lfile.open(tmp_buf, O_WRITE | O_APPEND | O_CREAT) ){

               trace(F("Cannot open or create sensor  log file %s\n"), tmp_buf);    // file create failed, return an error.
               return false;    // failed to open/create file
         }
		 // write header line
         sprintf_P(tmp_buf, PSTR("Day,Time,%S\n"), sensorName); 
		 lfile.write(tmp_buf, strlen(tmp_buf));
         
         trace(F("creating new log file for sensor:%S\n"), sensorName);
      }
//	  trace(F("Opened log file %s, len=%i\n"), tmp_buf, strlen(tmp_buf));

      sprintf_P(tmp_buf, PSTR("%u,%u:%u,%d\n"), day(t), hour(t), minute(t), sensor_reading);

//	  trace(F("Writing log string %s, len=%d\n"), tmp_buf, strlen(tmp_buf));
	  lfile.write(tmp_buf, strlen(tmp_buf));

      lfile.close();

      return true;    // standard exit-success
}


bool Logging::GraphZone(FILE* stream_file, time_t start, time_t end, GROUPING grouping)
{
        grouping = max(NONE, min(grouping, MONTHLY));
        char		bins = 0;
        uint32_t	bin_offset = 0;
        uint32_t	bin_scale = 1;

        switch (grouping)
        {
        case HOURLY:
                bins = 24;
                break;

        case DAILY:
                bins = 7;
                break;

        case MONTHLY:
                bins = 12;
                break;

        case NONE:
                bins = 10;
                bin_offset = start;
                bin_scale = (end-start)/bins;
                break;
        }

        int current_zone = -1;
        bool bFirstZone = true;
        long int bin_data[24];		// maximum bin size is 24

        if (start == 0)
                start = now();

        int    nyear=year(start);

        end = max(start,end) + 24*3600;  // add 1 day to end time.

        int curr_zone = 255;
        char bFirstRow = true;

		uint8_t	n_zones = GetNumZones();
        for( int xzone = 1; xzone <= n_zones; xzone++ ){  // iterate over zones

                    int bin_res = getZoneBins( xzone, start, end, bin_data, bins, grouping);
                    if( bin_res > 0 ){  // some data available
                    
                                    if( curr_zone != xzone ){
                                      
                                         if( curr_zone != 255 ) 
                                                   fprintf_P(stream_file, PSTR("], "));   // if this is not the first zone, add comma to the previous one
                                         
                                         fprintf_P(stream_file, PSTR("\n\t \"%d\": ["), xzone);   // JSON zone header
                                         curr_zone = xzone;
                                         bFirstRow = true;
                                    }

                                     bFirstRow = false;
                                     for (int i=0; i<bins; i++)
                                               fprintf(stream_file, "%s[%i, %lu]", (i==0)?"":",", i, bin_data[i]);

                    }  // if(bin_res>0)
                    
        }   // for( int xzone = 1; xzone <= xmaxzone; xzone++ )

        if( curr_zone != 255)
                     fprintf_P(stream_file, PSTR("\n\t\t\t\t\t ] \n"));    // close the last zone if we emitted

        return true;
}

int Logging::getZoneBins( int zone, time_t start, time_t end, long int bin_data[], int bins, GROUPING grouping)
{
        char tmp_buf[MAX_LOG_RECORD_SIZE];
        int    bin_counter[24];
        int    r_counter = 0;
        
//		trace(F("getZoneBins - entering, zone=%d, bins=%d, grouping=%d\n"), zone, bins, (int)grouping);
//		freeMemory();
		if( grouping == NONE )		// I have not implemented "no grouping" case yet
			return -2;

        memset( bin_counter, 0, bins*sizeof(int) );
        memset( bin_data, 0, bins*sizeof(long int) );

        if (start == 0)
                start = now();

        int    nyear=year(start);

        end = max(start,end) + 24*3600;  // add 1 day to end time.

        unsigned int  nmend = month(end);
        unsigned int  ndayend = day(end);

        if( year(end) != year(start) ){     // currently we cannot handle queries that span multiple years. Truncate the query to the year end.

             nmend = 12;    ndayend = 31;
        }
        
        sprintf_P(tmp_buf, PSTR(WATERING_LOG_FNAME_FORMAT), nyear, zone );

        if( !lfile.open(tmp_buf, O_READ) ){  // logs for each zone are stored in a separate file, with the file name based on the year and zone number. Try to open it.

//			trace(F("getZoneBins - cannot open watering log file: %s\n"), tmp_buf);
            return -1;  // cannot open watering log file
        }
        char bFirstRow = true;

        lfile.fgets(tmp_buf, MAX_LOG_RECORD_SIZE-1);  // skip first line in the file - column headers

// OK, we opened required watering log file. Iterate over records, filtering out necessary dates range
                  
        while( lfile.available() ){

                    unsigned int nhour = 0, nmonth = 0, nday = 0, nminute = 0;
                    int  nduration = 0, nschedule = 0;
                    int  nsadj = 0, nwunderground = 0;

                    int bytes = lfile.fgets(tmp_buf, MAX_LOG_RECORD_SIZE-1);
                    if (bytes <= 0)
                                  break;

// Parse the string into fields. First field (up to two digits) is the day of the month

                    sscanf_P( tmp_buf, PSTR("%u,%u,%u:%u,%i,%i,%i,%i"),
                                                    &nmonth, &nday, &nhour, &nminute, &nduration, &nschedule, &nsadj, &nwunderground);

                    if( (nmonth > nmend) || ((nmonth == nmend) && (nday > ndayend)) )    // check for the end date
                                 break;

                    if( (nmonth > month(start)) || ((nmonth == month(start)) && (nday >= day(start)) )  ){        // the record is within required range. nmonth is the month, nday is the day of the month, xzone is the zone we are currently emitting

                         switch (grouping)
                         {
                              case HOURLY:
                              if( nhour < 24 ){    // basic protection to ensure corrupted data will not crash the system
                     
                                   bin_data[nhour] += (long int)nduration;
                                   bin_counter[nhour]++;
                              }
                              break;

                              case DAILY:
                              {
                                       tmElements_t tm;   tm.Day = nday;  tm.Month = nmonth; tm.Year = nyear - 1970;  tm.Hour = nhour;  tm.Minute = nminute;  tm.Second = 0; 
                                       unsigned int  dow=weekday(makeTime(tm));
                                       
                                       if( dow < 7 ){    // basic protection to ensure corrupted data will not crash the system
                     
                                                 bin_data[dow] += (long int)nduration;
                                                 bin_counter[dow]++;
                                       }
                              }
                              break;

                              case MONTHLY:
                              if( nmonth < 12 ){    // basic protection to ensure corrupted data will not crash the system
                     
                                   bin_data[nmonth] += (long int)nduration;
                                   bin_counter[nmonth]++;
                              }
                              break;

                              default:
									lfile.close();
                                    return -2;  // unsupported grouping                        
                              break;
                         }  // switch(grouping)
                    }      
        } // while
        
        lfile.close();
        
//        trace(F("Zone=%i. Scaling bins, num bins=%i\n"), zone, bins);
        
        for( int i=0; i<bins; i++ ){
          
//              trace(F("bin_data[%i]=%lu, bin_counter[%i]=%i\n"), i, bin_data[i], i, bin_counter[i]);
              if( bin_counter[i] != 0 ){
                
                       bin_data[i] = bin_data[i]/(long int)bin_counter[i];   
                       r_counter ++;
              }
        }
  
        return r_counter;
}

bool Logging::TableZone(FILE* stream_file, time_t start, time_t end)
{
        char m;
        char tmp_buf[MAX_LOG_RECORD_SIZE];

        if (start == 0)
                start = now();

        unsigned int    nyear=year(start);

        end = max(start,end) + 24*3600;  // add 1 day to end time.

        unsigned int  nmend = month(end);
        unsigned int  ndayend = day(end);

//		trace(F("TableZone - entering, start year=%u, month=%u, day=%u\n"), year(start), month(start), day(start));

        if( year(end) != year(start) ){     // currently we cannot handle queries that span multiple years. Truncate the query to the year end.
             nmend = 12;    ndayend = 31;
        }

        int curr_zone = 255;
		uint8_t	n_zones = GetNumZones();
        for( int xzone = 1; xzone <= n_zones; xzone++ ){  // iterate over zones

                sprintf_P(tmp_buf, PSTR(WATERING_LOG_FNAME_FORMAT), nyear, xzone );

                if( lfile.open(tmp_buf, O_READ) ){  // logs for each zone are stored in a separate file, with the file name based on the year and zone number. Try to open it.

                    char bFirstRow = true;
                    
//                    if(xzone==1){
//                         trace(F("***Reading zone=1, year=%u, month end=%u, day end=%u***\n"), nyear, nmend, ndayend);
//                    }

                     lfile.fgets(tmp_buf, MAX_LOG_RECORD_SIZE-1);  // skip first line in the file - column headers

// OK, we opened required watering log file. Iterate over records, filtering out necessary dates range
                  
                     while( lfile.available() ){

                            int  nmonth = 0, nday = 0, nhour = 0, nminute = 0, nschedule = 0;
                            int  nduration = 0,  nsadj = 0, nwunderground = 0;

                            int bytes = lfile.fgets(tmp_buf, MAX_LOG_RECORD_SIZE-1);
                            if (bytes <= 0)
                                       break;

// Parse the string into fields. First field (up to two digits) is the day of the month

                            sscanf_P( tmp_buf, PSTR("%u,%u,%u:%u,%i,%i,%i,%i"),
                                                            &nmonth, &nday, &nhour, &nminute, &nduration, &nschedule, &nsadj, &nwunderground);

                            if( (nmonth > nmend) || ((nmonth == nmend) && (nday > ndayend)) )    // check for the end date
                                         break;

                            if( (nmonth > month(start)) || ((nmonth == month(start)) && (nday >= day(start)) )  ){        // the record is within required range. nmonth is the month, nday is the day of the month, xzone is the zone we are currently emitting

//                    if(xzone==1){
//                      
//                        trace(F("Found suitable string: %s\n"), tmp_buf);
//                    }

// we have something to output.

                                    if( curr_zone != xzone ){
                                      
                                         if( curr_zone != 255 ) 
                                                   fprintf_P(stream_file, PSTR("\n\t\t\t\t\t]\n\t\t\t\t},\n"));   // if this is not the first zone, close previous one
                                         
                                         fprintf_P(stream_file, PSTR("\n\t\t\t\t { \n\t\t\t\t \"zone\": %i,\n\t\t\t\t \"entries\": ["), xzone);   // JSON zone header
                                         curr_zone = xzone;
                                         bFirstRow = true;
                                    }

                                    tmElements_t tm;   tm.Day = nday;  tm.Month = nmonth; tm.Year = nyear - 1970;  tm.Hour = nhour;  tm.Minute = nminute;  tm.Second = 0;
                            
                                    fprintf_P(stream_file, PSTR("%s \n\t\t\t\t\t { \"date\":%lu, \"duration\":%i, \"schedule\":%i, \"seasonal\":%i, \"wunderground\":%i}"),
                                                                       bFirstRow ? "":",",
                                                                       makeTime(tm), nduration, nschedule, nsadj, nwunderground );

                                     bFirstRow = false;
                            }
                     }   // while
                     lfile.close();
                }
				else
				{
//					trace(F("TableZone - cannot open log file %s\n"), tmp_buf);
				}
        }   // for( int xzone = 1; xzone <= xmaxzone; xzone++ )

        if( curr_zone != 255)
                     fprintf_P(stream_file, PSTR("\n\t\t\t\t\t ] \n\t\t\t\t } \n"));    // close the last zone if we emitted

        return true;
}

// emit sensor log as JSON
bool Logging::EmitSensorLog(FILE* stream_file, time_t start, time_t end, char sensor_type, int sensor_id, char summary_type)
{
        char tmp_buf[MAX_LOG_RECORD_SIZE];
        char *sensor_name;

        if (start == 0)
                start = now();

        end = max(start,end) + 24*3600;  // add 1 day to end time.

        unsigned int    nyear, nyearend=year(end), nyearstart=year(start);
        unsigned int    nmonth, nmend = month(end), nmstart=month(start);
        unsigned int    ndayend = day(end), ndaystart=day(start);

        char bFirstRow = true, bHeader = true;

//  trace(F("EmitSensorLog - entering, nyearstart=%d, nmstart=%d, ndaystart=%d, nyearend=%d, nmend=%d, ndayend=%d\n"), nyearstart, nmstart, ndaystart, nyearend, nmend, ndayend );

        fprintf_P(stream_file, PSTR("\"series\": ["));   // JSON opening header

        for( nyear=nyearstart; nyear<=nyearend; nyear++ )
        {
          for( nmonth=nmstart; nmonth<=nmend; nmonth++ )
            {

//  trace(F("EmitSensorLog - processing month=%d\n"), nmonth );

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
                     trace(F("EmitSensorLog - requested sensor type not recognized\n"));
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
}


