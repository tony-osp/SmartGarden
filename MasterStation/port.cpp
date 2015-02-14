

#include "port.h"
#include <stdio.h>

static FILE serial;
static Stream *_trace_serial;
static bool bSerialSetup = false;

static int serial_putchar(char c, FILE *stream)
{
        return _trace_serial->write(c);
}


void trace_setup(Stream &tser)
{
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
   trace(F("Free Memory %d\n"), GetFreeMemory());
}

void sysreset()
{
        asm volatile ("  jmp 0");
}
