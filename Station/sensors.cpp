/*
  Sensors handling module for the SmartGarden system.


This module handles various sensors connected to the microcontroller - Temperature, Pressure, Humidity, Waterflow etc.
Multiple sensor types are supported - BMP180 (pressure), DS18B20 (soil temperature), DHT21 (humidity and temperature) etc. 
Each sensor type is handled by its own piece of code, all sensors are wrapped into the common Sensors class.

Operation is waitless (to a possible extent). The common Sensors class is expected to be called for initialization (from setup()), and from the polling loop.

The Sensors class will handle sensors configuration and sensors reading at configured frequency. Readings are logged using external Logging class (sd-log) 
using time-series pattern. Errors are reported using common Trace infrastructure.



Creative Commons Attribution-ShareAlike 3.0 license
Copyright 2014 tony-osp (http://tony-osp.dreamwidth.org/)


*/

#include "sensors.h"
#include "port.h"
#include <SFE_BMP180.h>
#include <Wire.h>
#include "XBeeRF.h"
#include "RProtocolMS.h"

#ifdef SENSOR_ENABLE_COUNTERMETER
#include "TimerOne.h"
#endif //SENSOR_ENABLE_COUNTERMETER

// external reference
extern Logging sdlog;

#ifdef SENSOR_ENABLE_DHT
// For DHT (AM2301 or similar) sensor we need to have an object, here called "dht_sensor"
DHT dht_sensor(DHTPIN, DHTTYPE);
#endif

#ifdef SENSOR_ENABLE_BMP180
// For BMP180 sensor we need SFE_BMP180 object, here called "bmp180":
SFE_BMP180 bmp180;
#endif



// maximum ulong value
#define MAX_ULONG       4294967295

// local forward declarations

byte  bmp180_Read(int *pressure, int *temperature);
void  pollSensorIsr(void);

// initialization. Intended to be called from setup()
//
// returns TRUE on success and FALSE otherwise
//
byte Sensors::begin(void)
{

// Setup sensors list

		uint8_t			numS = GetNumSensors();
		
		if( numS != 0 )	
		{
			for( uint8_t i=0; i<numS; i++ )
			{
				LoadShortSensor(i, &SensorsList[i].config);

				if( SensorsList[i].config.sensorType == SENSOR_TYPE_TEMPERATURE )
				{
					fLCDSensors = true;
#ifdef SENSOR_DEFAULT_LCD_TEMPERATURE
					iLCDTempIndex = SENSOR_DEFAULT_LCD_TEMPERATURE;
#else // SENSOR_DEFAULT_LCD_TEMPERATURE
					iLCDTempIndex = i;
#endif // SENSOR_DEFAULT_LCD_TEMPERATURE

				}
				else if( SensorsList[i].config.sensorType == SENSOR_TYPE_HUMIDITY )
				{
					fLCDSensors = true;
#ifdef SENSOR_DEFAULT_LCD_HUMIDITY
					iLCDHumidIndex = SENSOR_DEFAULT_LCD_HUMIDITY;
#else // SENSOR_DEFAULT_LCD_HUMIDITY
					iLCDHumidIndex = i;
#endif // SENSOR_DEFAULT_LCD_HUMIDITY
				}
				SensorsList[i].lastReading = 0;
				SensorsList[i].lastReadingTimestamp = (time_t)(MAX_ULONG/2);
			}
		}

// generate the list of remote stations to poll

		for( uint8_t i=0; i<MAX_STATIONS; i++ )
		{
			bool fFound = false;

			for( uint8_t s=0; s<numS; s++ )
			{
				if( SensorsList[s].config.sensorStationID == i )
					fFound = true;
			}
			if( fFound )
			{
					stationsToPollList[numStationsToPoll] = i;
					numStationsToPoll++;
			}
		}

		pollMinutesCounter = 0;  // trigger initial sensor read on the first minute poll
		nPoll = 0;

  // If we have local sensor, Initialize it (it is important to get calibration values stored on the device).

  // Initialize sensors (it is important to get calibration values stored on the device).

#ifdef SENSOR_ENABLE_BMP180
     if( !bmp180.begin() ){
           TRACE_ERROR(F("BMP180 sensor init failure.\n"));
     }
#endif  //   SENSOR_ENABLE_BMP180

#ifdef SENSOR_ENABLE_DHT
     dht_sensor.begin();
#endif

#ifdef SENSOR_ENABLE_ANALOG

#ifdef SENSOR_CHANNEL_ANALOG_1_PIN
	 pinMode(SENSOR_CHANNEL_ANALOG_1_PIN, INPUT);
#endif //SENSOR_CHANNEL_ANALOG_1_PIN

#ifdef SENSOR_CHANNEL_ANALOG_2_PIN
	 pinMode(SENSOR_CHANNEL_ANALOG_2_PIN, INPUT);
#endif //SENSOR_CHANNEL_ANALOG_2_PIN

#ifdef SENSOR_CHANNEL_ANALOG_3_PIN
	 pinMode(SENSOR_CHANNEL_ANALOG_3_PIN, INPUT);
#endif //SENSOR_CHANNEL_ANALOG_3_PIN

#ifdef SENSOR_CHANNEL_ANALOG_4_PIN
	 pinMode(SENSOR_CHANNEL_ANALOG_4_PIN, INPUT);
#endif //SENSOR_CHANNEL_ANALOG_4_PIN

#endif //SENSOR_ENABLE_ANALOG

#ifdef SENSOR_ENABLE_COUNTERMETER

// Counter/Meter is handled using timer-driven polling. Polling is done using ISR driven by Arduino timer

	 Timer1.initialize(10000);			// timer will fire every 10ms (10,000 microseconds)
	 Timer1.attachInterrupt(pollSensorIsr);	

#endif //SENSOR_ENABLE_COUNTERMETER


     return true;
}

#ifdef SENSOR_ENABLE_COUNTERMETER

// poll sensor ISR
void pollSensorIsr(void)
{

}

#endif //SENSOR_ENABLE_COUNTERMETER


// -- Operation --

static unsigned long  old_millis = millis() - 60000;             // setup initial condition to make it trigger on the first loop
// Main loop. Intended to be called regularly  to handle sensors readings. Usually this will be called from Arduino loop()
byte Sensors::loop(void)
{
       unsigned long  new_millis = millis();    // Note: we are using built-in Arduino millis() function instead of now() or time-zone adjusted LocalNow(), because it is a lot faster
                                                // and for detecting minutes changes it does not make any difference.
       if( (new_millis - old_millis) >= 60000 ){   // one minute detection
//       if( (new_millis - old_millis) >= 10000 ){   // debug - 10 sec instead of 1 minute

             old_millis = new_millis;             
             poll_MinTimer();             
       }
}


// timer worker for sensors polling.
// This function will be called once a minute, allowing sensors code to read data if required.
//
// The reporting sensors pipeline will update the dashboard (in-memory last values), and will log the data at the right frequency
//
void Sensors::poll_MinTimer(void)
{
    pollMinutesCounter--;
	if( pollMinutesCounter <= 0 )  // required repeat interval check
	{
			pollMinutesCounter = SENSORS_POLL_DEFAULT_REPEAT;	// reload counter
			nPoll = 0;
	}

	if( nPoll < numStationsToPoll  )			// we will poll remote sensors next minute after the local sensors. Signal to poll remote sensors will be set by the poll minutes counter logic above.
	{
		if( stationsToPollList[nPoll] == GetMyStationID() )	// poll local sensors
		{
// Local sensors

#ifdef SENSOR_ENABLE_BMP180
			{
				int  pressure, temperature;

				// read BMP180 sensor           
				if( bmp180_Read(&pressure, &temperature) == false ){
            
					TRACE_ERROR(F("Failure reading pressure from BMP180.\n"));
				}
				else
				{
					ReportSensorReading( GetMyStationID(), SENSOR_CHANNEL_BMP180_TEMPERATURE, temperature );	
					ReportSensorReading( GetMyStationID(), SENSOR_CHANNEL_BMP180_PRESSURE, pressure );	
				}
			}
#endif  //   SENSOR_ENABLE_BMP180

#ifdef SENSOR_ENABLE_DHT
			{
				register int  hum, temp;

				// now read DHT sensor
				float h = dht_sensor.readHumidity();
                float t = dht_sensor.readTemperature();

				if( isnan(t) || isnan(h) )
				{
					TRACE_ERROR(F("Failure reading temperature or humidity from DHT.\n"));

//					// debugging - temporary hardcode some value here
					temp = 10; hum = 20;
 					ReportSensorReading( GetMyStationID(), SENSOR_CHANNEL_DHT_TEMPERATURE, temp );	
 					ReportSensorReading( GetMyStationID(), SENSOR_CHANNEL_DHT_HUMIDITY, hum );	
				}
				else
				{
					temp = (int) (dht_sensor.convertCtoF(t) + 0.5);      // convert to int with rounding
					hum = (int) (h + 0.5);

 					ReportSensorReading( GetMyStationID(), SENSOR_CHANNEL_DHT_TEMPERATURE, temp );	
 					ReportSensorReading( GetMyStationID(), SENSOR_CHANNEL_DHT_HUMIDITY, hum );	
				}
			}
#endif  //   SENSOR_ENABLE_DHT

#ifdef SENSOR_ENABLE_ANALOG
			{
				int		val;
#ifdef SENSOR_CHANNEL_ANALOG_1_PIN				
				val = analogRead(SENSOR_CHANNEL_ANALOG_1_PIN);

//				TRACE_ERROR(F("Analog sensor#1 reading: %d\n"), val);

				if( val < SENSOR_CHANNEL_ANALOG_1_MINV ) val = SENSOR_CHANNEL_ANALOG_1_MINV;
				if( val > SENSOR_CHANNEL_ANALOG_1_MAXV ) val = SENSOR_CHANNEL_ANALOG_1_MAXV;

// to increase precision we are up-converting input signal by multiplying it by 16
// and by multiplying scaling factor by 16 as well. 
// It is OK, since input signal is guaranteed to be only 10bit (AtMega ADC).				
				val = (val-SENSOR_CHANNEL_ANALOG_1_MINV)*16;
				val = val/(SENSOR_CHANNEL_ANALOG_1_SCALE*16);
				val = val + SENSOR_CHANNEL_ANALOG_1_MINVAL;

//				TRACE_ERROR(F("Analog sensor#1 converted: %d, scale:%d\n"), val, int(SENSOR_CHANNEL_ANALOG_1_SCALE));
				ReportSensorReading( GetMyStationID(), SENSOR_CHANNEL_ANALOG_1_CHANNEL, val );	
#endif //SENSOR_CHANNEL_ANALOG_1_PIN

#ifdef SENSOR_CHANNEL_ANALOG_2_PIN				
				val = analogRead(SENSOR_CHANNEL_ANALOG_2_PIN);

				if( val < SENSOR_CHANNEL_ANALOG_2_MINV ) val = SENSOR_CHANNEL_ANALOG_2_MINV;
				if( val > SENSOR_CHANNEL_ANALOG_2_MAXV ) val = SENSOR_CHANNEL_ANALOG_2_MAXV;

				val = (val-SENSOR_CHANNEL_ANALOG_2_MINV)*16;
				val = val/(SENSOR_CHANNEL_ANALOG_2_SCALE*16);
				val = val + SENSOR_CHANNEL_ANALOG_2_MINVAL;
				ReportSensorReading( GetMyStationID(), SENSOR_CHANNEL_ANALOG_2_CHANNEL, val );	
#endif //SENSOR_CHANNEL_ANALOG_2_PIN

			}
#endif //SENSOR_ENABLE_ANALOG


		}
		else
		{
			// poll remote stations

			XBeeRF.PollStationSensors(stationsToPollList[nPoll]);
		}

		nPoll++;
	}
}

#ifdef SENSOR_ENABLE_BMP180

// Worker function to read bmp180 pressure sensor.
// Returns true if successful, false if failed.
byte bmp180_Read(int *pressure, int *temperature)
{
           double T,P;

           byte  status = bmp180.startTemperature();
           if (status != 0){

               delay(status);  // Wait for the measurement to complete

               // Retrieve the completed temperature measurement:
               // Note that the measurement is stored in the variable T.
               // Function returns 1 if successful, 0 if failure.

               status = bmp180.getTemperature(T);
               if (status != 0){
    
                    *temperature = (int) ((9.0/5.0)*T+32.5);    // Sensor reads temp in C, but we want to return temperature in F, also converting it to integer (note 0.5 correction for rounding).
      
                    // Start pressure measurement:
                    // The parameter is the oversampling setting, from 0 to 3 (highest res, longest wait).
                    // If request is successful, the number of ms to wait is returned.
                    // If request is unsuccessful, 0 is returned.

                    status = bmp180.startPressure(1);    // accuracy 1 which should be relatively fast.
                    if (status != 0){
     
                         delay(status); // Wait for the measurement to complete

                         // Note that the function requires the previous temperature measurement (T).

                         status = bmp180.getPressure(P,T);      // Function returns 1 if successful, 0 if failure.
                         if (status != 0)   *pressure = (int) (P + 0.5);  // convert it to int. Note 0.5 correction for rounding
                         else               return false;  // error retrieving pressure measurement
                    }
                    else return false;   // error starting pressure measurement
               }
               else return false; // error retrieving temperature measurement
           }
           else return false;  // error starting temperature measurement

           return  true;
}
#endif //SENSOR_ENABLE_BMP180

//
// Sensor readings dispatch and handling
//
//	Input:	stationID 		- the station this sensor is connected to
//			sensorChannel	- channel on that station the sensor is connected to
//			sensorReading	- actual sensor reading
//
void Sensors::ReportSensorReading( uint8_t stationID, uint8_t sensorChannel, int sensorReading )
{
	uint8_t			numS = GetNumSensors();

	for( uint8_t i=0; i<numS; i++ )
	{
		if( SensorsList[i].config.sensorType != SENSOR_TYPE_NONE )	// just in case check, ensuring that the entry is valid
		{
			if( (SensorsList[i].config.sensorStationID == stationID) && (SensorsList[i].config.sensorChannel == sensorChannel) )
			{
				// we found our sensor. Store latest reading and log it.
				
				SensorsList[i].lastReading = sensorReading;
				SensorsList[i].lastReadingTimestamp = millis();

				if( iLCDTempIndex == i )
				{
					Temperature = sensorReading;
				}
				else if( iLCDHumidIndex == i )
				{
					Humidity = sensorReading;
				}
#ifdef notdef	// I don't want to proactively send sensor readings to Master, let's poller pick it up
				if( GetEvtMasterFlags() & EVTMASTER_FLAGS_REPORT_SENSORS )
				{
					rprotocol.SendSensorsReport(0, GetMyStationID(), GetEvtMasterStationID(), i, 1);
				}
#endif //notdef
				sdlog.LogSensorReading( SensorsList[i].config.sensorType, (int)i, sensorReading );
				return;
			}
		}
	}
// scan complete but we have not found the sensor we need.

	TRACE_ERROR(F("ReportSensorReading - cannot find sensor, stationID=%d, channel=%d\n"), (int)stationID, (int)sensorChannel);
}

// Sensors handling
// Emit last reported sensors reading (as JSON)
//

bool Sensors::TableLastSensorsData(FILE* stream_file)
{
		uint8_t			numS = GetNumSensors();
		FullSensor		fullSensor;
		FullStation		fullStation;
		char			tmp_buf[MAX_SENSOR_NAME_LENGTH+1];
		
		if( numS == 0 )	// no sensors to display
		{
			fprintf_P(stream_file, PSTR("\n\t{\n\t}\n"));   // empty list
			return true;
		}
		
		// we have something to output
		
        fprintf_P(stream_file, PSTR("\n\t \"sensors\" : [\n"));    // open the list

		for( uint8_t i=0; i<numS; i++ )
		{
			LoadSensor(i, &fullSensor);
			LoadStation( fullSensor.sensorStationID, &fullStation);

			if( i!= 0 )
				fprintf_P(stream_file, PSTR("\n\t\t },"));	// close previous sensor record if this is not the first one
			
			memcpy( tmp_buf, fullSensor.name, 20 );
            fprintf_P(stream_file, PSTR("\n\t { \n\t\t \"sensorID\": %u,\n\t\t \"sensorName\": \"%s\","), (unsigned int)i, tmp_buf);
			
			if( fullSensor.sensorType == SENSOR_TYPE_TEMPERATURE )	  strcpy_P(tmp_buf, PSTR("Temperature"));
			else if( fullSensor.sensorType == SENSOR_TYPE_HUMIDITY )  strcpy_P(tmp_buf, PSTR("Humidity"));
			else if( fullSensor.sensorType == SENSOR_TYPE_PRESSURE )  strcpy_P(tmp_buf, PSTR("Pressure"));
			else if( fullSensor.sensorType == SENSOR_TYPE_WATERFLOW ) strcpy_P(tmp_buf, PSTR("Waterflow"));
			else if( fullSensor.sensorType == SENSOR_TYPE_VOLTAGE )   strcpy_P(tmp_buf, PSTR("Voltage"));
			else													  strcpy_P(tmp_buf, PSTR("Unknown"));
            fprintf_P(stream_file, PSTR("\n\t\t \"sensorType\": \"%s\","), tmp_buf);

			memcpy( tmp_buf, fullStation.name, 20 );
			fprintf_P(stream_file, PSTR("\n\t\t \"stationID\": %u, \n\t\t \"stationName\": \"%s\","), (unsigned int)(fullSensor.sensorStationID), tmp_buf);
			fprintf_P(stream_file, PSTR("\n\t\t \"sensorChannel\": %u, \n\t\t \"lastReading\": %i,\n\t\t \"readingAge\": %lu"), fullSensor.sensorChannel, SensorsList[i].lastReading, (millis() - SensorsList[i].lastReadingTimestamp)/1000);
		}
        fprintf_P(stream_file, PSTR("\n\t\t }\n\t ]\n"));    // close the last sensor if we emitted and the list

        return true;
}

Sensors sensorsModule;
