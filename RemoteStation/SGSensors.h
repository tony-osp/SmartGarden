/*
  Sensors handling module for the Sprinklers control program.

  This module is intended to be used with my modified version of the OpenSprinkler code, as well as
  with modified Sprinklers control program sprinklers_pi.


This module handles various sensors connected to the microcontroller - Temperature, Pressure, Humidity, Waterflow etc.
Multiple sensor types are supported - BMP180 (pressure), DS18B20 (soil temperature), DHT21 (humidity and temperature) etc. 
Each sensor type is handled by its own piece of code, all sensors are wrapped into the common Sensors class.

Operation is waitless (to a possible extent). The common Sensors class is expected to be called for initialization (from setup()), and from the polling loop.

The Sensors class will handle sensors configuration and sensors reading at configured frequency. Readings are logged using external Logging class (sd-log) 
using time-series pattern. Errors are reported using common Trace infrastructure.



 Creative Commons Attribution-ShareAlike 3.0 license
 Copyright 2014 tony-osp (http://tony-osp.dreamwidth.org/)


*/

#ifndef _OSSensors_h
#define _OSSensors_h

#if defined(ARDUINO) && ARDUINO >= 100
#include "Arduino.h"
#else
#include "WProgram.h"
#endif

#include <Time.h>
#include <wire.h>
#include "sdlog.h"

// Temporary I put sensors enable/disable flags here. (note: later on these flags will move to a config file)
//
//
//#define SENSOR_ENABLE_BMP180  1

#define SENSOR_ENABLE_DHT	1

#ifdef SENSOR_ENABLE_BMP180
#include <SFE_BMP180.h>
#include <Wire.h>
#endif

#ifdef SENSOR_ENABLE_DHT
#include <dht.h>
#endif

// when DHT sensor is connected, this defines the data pin
#define DHTPIN   3

#define DHTTYPE DHT21   // DHT 21 (AM2301)

// default repeat intervals, in minutes
#define SENSORS_PRESSURE_DEFAULT_REPEAT 60

//#define SENSORS_TEMPERATURE_DEFAULT_REPEAT 60
#define SENSORS_TEMPERATURE_DEFAULT_REPEAT 2

class Sensors {
public:

  // ====== Member Functions ======

  
  // -- Setup --
  static byte begin(void);                              // initialization. Intended to be called from setup()

    // -- Operation --
  static byte loop(void);                               // Main loop. Intended to be called regularly and frequently to handle sensors reading and logging. Usually  this will be called from Arduino loop()

// Data

  static	int	Temperature;		// latest known readings
  static	int	Humidity;

private:

// internal stuff

    
//  static byte display_board;                                                    // currently displayed board

//  static void lcd_print_station(byte line, char def_c, byte sel_stn, char sel_c);


};


#endif // _OSSensors_h
