

#include "port.h"
#include <stdio.h>

static FILE serial;
static Stream *_trace_serial;
static bool bSerialSetup = false;


#ifdef ENABLE_TRACE

static int serial_putchar(char c, FILE *stream)
{
        return _trace_serial->write(c);
}


void trace_setup(Stream &tser, unsigned long speed)
{
        Serial.begin(speed); 
		_trace_serial = &tser;
        fdev_setup_stream(&serial, serial_putchar, NULL, _FDEV_SETUP_WRITE);
		bSerialSetup = true;
}

void trace(const char * fmt, ...)
{
        if( !bSerialSetup )
           return;

        va_list parms;
        va_start(parms, fmt);
        vfprintf(&serial, fmt, parms);
        va_end(parms);
}

void trace(const __FlashStringHelper * fmt, ...)
{
        if( !bSerialSetup )
           return;

        va_list parms;
        va_start(parms, fmt);
        vfprintf_P(&serial, reinterpret_cast<const char *>(fmt), parms);
        va_end(parms);
}

#endif // ENABLE_TRACE

extern int __bss_end;
extern int *__brkval;

int GetFreeMemory(void)
{
    int free_memory;
    if(__brkval == 0)
        free_memory = ((int)&free_memory) - ((int)&__bss_end);
    else
        free_memory = ((int)&free_memory) - ((int)__brkval);

	return free_memory;
}

void freeMemory()
{
	int freeMem = GetFreeMemory();
	if( freeMem < TRACE_FREERAM_LIMIT )
	{
		TRACE_CRIT(F("Free Memory %d\n"), freeMem);
	}
}

void sysreset()
{
        asm volatile ("  jmp 0");
}
