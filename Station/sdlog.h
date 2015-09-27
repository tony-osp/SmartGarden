/*

Logging sub-system implementation for Sprinklers control program. This module handles both regular system logging
as well as temperature/humidity/waterflow/etc logging.

Note: Log operation signature is implemented to be compatible with the Sql-based logging in sprinklers_pi control program.



Creative Commons Attribution-ShareAlike 3.0 license
Copyright 2014 tony-osp (http://tony-osp.dreamwidth.org/)

*/



#ifndef SD_LOG_H_
#define SD_LOG_H_	

#include "port.h"
#include <Time.h>
#include "SGRProtocol.h"

//
// Log directories
//

// System log directory and file name format (mm-yyyy.log)
#define SYSTEM_LOG_DIR			"/logs"
#define SYSTEM_LOG_DIR_LEN		5
#define SYSTEM_LOG_FNAME_FORMAT "/logs/%2.2u-%4.4u.log"

// Watering activity log directory and file name format (wat-yyyy.nnn)
#define WATERING_LOG_DIR			"/watering.log"
#define WATERING_LOG_DIR_LEN		13
#define WATERING_LOG_FNAME_FORMAT "/watering.log/wat-%4.4u.%3.3u"
#define WATERING_SCH_LOG_FNAME_FORMAT "/watering.log/wat-%4.4u.sch"

// Water flow data directory and file name format (wflMM-YY.nnn)
#define WFLOW_LOG_DIR			"/wflow.log"
#define WFLOW_LOG_DIR_LEN		10
#define WFLOW_LOG_FNAME_FORMAT "/wflow.log/wfl%2.2u-%2.2u.%3.3u"

// Temperature data directory and file name format (temMM-YY.nnn)
#define TEMPERATURE_LOG_DIR		 	 "/tempr.log"
#define TEMPERATURE_LOG_DIR_LEN	 	 10
#define TEMPERATURE_LOG_FNAME_FORMAT "/tempr.log/tem%2.2u-%2.2u.%3.3u"

// Humidity data directory and file name format (humMM-YY.nnn)
#define HUMIDITY_LOG_DIR		  "/humid.log"
#define HUMIDITY_LOG_DIR_LEN	  10
#define HUMIDITY_LOG_FNAME_FORMAT "/humid.log/hum%2.2u-%2.2u.%3.3u"

// Atmospheric pressure data directory and file name format (preMM-YY.nnn)
#define PRESSURE_LOG_DIR		  "/pressure.log"
#define PRESSURE_LOG_DIR_LEN	  13
#define PRESSURE_LOG_FNAME_FORMAT "/pressure.log/pre%2.2u-%2.2u.%3.3u"


#ifdef notdef

// ***NOTE***
// Because Event codes are shared with the wire protocol, definitions are in SGRProtocol.h

//
// Event types for system log
//
#define SYSEVENT_CRIT					0	// critical event
#define SYSEVENT_ERROR					1
#define SYSEVENT_WARNING				2
#define SYSEVENT_INFO					3
#define SYSEVENT_VERBOSE				4	// verbose info message

#endif

// SYSEVT_LEVEL defines the level of events in system log. I have separate log macro for each trace type, and depending on the current SYSEVT_LEVEL
//  some of these macros (or all of them) will be enabled.
//
//	2 ==	Critical messages only (SYSEVT_CRIT)
//  3 ==	Critical messages and errors (SYSEVT_ERROR)
//  4 ==	Critical messages, errors and warnings (SYSEVT_WARNING)
//  5 ==	Critical messages, errors and warnings (SYSEVT_NOTICE)
//  6 ==	Critical messages, errors, warnings and informational messages (SYSEVT_INFO)
//  7 ==	Critical messages, errors, warnings, infrmational messages and verbose tracing (SYSEVT_VERBOSE)

#if SYSEVT_LEVEL <= SYSEVENT_CRIT
#define SYSEVT_CRIT(...)	syslog_evt(SYSEVENT_CRIT, __VA_ARGS__);
#define SYSEVT_ERROR(...)	TRACE_ERROR(__VA_ARGS__); 
#define SYSEVT_WARNING(...) TRACE_WARNING(__VA_ARGS__); 
#define SYSEVT_NOTICE(...)	TRACE_NOTICE(__VA_ARGS__); 
#define SYSEVT_INFO(...)	TRACE_INFO(__VA_ARGS__); 
#define SYSEVT_VERBOSE(...) TRACE_VERBOSE(__VA_ARGS__); 
#elif SYSEVT_LEVEL == SYSEVENT_ERROR
#define SYSEVT_CRIT(...)	syslog_evt(SYSEVENT_CRIT, __VA_ARGS__);
#define SYSEVT_ERROR(...)	syslog_evt(SYSEVENT_ERROR, __VA_ARGS__);
#define SYSEVT_WARNING(...) TRACE_WARNING(__VA_ARGS__); 
#define SYSEVT_NOTICE(...)	TRACE_NOTICE(__VA_ARGS__); 
#define SYSEVT_INFO(...)	TRACE_INFO(__VA_ARGS__); 
#define SYSEVT_VERBOSE(...) TRACE_VERBOSE(__VA_ARGS__);
#elif SYSEVT_LEVEL == SYSEVENT_WARNING
#define SYSEVT_CRIT(...)	syslog_evt(SYSEVENT_CRIT, __VA_ARGS__);
#define SYSEVT_ERROR(...)	syslog_evt(SYSEVENT_ERROR, __VA_ARGS__);
#define SYSEVT_WARNING(...) syslog_evt(SYSEVENT_WARNING, __VA_ARGS__);
#define SYSEVT_NOTICE(...)	TRACE_NOTICE(__VA_ARGS__); 
#define SYSEVT_INFO(...)	TRACE_INFO(__VA_ARGS__); 
#define SYSEVT_VERBOSE(...) TRACE_VERBOSE(__VA_ARGS__); 
#elif SYSEVT_LEVEL == SYSEVENT_NOTICE
#define SYSEVT_CRIT(...)	syslog_evt(SYSEVENT_CRIT, __VA_ARGS__);
#define SYSEVT_ERROR(...)	syslog_evt(SYSEVENT_ERROR, __VA_ARGS__);
#define SYSEVT_WARNING(...) syslog_evt(SYSEVENT_WARNING, __VA_ARGS__);
#define SYSEVT_NOTICE(...)	syslog_evt(SYSEVENT_NOTICE, __VA_ARGS__);
#define SYSEVT_INFO(...)	TRACE_INFO(__VA_ARGS__); 
#define SYSEVT_VERBOSE(...) TRACE_VERBOSE(__VA_ARGS__);
#elif SYSEVT_LEVEL == SYSEVENT_INFO
#define SYSEVT_CRIT(...)	syslog_evt(SYSEVENT_CRIT, __VA_ARGS__);
#define SYSEVT_ERROR(...)	syslog_evt(SYSEVENT_ERROR, __VA_ARGS__);
#define SYSEVT_WARNING(...) syslog_evt(SYSEVENT_WARNING, __VA_ARGS__);
#define SYSEVT_NOTICE(...)	syslog_evt(SYSEVENT_NOTICE, __VA_ARGS__);
#define SYSEVT_INFO(...)	syslog_evt(SYSEVENT_INFO, __VA_ARGS__);
#define SYSEVT_VERBOSE(...) TRACE_VERBOSE(__VA_ARGS__);
#else
#define SYSEVT_CRIT(...)	syslog_evt(SYSEVENT_CRIT, __VA_ARGS__);
#define SYSEVT_ERROR(...)	syslog_evt(SYSEVENT_ERROR, __VA_ARGS__);
#define SYSEVT_WARNING(...) syslog_evt(SYSEVENT_WARNING, __VA_ARGS__);
#define SYSEVT_NOTICE(...)	syslog_evt(SYSEVENT_NOTICE, __VA_ARGS__);
#define SYSEVT_INFO(...)	syslog_evt(SYSEVENT_INFO, __VA_ARGS__);
#define SYSEVT_VERBOSE(...) syslog_evt(SYSEVENT_VERBOSE, __VA_ARGS__);
#endif

// Main Syslog event routine
void syslog_evt(uint8_t event_type, const char * fmt, ...);
void syslog_evt(uint8_t event_type, const __FlashStringHelper * fmt, ...);
//
// Summarization codes
//
#define LOG_SUMMARY_NONE				0
#define LOG_SUMMARY_HOUR				1
#define LOG_SUMMARY_DAY					2
#define LOG_SUMMARY_MONTH				3


//
// Log types
//
#define LOG_TYPE_SYSTEM					1
#define LOG_TYPE_WATERING				2
#define LOG_TYPE_WATERFLOW				3
#define LOG_TYPE_TEMPERATURE			4
#define LOG_TYPE_HUMIDITY				5
#define LOG_TYPE_PRESSURE				6



class Logging
{
public:
        enum GROUPING {NONE, HOURLY, DAILY, MONTHLY};
        Logging();
        ~Logging();
        bool begin(void);
        void Close();
        // Watering activity logging. Note: signature is deliberately compatible with sprinklers_pi control program
        bool LogZoneEvent(time_t start, int zone, int duration, int schedule, int sadj, int wunderground);
		// Log whole schedule event
		bool LogSchedEvent(time_t start, int duration, int schedule, int sadj, int wunderground);

        // Retrieve data suitable for graphing
        bool GraphZone(FILE * stream_file, time_t start, time_t end, GROUPING group);

        // Emit zone watering data suitable for putting into a table
        bool TableZone(FILE* stream_file, time_t start, time_t end);

        // Emit schedue watering data suitable for putting into a table
        bool TableSchedule(FILE* stream_file, time_t start, time_t end);

        // Sensors logging. It covers all types of basic sensors (e.g. temperature, pressure etc) that provide momentarily (immediate) readings
        bool LogSensorReading(uint8_t sensor_type, int sensor_id, int32_t sensor_reading);

	bool EmitSensorLog(FILE* stream_file, time_t sdate, time_t edate, char sensor_type, int sensor_id, char summary_type);
        
        void HandleWebRq(char *sPage, FILE *pFile);
		void LogsHandler(char *sPage, FILE *stream_file, EthernetClient client);

// Data
		bool	logger_ready;
		SdFile  lfile;

private:


        int getZoneBins( int zone, time_t start, time_t end, long int *bin_data, int bins, GROUPING grouping);

};

extern Logging sdlog;


#endif //SD_LOG_H_
