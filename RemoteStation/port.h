#if defined(ARDUINO) && ARDUINO >= 100
	#include "Arduino.h"
#else
	#include "WProgram.h"
#endif

#include <stdio.h>
#include <EEPROM.h>

#define ENABLE_TRACE 1


#ifdef ENABLE_TRACE

void trace_setup(Stream &tser);
void trace(const char * fmt, ...);
void trace(const __FlashStringHelper * fmt, ...);

#else	//ENABLE_TRACE

#define trace(param, ...)
#define trace_setup(param)


#endif	//ENABLE_TRACE

void freeMemory();
int GetFreeMemory(void);
void sysreset();

#define EXIT_FAILURE 1
