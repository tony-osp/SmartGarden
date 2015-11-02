/*

Thermistor - based temp sensor support routine for SmartGarden system.

Temp conversion function is taken from: https://github.com/EasternStarGeek/Fun-with-Thermistors

Note:	the LUT and original code logic produces temp readings in C, final conversion to F is done at the last step, 
		by multiplying C value by 1.8 (directly in the map() call), and then adding 32.

*/

#include "Arduino.h"

int convertTempInt (int intputVal);
