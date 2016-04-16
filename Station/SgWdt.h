/*

 Watchdog timer support, handles automatic reset in case the system gets stuck.
 Uses Watchdog interrupt on AVR to allow watchdog delay longer than hardware-supported 8 seconds.
 This module is a part of the SmartGarden system.
 

Creative Commons Attribution-ShareAlike 3.0 license
Copyright 2014 tony-osp (http://tony-osp.dreamwidth.org/)
*/
#ifndef _SGWDT_h
#define _SGWDT_h

extern volatile uint8_t _wdtCounter;

inline void SgWdtReset(void)
{
	_wdtCounter = 0;
}

void SgWdtBegin(void);

#endif //_SGWDT_h