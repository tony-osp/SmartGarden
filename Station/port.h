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
//	2 ==	Critical messages only (TRACE_CRIT)
//  3 ==	Critical messages and errors (TRACE_ERROR)
//  4 ==	Critical messages, errors and warnings (TRACE_WARNING)
//  5 ==	Critical messages, errors, warnings, informational messages and notices (TRACE_NOTICE)
//  6 ==	Critical messages, errors, warnings and informational messages (TRACE_INFO)
//  7 ==	Critical messages, errors, warnings, infrmational messages and verbose tracing (TRACE_VERBOSE)

#if TRACE_LEVEL <= 2
#define TRACE_CRIT(...)		trace(__VA_ARGS__);
#define TRACE_ERROR(...)	{ ; }
#define TRACE_WARNING(...)	{ ; }
#define TRACE_NOTICE(...)	{ ; }
#define TRACE_INFO(...)		{ ; }
#define TRACE_VERBOSE(...)	{ ; }
#elif TRACE_LEVEL == 3
#define TRACE_CRIT(...)		trace(__VA_ARGS__);
#define TRACE_ERROR(...)	trace(__VA_ARGS__);
#define TRACE_WARNING(...)	{ ; }
#define TRACE_NOTICE(...)	{ ; }
#define TRACE_INFO(...)		{ ; }
#define TRACE_VERBOSE(...)	{ ; }
#elif TRACE_LEVEL == 4
#define TRACE_CRIT(...)		trace(__VA_ARGS__);
#define SYSEVT_ERROR(...)	trace(__VA_ARGS__);
#define TRACE_WARNING(...)	trace(__VA_ARGS__);
#define TRACE_NOTICE(...)	{ ; }
#define TRACE_INFO(...)		{ ; }
#define TRACE_VERBOSE(...)	{ ; }
#elif TRACE_LEVEL == 5
#define TRACE_CRIT(...)		trace(__VA_ARGS__);
#define TRACE_ERROR(...)	trace(__VA_ARGS__);
#define TRACE_WARNING(...)	trace(__VA_ARGS__);
#define TRACE_NOTICE(...)	trace(__VA_ARGS__);
#define TRACE_INFO(...)		{ ; }
#define TRACE_VERBOSE(...)	{ ; }
#elif TRACE_LEVEL == 6
#define TRACE_CRIT(...)		trace(__VA_ARGS__);
#define TRACE_ERROR(...)	trace(__VA_ARGS__);
#define TRACE_WARNING(...)	trace(__VA_ARGS__);
#define TRACE_NOTICE(...)	trace(__VA_ARGS__);
#define TRACE_INFO(...)		trace(__VA_ARGS__);
#define TRACE_VERBOSE(...)	{ ; }
#else
#define TRACE_CRIT(...)		trace(__VA_ARGS__);
#define TRACE_ERROR(...)	trace(__VA_ARGS__);
#define TRACE_WARNING(...)	trace(__VA_ARGS__);
#define TRACE_NOTICE(...)	trace(__VA_ARGS__);
#define TRACE_INFO(...)		trace(__VA_ARGS__);
#define TRACE_VERBOSE(...)	trace(__VA_ARGS__);
#endif

void trace_setup(Stream &tser, unsigned long speed);
void trace(const char * fmt, ...);
void trace(const __FlashStringHelper * fmt, ...);
void trace_char(char c);

#else	//ENABLE_TRACE

#define TRACE_CRIT(...)	{ ; }
#define TRACE_ERROR(...) { ; }
#define TRACE_WATNING(...) { ; }
#define TRACE_INFO(...) { ; }
#define TRACE_VERBOSE(...) { ; }

#define trace_setup(...) { ; }
void trace_char(...)			{ ; }

#endif	//ENABLE_TRACE

void freeMemory();
int GetFreeMemory(void);
void sysreset();

#define EXIT_FAILURE 1
