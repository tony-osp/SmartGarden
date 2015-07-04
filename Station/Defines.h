/*

Defaults definition for the SmartGarden system.


Creative Commons Attribution-ShareAlike 3.0 license
Copyright 2014 tony-osp (http://tony-osp.dreamwidth.org/)

*/

#ifndef _DEFINES_H

#define _DEFINES_H 1

#define ENABLE_TRACE 1
#define VERBOSE_TRACE 1


// Supported hardware version definitions

#define HW_V10_MASTER			1	// compile for Master station, hardware version 1.0 (Mega 2560-based)
#define HW_V15_MASTER			2	// compile for Master station, hardware version 1.5 (Moteino Mega-based)
#define HW_V15_REMOTE			3	// compile for Remote station, hardware version 1.5 (Moteino Mega-based)

// Uncomment the line that corresponds to the actual hardware

#define SG_HARDWARE				HW_V15_REMOTE
//#define SG_HARDWARE				HW_V15_MASTER
//#define SG_HARDWARE				HW_V10_MASTER

#if SG_HARDWARE == HW_V15_MASTER

#define HW_ENABLE_ETHERNET		1
#define HW_ENABLE_SD			1
#define SG_STATION_MASTER		1

#define W5500_USE_CUSOM_SS		1	// special keyword for W5500 ethernet library, to force use of custom SS
#define W5500_SS				1	// define W5500 SS as D1

#define SD_USE_CUSOM_SS			1	// special keyword for SD card library, to force use of custom SS
#define SD_SS					3	// define SD card SS as D3

#define DEFAULT_STATION_ID		0	// for Master default station ID is 0

#endif //HW_V15_MASTER

#if SG_HARDWARE == HW_V15_REMOTE

#define USE_I2C_LCD				1	// Use I2C LCD (instead of the parallel-connected LCD)
#define SG_RF_TIME_CLIENT		1	// accept time broadcast messages on XBee network

#define DEFAULT_STATION_ID		2	// for Remote default station ID is 1

#endif //HW_V15_REMOTE

#define HW_ENABLE_XBEE			1

#define SG_STATION_SLAVE		1	// allow acting as a slave (allow remote access via RF network)
#define DEFAULT_MAX_DURATION	99	// default maximum runtime - 99 minutes


#define MAX_SCHEDULES	4
#define MAX_STATIONS	16
#define MAX_ZONES		64
#define MAX_SENSORS		10

// remote stations may have numbers from 1 to 9
#define MAX_REMOTE_STATIONS  9

// My (master) station ID for network communication
#define MY_STATION_ID		0

#define MAX_STATTION_NAME_LENGTH	20
#define MAX_SENSOR_NAME_LENGTH		20

#define EEPROM_SHEADER "T2.7"
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
#define SENSORS_POLL_DEFAULT_REPEAT  60
//#define SENSORS_POLL_DEFAULT_REPEAT  5

// XBee RF network
#define NETWORK_ADDRESS_BROADCAST	0x0FFFF

// HardwiredConfig.h specifies compile-time defaults for hardware config.
// It is used when device.ini file is unavailable (e.g. no SD card), also few hardware definitions are always compile-time and 
//  cannot be specified in device.ini.
//
#include "HardwiredConfig.h"

#endif // _DEFINES_H