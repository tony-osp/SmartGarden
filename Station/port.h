#if defined(ARDUINO) && ARDUINO >= 100
	#include "Arduino.h"
#else
	#include "WProgram.h"
#endif

#include <SdFat.h>
#include <stdio.h>
#include <EEPROM.h>
#include <Ethernet.h>

#include "Defines.h"

#ifdef ENABLE_TRACE

// TRACE_LEVEL defines the level of tracing. I have separate trace macro for each trace type, and depending on the current TRACE_LEVEL
//  some of these macros (or all of them) will be enabled.
//
//	0 ==	Critical messages only (TRACE_CRIT)
//  1 ==	Critical messages and errors (TRACE_ERROR)
//  2 ==	Critical messages, errors and warnings (TRACE_WARNING)
//  3 ==	Critical messages, errors, warnings and informational messages (TRACE_INFO)
//  4 ==	Critical messages, errors, warnings, infrmational messages and verbose tracing

#if TRACE_LEVEL == 0
#define TRACE_CRIT(...)	trace(__VA_ARGS__);
#define TRACE_ERROR(...) { ; }
#define TRACE_WATNING(...) { ; }
#define TRACE_INFO(...) { ; }
#define TRACE_VERBOSE(...) { ; }
#elif TRACE_LEVEL == 1
#define TRACE_CRIT(...)	trace(__VA_ARGS__);
#define TRACE_ERROR(...) trace(__VA_ARGS__);
#define TRACE_WARNING(...) { ; }
#define TRACE_INFO(...) { ; }
#define TRACE_VERBOSE(...) { ; }
#elif TRACE_LEVEL == 2
#define TRACE_CRIT(...)	trace(__VA_ARGS__);
#define TRACE_ERROR(...) trace(__VA_ARGS__);
#define TRACE_WARNING(...) trace(__VA_ARGS__);
#define TRACE_INFO(...) { ; }
#define TRACE_VERBOSE(...) { ; }
#elif TRACE_LEVEL == 3
#define TRACE_CRIT(...)	trace(__VA_ARGS__);
#define TRACE_ERROR(...) trace(__VA_ARGS__);
#define TRACE_WARNING(...) trace(__VA_ARGS__);
#define TRACE_INFO(...) trace(__VA_ARGS__);
#define TRACE_VERBOSE(...) { ; }
#else
#define TRACE_CRIT(...)	trace(__VA_ARGS__);
#define TRACE_ERROR(...) trace(__VA_ARGS__);
#define TRACE_WARNING(...) trace(__VA_ARGS__);
#define TRACE_INFO(...) trace(__VA_ARGS__);
#define TRACE_VERBOSE(...) trace(__VA_ARGS__);
#endif

void trace_setup(Stream &tser, unsigned long speed);
void trace(const char * fmt, ...);
void trace(const __FlashStringHelper * fmt, ...);

#else	//ENABLE_TRACE

#define trace(param, ...)
#define trace_setup(param, ...)

#endif	//ENABLE_TRACE

void freeMemory();
int GetFreeMemory(void);
void sysreset();

#define EXIT_FAILURE 1
