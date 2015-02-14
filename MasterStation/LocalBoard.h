/*

 Local Board handler, handles On/Off operations on the channels physically connected to this station.
 
 Local Board handler supports direct Positive or Negative IO, as well as OpenSprinkler-compatible (SPI-style) IO.
 Local Board handler uses hardware config information defined in EEPROM.


Creative Commons Attribution-ShareAlike 3.0 license
Copyright 2014 tony-osp (http://tony-osp.dreamwidth.org/)
*/

#ifndef LOCAL_BOARD_H_
#define LOCAL_BOARD_H_

#include <Arduino.h>
#include "port.h"
#include "settings.h"



class LocalBoardParallel
{
public:
        LocalBoardParallel();
        bool begin(void);
		bool ChannelOn( uint8_t chan );
		bool ChannelOff( uint8_t chan );
		bool AllChannelsOff(void);

private:
        bool	lBoard_ready;

		void io_setup();
};

class LocalBoardSerial
{
public:
        LocalBoardSerial();
        bool begin(void);
		bool ChannelOn( uint8_t chan );
		bool ChannelOff( uint8_t chan );
		bool AllChannelsOff(void);

private:
        bool	lBoard_ready;
		uint8_t	outState[LOCAL_NUM_CHANNELS/8];

		void io_setup();
};

#endif // LOCAL_BOARD_H_