/*

 Watchdog timer support, handles automatic reset in case the system gets stuck.
 Uses Watchdog interrupt on AVR to allow watchdog delay longer than hardware-supported 8 seconds.
 This module is a part of the SmartGarden system.
 

Creative Commons Attribution-ShareAlike 3.0 license
Copyright 2014 tony-osp (http://tony-osp.dreamwidth.org/)
*/
#include <Arduino.h>
#include <avr/wdt.h>
#include "Defines.h"
#include "port.h"

volatile uint8_t _wdtCounter = 0;	// WDT tick counter 
#include "SgWdt.h"

void SgWdtBegin(void)
{
	wdt_disable();	// wdt should be already disabled, but just in case

	cli();
	WDTCSR = (1<<WDCE | 1<<WDE);   				// Enable the WD Change Bit - configure mode
	WDTCSR = (1<<WDIE)|(0<<WDE)|(1<<WDP3)|(0<<WDP2)|(0<<WDP1)|(1<<WDP0); // Enable WDT interrupt and set WDT to 8 seconds
	sei();			
}


ISR(WDT_vect)
{
	_wdtCounter++;
	if (_wdtCounter < SG_WDT_MAX_TICK_ALLOWED) { // predefined limit
		// start timer again (we are still in interrupt-only mode)
		wdt_reset();
	} else {
		// go for immediate reset

		wdt_disable();
		//sysreset();
        asm volatile ("  jmp 0");
	}
}
