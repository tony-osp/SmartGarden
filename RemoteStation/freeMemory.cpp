

#include "freeMemory.h"
#include "port.h"

#if defined(ARDUINO) && ARDUINO >= 100
#include "Arduino.h"
#else
#include "WProgram.h"
#endif


extern int __bss_end;
extern int __bss_start;
extern int __data_end;
extern int __data_start;
extern int __heap_start;
extern int *__brkval;

void freeMemory() 
{
    int free_memory;
    if(__brkval == 0)
        free_memory = ((int)&free_memory) - ((int)&__bss_end);
    else
        free_memory = ((int)&free_memory) - ((int)__brkval);

	trace(F("Free Memory: %d\n"), free_memory);
}

