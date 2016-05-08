/*

 Local Board handler, handles On/Off operations on the channels physically connected to this station.
 This module is a part of the SmartGarden system.
 
 Local Board handler supports direct Positive or Negative IO, as well as OpenSprinkler-compatible (SPI-style) IO.
 Local Board handler uses hardware config information defined in EEPROM.



Creative Commons Attribution-ShareAlike 3.0 license
Copyright 2014 tony-osp (http://tony-osp.dreamwidth.org/)
*/

#include "LocalBoard.h"

// Local forward declarations

// Parallel channels support class

LocalBoardParallel::LocalBoardParallel(void)
{
	lBoard_ready = false;
}

bool LocalBoardParallel::begin(void)
{
	if( GetNumIOChannels() != 0 )
		io_setup();

   return true;
}


//  Turn local board channel On.
//
//	Input: channel - numbered from 0.
//
//  Note: channel should be within configured IO range.
//

bool LocalBoardParallel::ChannelOn( uint8_t chan )
{
		if( lBoard_ready != true )		// to ensure local board is ready
			return false;

		if( chan > GetNumIOChannels() )
		{
			SYSEVT_ERROR(F("LocalBoardParallel:ChannelOn - channel %d outside of range"), (int)chan);
			return false;				// channel number is outside of the configured IO range
		}

//		SYSEVT_ERROR(F("LocalBoardParallel:ChannelOn - channel %d"), (int)chan);

		const EOT eot = GetOT();		// get local IO type
        switch (eot)
        {
        case OT_DIRECT_POS:
        case OT_DIRECT_NEG:

                digitalWrite(GetDirectIOPin(chan), (eot==OT_DIRECT_POS)?1:0);
				return true;

        default:
                break;
        }
		return false;
}

bool LocalBoardParallel::ChannelOff( uint8_t chan )
{
		if( lBoard_ready != true )		// to ensure local board is ready
			return false;

		if( chan > GetNumIOChannels() )
		{
			SYSEVT_ERROR(F("LocalBoardParallel:ChannelOff - channel %d outside of range"), (int)chan);
			return false;				// channel number is outside of the configured IO range
		}

		const EOT eot = GetOT();		// get local IO type
        switch (eot)
        {
        case OT_DIRECT_POS:
        case OT_DIRECT_NEG:

                digitalWrite(GetDirectIOPin(chan), (eot==OT_DIRECT_POS)?0:1);
                return true;

        default:
                break;
        }
		return false;
}

bool LocalBoardParallel::AllChannelsOff(void)
{
		if( lBoard_ready != true )		// to ensure local board is ready
			return false;

		for( uint8_t i=0; i<GetNumIOChannels(); i++ ) 
		{
			ChannelOff(i);
		}

		return true;
}


void LocalBoardParallel::io_setup()
{
        const EOT eot = GetOT();

        if( (eot == OT_DIRECT_POS) || (eot == OT_DIRECT_NEG) )
        {
             for( uint8_t i=0; i<GetNumIOChannels(); i++ )
             {
				pinMode(GetDirectIOPin(i), OUTPUT);
                digitalWrite(GetDirectIOPin(i), (eot==OT_DIRECT_NEG)?1:0);
             }
        }

		lBoard_ready = true;
        AllChannelsOff();
}


// Serial channels support class

LocalBoardSerial::LocalBoardSerial(void)
{
	lBoard_ready = false;
}

bool LocalBoardSerial::begin(void)
{
	if( GetNumOSChannels() != 0 )
		io_setup();

   return true;
}


//  Turn local board channel On.
//
//	Input: channel - numbered from 0.
//
//  Note: channel should be within configured IO range.
//

bool LocalBoardSerial::ChannelOn( uint8_t chan )
{
	//TRACE_CRIT(F("LocalBoardSerial::ChannelOn - entering, channel=%i\n"), int(chan));

		if( lBoard_ready != true )		// to ensure local board is ready
			return false;				

		if( chan >= GetNumOSChannels() )
		{
			SYSEVT_ERROR(F("LocalBoardSerial:ChannelOn - channel %d outside of range of %d"), (int)chan, (int)GetNumOSChannels());
			return false;				// channel number is outside of the configured IO range
		}

		outState[chan/8] |= 0x01 << (chan%8);	// record change in the local state cache

		SrIOMapStruct	SrIoMap; 
		LoadSrIOMap(&SrIoMap);			// load serial (OpenSprinkler-style) IO map from EEPROM

	//TRACE_CRIT(F("LocalBoardSerial::ChannelOn - serial output\n"));

		// turn off the latch pin
        digitalWrite(SrIoMap.SrLatPin, 0);
        digitalWrite(SrIoMap.SrClkPin, 0);

		uint8_t numOSChannels = GetNumOSChannels();

        for( uint8_t iboard = 0; iboard < (numOSChannels/8); iboard++ )
		{
			for( uint8_t i = 0; i < 8; i++ )
			{
	//TRACE_CRIT(F("Output iteration, val=%i\n"), int(outState[iboard]&(0x01<<(7-i))) );

				digitalWrite(SrIoMap.SrClkPin, 0);
				digitalWrite(SrIoMap.SrDatPin, outState[iboard]&(0x01<<(7-i)));
				digitalWrite(SrIoMap.SrClkPin, 1);
			}
        }
	//TRACE_CRIT(F("LocalBoardSerial::ChannelOn - latching state and enabling output\n"));
        // latch the outputs
        digitalWrite(SrIoMap.SrLatPin, 1);

        // Turn off the NOT enable pin (turns on outputs)
        digitalWrite(SrIoMap.SrNoePin, 0);
        return true;
}

bool LocalBoardSerial::ChannelOff( uint8_t chan )
{
		if( lBoard_ready != true )		// to ensure local board is ready
			return false;				

		if( chan >= GetNumOSChannels() )
		{
			SYSEVT_ERROR(F("LocalBoardSerial:ChannelOff - channel %d outside of range"), (int)chan);
			return false;				// channel number is outside of the configured IO range
		}

        outState[chan/8] &= ~(0x01 << (chan%8));	// record change in the local state cache

		SrIOMapStruct	SrIoMap; 
		LoadSrIOMap(&SrIoMap);			// load serial (OpenSprinkler-style) IO map from EEPROM

		// turn off the latch pin
        digitalWrite(SrIoMap.SrLatPin, 0);
        digitalWrite(SrIoMap.SrClkPin, 0);

		uint8_t numOSChannels = GetNumOSChannels();

        for( uint8_t iboard = 0; iboard < (numOSChannels/8); iboard++ )
		{
			for( uint8_t i = 0; i < 8; i++ )
			{
				digitalWrite(SrIoMap.SrClkPin, 0);
				digitalWrite(SrIoMap.SrDatPin, outState[iboard]&(0x01<<(7-i)));
				digitalWrite(SrIoMap.SrClkPin, 1);
			}
        }
        // latch the outputs
        digitalWrite(SrIoMap.SrLatPin, 1);

        // Turn off the NOT enable pin (turns on outputs)
        digitalWrite(SrIoMap.SrNoePin, 0);
        return true;
}

bool LocalBoardSerial::AllChannelsOff(void)
{
		if( lBoard_ready != true )		// to ensure local board is ready
			return false;

		for( uint8_t i=0; i<GetNumOSChannels(); i++ ) 
		{
			ChannelOff(i);
		}

		return true;
}


// refresh channel pins without changing their state (just output current state)
bool LocalBoardSerial::loop(void)
{
		if( lBoard_ready != true )		// to ensure local board is ready
			return false;

		SrIOMapStruct	SrIoMap; 
		LoadSrIOMap(&SrIoMap);			// load serial (OpenSprinkler-style) IO map from EEPROM

		// turn off the latch pin
        digitalWrite(SrIoMap.SrLatPin, 0);
        digitalWrite(SrIoMap.SrClkPin, 0);

		uint8_t numOSChannels = GetNumOSChannels();

        for( uint8_t iboard = 0; iboard < (numOSChannels/8); iboard++ )
		{
			for( uint8_t i = 0; i < 8; i++ )
			{
				digitalWrite(SrIoMap.SrClkPin, 0);
				digitalWrite(SrIoMap.SrDatPin, outState[iboard]&(0x01<<(7-i)));
				digitalWrite(SrIoMap.SrClkPin, 1);
			}
        }
        // latch the outputs
        digitalWrite(SrIoMap.SrLatPin, 1);

        // Turn off the NOT enable pin (turns on outputs)
        digitalWrite(SrIoMap.SrNoePin, 0);

		return true;
}


void LocalBoardSerial::io_setup()
{
		SrIOMapStruct	SrIoMap; 
		LoadSrIOMap(&SrIoMap);			// load serial (OpenSprinkler-style) IO map from EEPROM

        pinMode(SrIoMap.SrClkPin, OUTPUT);
        digitalWrite(SrIoMap.SrClkPin, 0);
        pinMode(SrIoMap.SrNoePin, OUTPUT);
        digitalWrite(SrIoMap.SrNoePin, 0);
        pinMode(SrIoMap.SrDatPin, OUTPUT);
        digitalWrite(SrIoMap.SrDatPin, 0);
        pinMode(SrIoMap.SrLatPin, OUTPUT);
        digitalWrite(SrIoMap.SrLatPin, 0);

		lBoard_ready = true;
        AllChannelsOff();
}

