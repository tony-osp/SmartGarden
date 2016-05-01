/*
Hardwired config definition for the SmartGarden system.

These settings are used when no device.ini is found, or if SD card support is disabled.
Primary case for it is Remote station that typically does not have an SD card, also some of the hardware
configuration (e.g. input buttons) are defined here, at compile time as opposed to device.ini.

Creative Commons Attribution-ShareAlike 3.0 license
Copyright 2014-2015 tony-osp (http://tony-osp.dreamwidth.org/)
*/

#ifndef _HARDWIREDCONFIG_H
#define	_HARDWIREDCONFIG_H

//  Local parallel channels (for Remote stations)
//  This config will be used when device.ini config is not available

//#define LOCAL_NUM_DIRECT_CHANNELS	16		// use this definition for Master station on Mega 2560, with 16 parallel OUT channels
//#define LOCAL_NUM_DIRECT_CHANNELS	4		// use this definition for Master or Remote station with 4 parallel OUT channels

// use this definition for Mega 2560 Master with 16 parallel Output channels
#if SG_HARDWARE == HW_V10_MASTER
#define PARALLEL_PIN_OUT_MAP	{41, 40, 42, 43, 44, 45, 46, 47, 38, 37, 36, 35, 34, 33, 32, 31}
#endif

// use this definition for RBoard-based 4-channel Remote station
#if SG_HARDWARE == HW_V10_REMOTE
#define PARALLEL_PIN_OUT_MAP	{4, 5, 6, 7 }
#endif

// use this definition for Moteino Mega - based Remote station with 8 parallel channels
#if (SG_HARDWARE == HW_V15_REMOTE) || (SG_HARDWARE == HW_V16_REMOTE)
#define LOCAL_NUM_DIRECT_CHANNELS	8		// use this definition for Master or Remote station with 8 parallel OUT channels
#define PARALLEL_PIN_OUT_MAP {12, 13, 14, 18, 19, 20, 21, 22 }
#endif

// use this definition for Moteino Mega - based Remote station with 4 parallel channels and 4 sensors
#if SG_HARDWARE == HW_V17_REMOTE
#define LOCAL_NUM_DIRECT_CHANNELS	4		// use this definition for Remote station with 4 parallel OUT channels
#define PARALLEL_PIN_OUT_MAP {12, 13, 14, 18 }
#endif

// LCD

//Large-screen LCD on MEGA
#if SG_HARDWARE == HW_V10_MASTER
#define PIN_LCD_D4         25    // LCD d4 pin
#define PIN_LCD_D5         24    // LCD d5 pin
#define PIN_LCD_D6         23    // LCD d6 pin
#define PIN_LCD_D7         22    // LCD d7 pin
#define PIN_LCD_EN         26    // LCD enable pin
#define PIN_LCD_RS         27    // LCD rs pin
#endif //HW_V10_MASTER

//Large-screen LCD on Master V1.5
#if (SG_HARDWARE == HW_V15_MASTER) || (SG_HARDWARE == HW_V16_MASTER)
#define PIN_LCD_D4         18    // LCD d4 pin
#define PIN_LCD_D5         14    // LCD d5 pin
#define PIN_LCD_D6         13    // LCD d6 pin
#define PIN_LCD_D7         12    // LCD d7 pin
#define PIN_LCD_EN         19    // LCD enable pin
#define PIN_LCD_RS         20    // LCD rs pin
#endif //HW_V15_MASTER

// Input buttons
//
// switch which input method to use - 1==Analog, undefined - Digital buttons
//#define ANALOG_KEY_INPUT  1
//#define KEY_ANALOG_CHANNEL 1

// Digital input buttons
#if SG_HARDWARE == HW_V10_MASTER
 #define PIN_BUTTON_1      A8    // button Next
 #define PIN_BUTTON_2      A10   // button Up
 #define PIN_BUTTON_3      A9    // button Down
 #define PIN_BUTTON_4      A11   // button Select

#elif (SG_HARDWARE == HW_V15_MASTER) || (SG_HARDWARE == HW_V16_MASTER)
 #define PIN_BUTTON_1      A3    // button 1
 #define PIN_BUTTON_2      A1    // button 2
 #define PIN_BUTTON_3      A2    // button 3
 #define PIN_BUTTON_4      A0    // button 4

// Input buttons may be direct or inverted
#define PIN_INVERTED_BUTTON1  1
#define PIN_INVERTED_BUTTON2  1
#define PIN_INVERTED_BUTTON3  1
//#define PIN_INVERTED_BUTTON4  1

#elif SG_HARDWARE == HW_V17_REMOTE

#define PIN_BUTTON_1      19    // button 1
#define PIN_BUTTON_2      21    // button 2
#define PIN_BUTTON_3      20    // button 3
#define PIN_BUTTON_4      22    // button 4

#else	// default (e.g. HW_V15_REMOTE)
 #define PIN_BUTTON_1      A0    // button 1
 #define PIN_BUTTON_2      A2    // button 2
 #define PIN_BUTTON_3      A1    // button 3
 #define PIN_BUTTON_4      A3    // button 4
#endif //Hardware V10/V15


// Sensors
//
// locally connected DHT (temp & humidity) sensor

#if SG_HARDWARE == HW_V16_REMOTE

#define SENSOR_ENABLE_DHT				1
#define DHTPIN							A4
#define DHTTYPE							DHT21   // DHT 21 (AM2301)
#define SENSOR_CHANNEL_DHT_TEMPERATURE	0		// logical channel this sensor is mapped to
#define SENSOR_CHANNEL_DHT_HUMIDITY		1		// logical channel this sensor is mapped to

#define SENSOR_DEFAULT_LCD_TEMPERATURE	0		// if defined, this will be the sensor channel shown as Temperature reading on the local LCD
#define SENSOR_DEFAULT_LCD_HUMIDITY		1		// if defined, this will be the sensor channel shown as Humidity reading on the local LCD

#endif //SG_HARDWARE == HW_V16_REMOTE

// when DHT sensor is connected, this defines the data pin
#if SG_HARDWARE == HW_V10_MASTER
#define SENSOR_ENABLE_DHT				1
#define DHTPIN							A15
// DHT sensor sub-type
#define DHTTYPE							DHT21   // DHT 21 (AM2301)
#endif


// locally connected BMP180 (air pressure & temp) sensor
//#define SENSOR_ENABLE_BMP180	1
//#define	SENSOR_CHANNEL_BMP180_TEMPERATURE	3
//#define	SENSOR_CHANNEL_BMP180_PRESSURE		4

// locally connected Humidity sensor via Analog port

#if SG_HARDWARE == HW_V17_REMOTE	// V17_REMOTE hardware has 4 channels

#define SENSOR_ENABLE_ANALOG				1	// enable Analog sensor port
#define SENSOR_ANALOG_CHANNELS				2	// 2 Analog channels

#define SENSOR_CHANNEL_ANALOG_1_PIN			A4	// moisture sensor 1 - 1st external port
#define SENSOR_CHANNEL_ANALOG_1_TYPE		SENSOR_TYPE_HUMIDITY	// humidity sensor
#define SENSOR_CHANNEL_ANALOG_1_CHANNEL		0	// logical channel this sensor is mapped to

#define SENSOR_CHANNEL_ANALOG_1_MINV		0	// minimum analog input value for this port
#define SENSOR_CHANNEL_ANALOG_1_MAXV		970	// maximum analog input value for this port
#define SENSOR_CHANNEL_ANALOG_1_MINVAL		0	// minimum sensor output value for this port
#define SENSOR_CHANNEL_ANALOG_1_MAXVAL		100	// maximum sensor output value for this port

#define SENSOR_CHANNEL_ANALOG_2_PIN			A5	// moisture sensor 2 - 2nd external port
#define SENSOR_CHANNEL_ANALOG_2_TYPE		SENSOR_TYPE_HUMIDITY	// humidity sensor
#define SENSOR_CHANNEL_ANALOG_2_CHANNEL		1	// logical channel this sensor is mapped to

#define SENSOR_CHANNEL_ANALOG_2_MINV		0	// minimum analog input value for this port
#define SENSOR_CHANNEL_ANALOG_2_MAXV		970	// maximum analog input value for this port
#define SENSOR_CHANNEL_ANALOG_2_MINVAL		0	// minimum sensor output value for this port
#define SENSOR_CHANNEL_ANALOG_2_MAXVAL		100	// maximum sensor output value for this port

// locally connected Temperature-Thermistor sensor via Analog port
#define SENSOR_ENABLE_THERMISTOR				1	// enable Thermistor sensor port

#define SENSOR_CHANNEL_THERMISTOR_1_PIN			A6	// Thermistor sensor connected to A6
#define SENSOR_CHANNEL_THERMISTOR_1_TYPE		SENSOR_TYPE_TEMPERATURE	// standard sensor type - Temperature
#define SENSOR_CHANNEL_THERMISTOR_1_CHANNEL		2	// logical channel this sensor is mapped to

#define SENSOR_DEFAULT_LCD_HUMIDITY			0	// show Sensor#0 (humidity 1) on LCD

#endif //HW_V17_REMOTE

// locally connected "counter" - type sensor, typically this would be Waterflow meter
//#define SENSOR_ENABLE_COUNTERMETER			1	// enable Counter/Meter sensor port
#define SENSOR_COUNTERMETER_CHANNELS		1	// one Counter/Meter channel
#define SENSOR_COUNTERMETER_RING_DELAY		3	// 3 cycles (i.e. 30ms) delay before counter-meter change is registered

#define SENSOR_CHANNEL_COUNTERMETER_1_PIN		A5	// Arduino pin for this sensor
#define SENSOR_CHANNEL_COUNTERMETER_1_ACTIVE	LOW	// active value for this CounterMeter is LOW or HIGH
#define SENSOR_CHANNEL_COUNTERMETER_1_TYPE		SENSOR_TYPE_WATERFLOW	// waterflow meter/sensor
#define SENSOR_CHANNEL_COUNTERMETER_1_CHANNEL	1	// logical channel this sensor is mapped to

// Normalized CounterMeter reading is calculated by multiplying actual tick delta reading by a constant1 (below) and then dividing by another constant, all in 32bit integer. 
// This is equivalent to multiplying by floating point coefficient, but is a lot faster, while providing reasonable precision
#define SENSOR_CHANNEL_COUNTERMETER_1_MULT		1	// multiplier for computing CounterMeter1 normalized reading
#define SENSOR_CHANNEL_COUNTERMETER_1_DIV		1	// divider for computing CounterMeter1 normalized reading





// XBee RF network

// Indicator that this node is using XBee Pro 900 (Pro 900 uses different channel conventions and addressing)
//#define XBEE_TYPE_PRO900	1


#define NETWORK_XBEE_DEFAULT_PANID	5520	
#ifdef XBEE_TYPE_PRO900		// For XBee Pro 900 we are using channel 7
#define NETWORK_XBEE_DEFAULT_CHAN	7
#else // and for regular 2.4 GHz XBee we must use channels in the range 0xB-0x1A
#define NETWORK_XBEE_DEFAULT_CHAN	15
#endif //XBEE_TYPE_PRO900

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

// MoteinoRF (RFM69)

#define NETWORK_MOTEINORF_DEFAULT_PANID	55

#endif // _HARDWIREDCONFIG_H