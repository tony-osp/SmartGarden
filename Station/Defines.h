/*

Defaults definition for the SmartGarden system.


Creative Commons Attribution-ShareAlike 3.0 license
Copyright 2014 tony-osp (http://tony-osp.dreamwidth.org/)

*/

#define HW_V15_MASTER			1	// compile for Master station, hardware version 1.5 (Moteino Mega-based)
//#define HW_V15_REMOTE			1	// compile for Remote station, hardware version 1.5 (Moteino Mega-based)
//#define HW_V10_MASTER			1	// compile for Master station, hardware version 1.0 (Mega 2560-based)

#ifdef HW_V15_MASTER

#define HW_ENABLE_ETHERNET		1
#define HW_ENABLE_SD			1
//#define SG_STATION_MASTER		1

#define W5500_USE_CUSOM_SS		1	// special keyword for W5500 ethernet library, to force use of custom SS
#define W5500_SS				1	// define W5500 SS as D1

#define SD_USE_CUSOM_SS			1	// special keyword for SD card library, to force use of custom SS
#define SD_SS					3	// define SD card SS as D3

#endif //HW_V15_MASTER

#ifdef HW_V15_REMOTE
#define USE_I2C_LCD				1
#endif //HW_V15_REMOTE

#define HW_ENABLE_XBEE			1

#define SG_STATION_SLAVE		1

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

#define EEPROM_SHEADER "T2.8"

#define EEPROM_INI_FILE	"/device.ini"

#define DEFAULT_MANUAL_RUNTIME	5

// Locally connected channels

#define LOCAL_NUM_CHANNELS			48		// total maximum number of local IO channels

//  Local parallel channels (for Remote stations)
//  This config will be used when device.ini config is not available

//#define LOCAL_NUM_DIRECT_CHANNELS	16		// use this definition for Master station on Mega 2560, with 16 parallel OUT channels
#define LOCAL_NUM_DIRECT_CHANNELS	8		// use this definition for Master or Remote station with 8 parallel OUT channels
//#define LOCAL_NUM_DIRECT_CHANNELS	4		// use this definition for Master or Remote station with 4 parallel OUT channels

// use this definition for Mega 2560 Master with 16 parallel Output channels
//#define PARALLEL_PIN_OUT_MAP	{41, 40, 42, 43, 44, 45, 46, 47, 38, 37, 36, 35, 34, 33, 32, 31}

// use this definition for RBoard-based 4-channel Remote station
//#define PARALLEL_PIN_OUT_MAP	{4, 5, 6, 7 }

// use this definition for Moteino Mega - based Remote station with 8 parallel channels
#define PARALLEL_PIN_OUT_MAP {12, 13, 14, 18, 19, 20, 21, 22 }



#define NETWORK_ADDRESS_BROADCAST	0x0FFFF

// LCD
#define SHOW_MEMORY 0		// if set to 1 - LCD will show free memory 

//Large-screen LCD on MEGA
#ifdef HW_V10_MASTER
#define PIN_LCD_D4         25    // LCD d4 pin
#define PIN_LCD_D5         24    // LCD d5 pin
#define PIN_LCD_D6         23    // LCD d6 pin
#define PIN_LCD_D7         22    // LCD d7 pin
#define PIN_LCD_EN         26    // LCD enable pin
#define PIN_LCD_RS         27    // LCD rs pin
#endif //HW_V10_MASTER

//Large-screen LCD on Master V1.5
#ifdef HW_V15_MASTER
#define PIN_LCD_D4         18    // LCD d4 pin
#define PIN_LCD_D5         14    // LCD d5 pin
#define PIN_LCD_D6         13    // LCD d6 pin
#define PIN_LCD_D7         12    // LCD d7 pin
#define PIN_LCD_EN         19    // LCD enable pin
#define PIN_LCD_RS         20    // LCD rs pin
#endif //HW_V15_MASTER

// LCD size definitions

#ifdef USE_I2C_LCD
// I2C LCD has 20x2 characters 
#define LOCAL_UI_LCD_X          20
#define LOCAL_UI_LCD_Y          2
#else

#define LOCAL_UI_LCD_X          16
#define LOCAL_UI_LCD_Y          2
#endif


//Buttons
// switch which input method to use - 1==Analog, undefined - Digital buttons
//#define ANALOG_KEY_INPUT  1

#define KEY_ANALOG_CHANNEL 1


#if defined(__AVR_ATmega2560__)

 #define PIN_BUTTON_1      A8    // button Next
 #define PIN_BUTTON_2      A10   // button Up
 #define PIN_BUTTON_3      A9    // button Down
 #define PIN_BUTTON_4      A11   // button Select
#else

 #define PIN_BUTTON_1      A0    // button 1
 #define PIN_BUTTON_2      A2    // button 2
 #define PIN_BUTTON_3      A1    // button 3
 #define PIN_BUTTON_4      A3    // button 4
#endif //Arduino MEGA

#define KEY_DEBOUNCE_DELAY   50
#define KEY_HOLD_DELAY       1200
#define KEY_REPEAT_INTERVAL  200

// Input buttons may be direct or inverted
//#define PIN_INVERTED_BUTTON1  1
//#define PIN_INVERTED_BUTTON2  1
//#define PIN_INVERTED_BUTTON3  1
//#define PIN_INVERTED_BUTTON4  1

// SD Card logging
#define MAX_LOG_RECORD_SIZE    80

// Sensors
// Default sensors logging interval, minutes
#define SENSORS_POLL_DEFAULT_REPEAT  60
//#define SENSORS_POLL_DEFAULT_REPEAT  5

// locally connected BMP180 (air pressure & temp) sensor
//#define SENSOR_ENABLE_BMP180	1
// locally connected DHT (temp & humidity) sensor
#define SENSOR_ENABLE_DHT		1

// when DHT sensor is connected, this defines the data pin
#if defined(__AVR_ATmega2560__)
#define DHTPIN   A15
#else  // Moteino Mega
#define DHTPIN   A4
#endif //Arduino MEGA
// DHT sensor sub-type
#define DHTTYPE DHT21   // DHT 21 (AM2301)


// XBee RF network

// Indicator that this node is using XBee Pro 900 (Pro 900 uses different channel conventions and addressing)
#define XBEE_TYPE_PRO900	1

#define NETWORK_XBEE_DEFAULT_PANID	5520	
#define NETWORK_XBEE_DEFAULT_CHAN	7

#if defined(__AVR_ATmega2560__)
#define NETWORK_XBEE_DEFAULT_PORT	3
#else //__AVR_ATmega2560__
#if defined(__AVR_ATmega1284P__) || defined(__AVR_ATmega1284p__)
#define NETWORK_XBEE_DEFAULT_PORT	1
#else //uno or equivalent
#define NETWORK_XBEE_DEFAULT_PORT	0
#endif
#endif //__AVR_ATmega2560__

#define NETWORK_XBEE_DEFAULT_SPEED	57600

