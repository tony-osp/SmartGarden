/*
        EEPROM Map for the Master Station of the SmartGarden system



Creative Commons Attribution-ShareAlike 3.0 license
Copyright 2014-2015 tony-osp (http://tony-osp.dreamwidth.org/)

*/

#ifndef _EEPROM_MAP_h
#define _EEPROM_MAP_h

#define ADDR_SCHEDULE_COUNT						4
#define ADDR_OP1                                5
#define ADDR_NUM_ZONES                          7
#define ADDR_PUMP_STATION                       8
#define ADDR_PUMP_CHANNEL                       9

#define END_OF_ZONE_BLOCK						4096
#define END_OF_SCHEDULE_BLOCK					2048
#define ADDR_NTP_IP                             20
#define ADDR_NTP_OFFSET							24
#define ADDR_HOST                               25 // NOT USED
#define MAX_HOST_LEN							20  // NOT USED
#define ADDR_IP                                 46
#define ADDR_NETMASK							50
#define ADDR_GATEWAY							54
#define ADDR_DHCP                               58 // NOT USED
#define ADDR_WUIP                               62
#define ADDR_ZIP                                66
#define ADDR_APIKEY                             70
#define ADDR_OTYPE                              78
#define ADDR_WEB                                79
#define ADDR_SADJ                               81
#define ADDR_PWS                                82	// 11 bytes long field
#define ADDR_MY_STATION_ID						95
#define ADDR_EVTMASTER_FLAGS					96	// 2 bytes
#define ADDR_EVTMASTER_STATIONID				98

#define ADDR_NUM_OT_DIRECT_IO					100
#define	ADDR_OT_DIRECT_IO						101		// Local direct IO map (positive or negative), maximum of 16 GPIO pins
#define ADDR_NUM_OT_OPEN_SPRINKLER				117
#define ADDR_OT_OPEN_SPRINKLER					118		// OpenSprinkler-style serial IO hookup

// Networks config


#define	ADDR_NETWORK_XBEE_FLAGS		128		// XBee network status flags (enabled/on/etc)
#define ADDR_NETWORK_XBEE_PORT		129		// Serial port to use
#define ADDR_NETWORK_XBEE_SPEED		130		// Serial port speed to use, two bytes

#define ADDR_NETWORK_XBEE_PANID		132		// XBee PAN ID, two bytes
#define ADDR_NETWORK_XBEE_ADDR16	134		// XBee address, two bytes (we are using two-byte mode)
#define ADDR_NETWORK_XBEE_CHAN		136		// XBee channel, one byte

#define SCHEDULE_OFFSET 1536
#define SCHEDULE_INDEX 128
#define ZONE_OFFSET 2048
#define ZONE_INDEX 32

#define STATION_INDEX			48
#define STATION_OFFSET			768
#define END_OF_STATION_BLOCK	1536

#define ADDR_NUM_SENSORS		256

#define SENSOR_INDEX			24
#define SENSOR_OFFSET			257
#define END_OF_SENSORS_BLOCK	641

#define ADDR_WWCOUNTERS			642			// last 7 days water counters (16bit, one per day of the week)
#define	ADDR_TOTAL_WCOUNTER		657			// lifetime water counter, 32bit, updated daily
#define ADDR_D_WWCOUNTERS		661			// date stamps of the WWCOUNTERS updates, 32bit per date stamp, 7 date stamps

#if ZONE_OFFSET + (ZONE_INDEX * MAX_ZONES) > END_OF_ZONE_BLOCK
#error Number of Zones is too large
#endif

#if SCHEDULE_OFFSET + (SCHEDULE_INDEX * MAX_SCHEDULES) > END_OF_SCHEDULE_BLOCK
#error Number of Schedules is too large
#endif

#if STATION_OFFSET + (STATION_INDEX * MAX_STATIONS) > END_OF_STATION_BLOCK
#error Number of Stations is too large
#endif

#if SENSORS_OFFSET + (SENSORS_INDEX * MAX_SENSORS) > END_OF_SENSORS_BLOCK
#error Number of Sensors is too large
#endif


#endif //_EEPROM_MAP_h