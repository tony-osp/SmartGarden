/*

Defaults definition for the SmartGarden system.


Creative Commons Attribution-ShareAlike 3.0 license
Copyright 2014 tony-osp (http://tony-osp.dreamwidth.org/)

*/

#define VERBOSE_TRACE 1


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

#define EEPROM_INI_FILE	"/device.ini"

#define DEFAULT_MANUAL_RUNTIME	5

//  Number of locally connected channels
//
//	Note:	For OpenSprinkler-type IO uses common serial hookup (the same IO pins), just need to make sure that LOCAL_NUM_CHANNELS is sufficiently
//			large to cover connected number of OpenSprinkler-compatible boards.
//
//			!!Important!! LOCAL_NUM_CHANNELS should be a multiply of 8!!
//
//			Also LOCAL_NUM_CHANNELS must be equal or greater than LOCAL_NUM_DIRECT_CHANNELS
//
#define LOCAL_NUM_DIRECT_CHANNELS	16		// number of directly connected (parallel connection) IO channels - positive or negative
#define LOCAL_NUM_CHANNELS			48		// total maximum number of local IO channels

#define NETWORK_ADDRESS_BROADCAST	0x0FFFF

// LCD

#define SHOW_MEMORY 0		// if set to 1 - LCD will show free memory 

//This is my large-screen LCD on MEGA

#define PIN_LCD_D4         25    // LCD d4 pin
#define PIN_LCD_D5         24    // LCD d5 pin
#define PIN_LCD_D6         23    // LCD d6 pin
#define PIN_LCD_D7         22    // LCD d7 pin
#define PIN_LCD_RS         27    // LCD rs pin
#define PIN_LCD_EN         26    // LCD enable pin

// LCD size definitions

#define LOCAL_UI_LCD_X          16
#define LOCAL_UI_LCD_Y          2


//Buttons
// switch which input method to use - 1==Analog, undefined - Digital buttons
//#define ANALOG_KEY_INPUT  1

#define KEY_ANALOG_CHANNEL 1


 #define PIN_BUTTON_1      A8    // button Next
 #define PIN_BUTTON_2      A10   // button Up
 #define PIN_BUTTON_3      A9    // button Down
 #define PIN_BUTTON_4      A11   // button Select

#define KEY_DEBOUNCE_DELAY   50
#define KEY_HOLD_DELAY       1200
#define KEY_REPEAT_INTERVAL  200

// Input buttons may be direct or inverted
#define PIN_INVERTED_BUTTON1  1
#define PIN_INVERTED_BUTTON2  1
#define PIN_INVERTED_BUTTON3  1
//#define PIN_INVERTED_BUTTON4  1

// SD Card logging
#define MAX_LOG_RECORD_SIZE    80

// Sensors
// Default sensors logging interval, minutes
#define SENSORS_POLL_DEFAULT_REPEAT  60
//#define SENSORS_POLL_DEFAULT_REPEAT  5

// locally connected BMP180 (air pressure & temp) sensor
#define SENSOR_ENABLE_BMP180	1
// locally connected DHT (temp & humidity) sensor
#define SENSOR_ENABLE_DHT		1

// when DHT sensor is connected, this defines the data pin
#define DHTPIN   A15
// DHT sensor sub-type
#define DHTTYPE DHT21   // DHT 21 (AM2301)


// XBee

// Indicator that this node is using XBee Pro 900 (Pro 900 uses different channel conventions and addressing)
#define XBEE_TYPE_PRO900	1