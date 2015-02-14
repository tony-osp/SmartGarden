/*

Logging sub-system implementation for Sprinklers control program. This module handles both regular system logging
as well as temperature/humidity/waterflow/etc logging.

  This module is intended to be used with my multi-station environment monitoring and sprinklers control system (SmartGarden),
  as well as modified version of the OpenSprinkler code and with modified sprinkler control program sprinklers_pi.

Log file format is described in log_format2.txt file.


 Creative Commons Attribution-ShareAlike 3.0 license
 Copyright 2014 tony-osp (http://tony-osp.dreamwidth.org/)

*/

#define __STDC_FORMAT_MACROS
#include "sdlog.h"
#include "port.h"
#include "settings.h"

#define CL_TMPB_SIZE  256    // size of the local temporary buffer



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

   if( flag ){   // progmem or regular string

      if( strlen_P(str) > (CL_TMPB_SIZE-20) ) return false;   // input string too long, reject it. Note: we need almost 20 bytes for the date/time etc
   }
   else {
      if( strlen(str) > (CL_TMPB_SIZE-20) ) return false;   // input string too long, reject it. Note: we need almost 20 bytes for the date/time etc
   }

   if( flag ){

      sprintf_P(tmp_buf, PSTR("%u,%u:%u:%u,"), day(t), hour(t), minute(t), second(t) );

//        sendLogStr(tmp_buf);

// Because this is PROGMEM string we have to output it manually, one char at a time because we don't want to pre-allocate space for the whole string (RAM is a scarse resource!). But SdFat library will buffer output anyway, so it is OK.
// SdFat will flush the buffer on close().
      char c;
      while((c = pgm_read_byte(str++))){

//                      sendLogChar(c);
      }
//              sendLogChar('\n');
   }
   else {

      sprintf_P(tmp_buf, PSTR("%u,%u:%u:%u,"), day(t), hour(t), minute(t), second(t) );

//        sendLogStr(tmp_buf);
//        sendLogStr(str);
   }

   return true;
}


// Record watering event
//
// Note: for watering events we open/close file on each event

#define MAX_WATERING_LOG_RECORD_SIZE    80

bool Logging::LogZoneEvent(time_t start, int zone, int duration, int schedule, int sadj, int wunderground)
{
      time_t t = now();
// temp buffer for log strings processing
      char tmp_buf[MAX_WATERING_LOG_RECORD_SIZE];

      sprintf_P(tmp_buf, PSTR("%u,%u,%u:%u,%u,%u,%i,%i"), month(start), day(start), hour(start), minute(start), duration, schedule, sadj, wunderground);

//        sendLogStr(tmp_buf);

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
bool Logging::LogSensorReading(char sensor_type, int sensor_id, int sensor_reading)
{
//    trace(F("LogSensorReading - enter\n"));

      time_t t = now();

// temp buffer for log strings processing
      char tmp_buf[20];

      sprintf_P(tmp_buf, PSTR("%u,%u:%u,%d"), day(t), hour(t), minute(t), sensor_reading);
//        sendLogStr(tmp_buf);

      return true;    // standard exit-success
}


