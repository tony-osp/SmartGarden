// XBeeRF.h
/*
        XBee RF support for SmartGarden

  This module is intended to be used with my SmartGarden multi-station monitoring and sprinkler control system.


This module handles XBee RF communication in SmartGarden system.



Copyright 2014 tony-osp (http://tony-osp.dreamwidth.org/)

Licensed under the Apache License, Version 2.0 (the "License");
you may not use this file except in compliance with the License.
You may obtain a copy of the License at

    http://www.apache.org/licenses/LICENSE-2.0

Unless required by applicable law or agreed to in writing, software
distributed under the License is distributed on an "AS IS" BASIS,
WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
See the License for the specific language governing permissions and
limitations under the License.

*/

#ifndef _XBEERF_h
#define _XBEERF_h

#if defined(ARDUINO) && ARDUINO >= 100
	#include "Arduino.h"
#else
	#include "WProgram.h"
#endif

#include <XBee.h>

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


private:

	bool	sendAtCommand(const char *cmd_pstr);
	bool	sendAtCommandParam(const char *cmd_pstr, uint8_t *param, uint8_t param_len);
	bool	sendAtCommandParam(const char *cmd_pstr, uint8_t param8);
	bool	sendAtCommandParam(const char *cmd_pstr, uint16_t param16);
	bool	sendAtCommandParam(const char *cmd_pstr, uint32_t param32);

};

extern XBeeRFClass XBeeRF;

#endif

