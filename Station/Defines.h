/*

Defaults definition for the SmartGarden system.

The same source can be compiled to run on different versions of hardware, 
as well as to select Master or Remote station HW config.
To select specific hardware version uncomment the line below corresponding to required HW version.

Please take into account that for Master station version only baseline HW config is defined at compile time,
the rest of config (e.g. number of stations, sensors, sensor types etc) is configured dynamically, 
using device.ini file on SD card.

For Remote station version config is defined at compile time, with only limited configurability (e.g. StationID)
defined at run time using LCD UI.

Creative Commons Attribution-ShareAlike 3.0 license
Copyright 2014-2016 tony-osp (http://tony-osp.dreamwidth.org/)

*/

#ifndef _DEFINES_H
#define _DEFINES_H 

#define ENABLE_TRACE	1		// enable trace output (via Serial port)
#define TRACE_LEVEL		2		// critical and alert messages only
//#define TRACE_LEVEL			6		// all info
#define TRACE_FREERAM_LIMIT	2000	// when free RAM goes below this limit freeMem() calls will start producing critical notifications

// System events level threshold. System events are written to the system log, also they are copied to the trace output.
#define SYSEVT_LEVEL	2	// critical events only

// Supported hardware version definitions

#define HW_V10_MASTER			1	// Master station, hardware version 1.0 (Mega 2560-based)
#define HW_V15_MASTER			2	// Master station, hardware version 1.5 (Moteino Mega-based)
#define HW_V15_REMOTE			3	// Remote station, hardware version 1.5 (Moteino Mega-based)

#define HW_V16_MASTER			4	// Master station, hardware version 1.6 (Moteino Mega-based, native Moteino RF module)
#define HW_V16_REMOTE			5	// Remote station, hardware version 1.6 (Moteino Mega-based, native Moteino RF module)

//To select specific hardware version uncomment the line below corresponding to required HW version.

//#define SG_HARDWARE				HW_V15_REMOTE
//#define SG_HARDWARE				HW_V15_MASTER
//#define SG_HARDWARE				HW_V10_MASTER
#define SG_HARDWARE				HW_V16_MASTER
//#define SG_HARDWARE				HW_V16_REMOTE

//
// This section defined macro-level HW config for different versions.
// Please note that part of the HW config (e.g. specific pin assignments etc) is defined in HardwiredConfig.h file.

#if SG_HARDWARE == HW_V16_MASTER

#define HW_ENABLE_ETHERNET		1
#define HW_ENABLE_SD			1
#define SG_STATION_MASTER		1
#define HW_ENABLE_MOTEINORF		1


#define W5500_USE_CUSOM_SS		1	// special keyword for W5500 ethernet library, to force use of custom SS
#define W5500_SS				1	// define W5500 SS as D1

#define SD_USE_CUSOM_SS			1	// special keyword for SD card library, to force use of custom SS
#define SD_SS					3	// define SD card SS as D3

#define DEFAULT_STATION_ID		0	// for Master default station ID is 0

#endif //HW_V16_MASTER

#if SG_HARDWARE == HW_V15_MASTER

#define HW_ENABLE_ETHERNET		1
#define HW_ENABLE_SD			1
#define SG_STATION_MASTER		1
#define HW_ENABLE_XBEE			1

#define W5500_USE_CUSOM_SS		1	// special keyword for W5500 ethernet library, to force use of custom SS
#define W5500_SS				1	// define W5500 SS as D1

#define SD_USE_CUSOM_SS			1	// special keyword for SD card library, to force use of custom SS
#define SD_SS					3	// define SD card SS as D3

#define DEFAULT_STATION_ID		0	// for Master default station ID is 0

#endif //HW_V15_MASTER

#if SG_HARDWARE == HW_V15_REMOTE

#define HW_ENABLE_XBEE			1

#define USE_I2C_LCD				1	// Use I2C LCD (instead of the parallel-connected LCD)
#define SG_RF_TIME_CLIENT		1	// accept time broadcast messages on RF network

#define DEFAULT_STATION_ID		2	// 

#endif //HW_V15_REMOTE

#if SG_HARDWARE == HW_V16_REMOTE

#define HW_ENABLE_MOTEINORF		1

#define USE_I2C_LCD				1	// Use I2C LCD (instead of the parallel-connected LCD)
#define SG_RF_TIME_CLIENT		1	// accept time broadcast messages on RF network

#define DEFAULT_STATION_ID		2	// 

#endif //HW_V16_REMOTE

// Some common definitions

#define SG_STATION_SLAVE		1	// allow acting as a slave (allow remote access via RF network)
#define DEFAULT_MAX_DURATION	99	// default maximum runtime - 99 minutes

// Weather Underground stuff

// Weather Underground data validity time, mins
#define	WU_VALID_TIME			480


#define MAX_SCHEDULES	4
#define MAX_STATIONS	16
#define MAX_ZONES		64
#define MAX_SENSORS		16

// remote stations may have numbers from 1 to 9
#define MAX_REMOTE_STATIONS  9

// My (master) station ID for network communication
#define MY_STATION_ID		0

#define MAX_STATTION_NAME_LENGTH	20
#define MAX_SENSOR_NAME_LENGTH		20

#define EEPROM_SHEADER "SG16"
#define SG_FIRMWARE_VERSION	27

#define EEPROM_INI_FILE	"/device.ini"

#define DEFAULT_MANUAL_RUNTIME	5

#define DEFAULT_MAX_OFFLINE_TDELTA  1500000UL		// time (in milliseconds) since last time sync before indicator starts to show that the station is offline
													// 1,500,000ms is 25minutes.

// Locally connected channels

#define LOCAL_NUM_CHANNELS			48		// total maximum number of local IO channels

// LCD
#define SHOW_MEMORY 0		// if set to 1 - LCD will show free memory 

// LCD size definitions

#ifdef USE_I2C_LCD
// I2C LCD has 20x2 characters 
#define LOCAL_UI_LCD_X          20
#define LOCAL_UI_LCD_Y          2
#else

#define LOCAL_UI_LCD_X          16
#define LOCAL_UI_LCD_Y          2
#endif

#define KEY_DEBOUNCE_DELAY   50
#define KEY_HOLD_DELAY       1200
#define KEY_REPEAT_INTERVAL  200

// SD Card logging
#define MAX_LOG_RECORD_SIZE    80

// Sensors
// Default sensors logging interval, minutes
#if (SG_HARDWARE == HW_V15_MASTER) || (SG_HARDWARE == HW_V16_MASTER)
#define SENSORS_POLL_DEFAULT_REPEAT  60		// on Master station polling interval is 60minutes - quite high, but Master will poll all stations
#else 
#define SENSORS_POLL_DEFAULT_REPEAT  5		// on Remote station polling interval is 5minutes, to ensure local LCD display updates relatively quickly, and Remote station is not polling anybody else
#endif

// XBee RF network
#define NETWORK_ADDRESS_BROADCAST	0x0FFFF

#define STATIONID_BROADCAST			255		// reserved stationID for broadcasts


//
// Sensor types
//
#define SENSOR_TYPE_NONE				0
#define SENSOR_TYPE_TEMPERATURE			1
#define SENSOR_TYPE_PRESSURE			2
#define SENSOR_TYPE_HUMIDITY			3
#define SENSOR_TYPE_WATERFLOW			4
#define SENSOR_TYPE_VOLTAGE				5

#define SENSOR_DEFAULT_LCD_TEMPERATURE	0		// if defined, this will be the sensor channel shown as Temperature reading on the local LCD
#define SENSOR_DEFAULT_LCD_HUMIDITY		1		// if defined, this will be the sensor channel shown as Humidity reading on the local LCD

// Watchdog timer config
#define SG_WDT_ENABLED			1	// enable WDT 

// Max number of watchdog timer ticks before reset
#define SG_WDT_MAX_TICK_ALLOWED 8	// 8 ticks
// Watchdog timer tick
#define SG_WDT_TICK	WDT_8S	// 8 seconds per tick

// HardwiredConfig.h specifies compile-time defaults for hardware config.
// It is used when device.ini file is unavailable (e.g. no SD card), also few hardware definitions are always compile-time and 
//  cannot be specified in device.ini.
//
#include "HardwiredConfig.h"

#endif // _DEFINES_H