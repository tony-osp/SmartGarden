/*

Logging sub-system implementation for Sprinklers control program. This module handles both regular system logging
as well as temperature/humidity/waterflow/etc logging.

Note: Log operation signature is implemented to be compatible with the Sql-based logging in sprinklers_pi control program.


 Creative Commons Attribution-ShareAlike 3.0 license
 Copyright 2014 tony-osp (http://tony-osp.dreamwidth.org/)


*/



#ifndef SD-LOG_H_
#define SD-LOG_H_

#include "port.h"
#include <Time.h>

//
// Event types for system log
//
#define SYSEVENT_ERROR       1
#define SYSEVENT_WARNING  2
#define SYSEVENT_INFO           3


//
// Sensor types
//
#define SENSOR_TYPE_TEMPERATURE    1
#define SENSOR_TYPE_PRESSURE            2
#define SENSOR_TYPE_HUMIDITY              3
#define SENSOR_TYPE_WATERFLOW        4

//
// Summarization codes
//
#define LOG_SUMMARY_NONE         0
#define LOG_SUMMARY_HOUR         1
#define LOG_SUMMARY_DAY            2
#define LOG_SUMMARY_MONTH      3


//
// Log types
//
#define LOG_TYPE_SYSTEM                        1
#define LOG_TYPE_WATERING                   2
#define LOG_TYPE_WATERFLOW               3
#define LOG_TYPE_TEMPERATURE           4
#define LOG_TYPE_HUMIDITY                     5
#define LOG_TYPE_PRESSURE                  6


// max log record size
#define MAX_LOG_RECORD_SIZE          32

class Logging
{
public:
        Logging();
        ~Logging();
        bool begin(char *str);
        void Close();
        // Watering activity logging. Note: signature is deliberately compatible with sprinklers_pi control program
        bool LogZoneEvent(time_t start, int zone, int duration, int schedule, int sadj, int wunderground);

        // Sensors logging. It covers all types of basic sensors (e.g. temperature, pressure etc) that provide momentarily (immediate) readings
        bool LogSensorReading(char sensor_type, int sensor_id, int sensor_reading);

        // add event to the system log with the string str
        byte syslog_str(char evt_type, char *str);
        // add event to the system log with the string str in PROGMEM
        byte syslog_str_P(char evt_type, char *str);
        
private:

        bool   logger_ready;
        
        byte syslog_str_internal(char evt_type, char *str, char flag);
};

#endif /* SD-LOG_H_ */
