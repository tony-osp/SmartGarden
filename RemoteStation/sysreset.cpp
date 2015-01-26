#include "sysreset.h"

void sysreset()
{
        asm volatile ("  jmp 0");
}

