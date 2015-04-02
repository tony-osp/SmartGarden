// XBeeRF.h
/*
        XBee RF support for SmartGarden

  This module is intended to be used with my SmartGarden multi-station monitoring and sprinkler control system.


This module handles XBee RF communication in SmartGarden system.



 Creative Commons Attribution-ShareAlike 3.0 license
 Copyright 2014 tony-osp (http://tony-osp.dreamwidth.org/)


*/

#ifndef _XBEERF_h
#define _XBEERF_h

#if defined(ARDUINO) && ARDUINO >= 100
	#include "Arduino.h"
#else
	#include "WProgram.h"
#endif

#include <XBee.h>
#include "settings.h"

struct LongXBeeAddress {    // note: we are storing long 64bit addresses in XBee format (big endian)
	uint32_t	MSB;
	uint32_t	LSB;
};

class XBeeRFClass
{
 public:
	XBeeRFClass();
	void begin(void);
	void loop(void);

// Remote stations commands

	XBee		xbee;				// Core XBee object
	bool		fXBeeReady;			// Flag indicating that XBee is initialized and ready
	uint8_t		frameIDCounter;		// Rolling counter used to generate FrameID

	LongXBeeAddress	arpTable[NETWORK_MAX_STATIONS];

private:

	bool	sendAtCommand(const char *cmd_pstr);
	bool	sendAtCommandParam(const char *cmd_pstr, uint8_t *param, uint8_t param_len);
	bool	sendAtCommandParam(const char *cmd_pstr, uint8_t param8);
	bool	sendAtCommandParam(const char *cmd_pstr, uint16_t param16);
	bool	sendAtCommandParam(const char *cmd_pstr, uint32_t param32);

};

extern XBeeRFClass XBeeRF;

#endif

