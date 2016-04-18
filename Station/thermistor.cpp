/*

Thermistor - based temp sensor support routine for SmartGarden system.

Temp conversion function is taken from: https://github.com/EasternStarGeek/Fun-with-Thermistors

Note:	the LUT and original code logic produces temp readings in Celsius, final conversion to Fahrenheit is done at the last step, 
		by multiplying C value by 1.8 (directly in the map() call), and then adding 32.

*/

#include "thermistor.h"

const uint16_t LUT_Therm[90] PROGMEM = {
    105, 110, 116, 121, 127, 133, 139, 145, 152, 159,   // -20C to -11C 
    165, 173, 180, 187, 195, 203, 211, 219, 227, 236,   // -10C to -1C 
    245, 254, 264, 273, 283, 293, 303, 313, 324, 334,   //  0C  to +9C 
    345, 355, 366, 377, 388, 399, 410, 422, 433, 444,   //  10C to 19C 
    455, 467, 478, 489, 500, 512, 523, 534, 545, 555,   //  20C to 29C 
    566, 577, 588, 598, 608, 619, 629, 639, 648, 658,   //  30C to 39C 
    667, 677, 686, 695, 704, 712, 721, 729, 737, 745,   //  40C to 49C 
    753, 760, 768, 775, 782, 789, 795, 802, 808, 814,   //  50C to 59C 
    820, 826, 832, 837, 843, 848, 853, 858, 863, 867    //  60C to 69C 
  };

int convertTempInt (int intputVal)  {

  /*
  This function converts a Thermistor reading into a corresponding temperature in degrees C.
  
   The Thermistor is incorporated into a Voltage Divider Circuit with Ra=Thermistor and Rb=10K,
   ASCII circuit diagram, below:
   
   +Vref---[Thermistor]---+--[10K]---GND
                          |
                         ADC @ thermPin
   
   ADC Values were externally calculated from the Thermistor Resistance Table
   using the formula:   ADC = 1023 * 10000/(Rtherm+10000)
   
   (ADC: Analog to Digital Converter)
   
   The lookup table, below, is an array of integer constants containing 
   the predicted ADC values for all temperatures between -20 deg C to + 69 deg C.
   The array index starts at zero, which corresponds to a temperature of -20 deg C.
   
   The resolution of the LUT itself is 1 degree C, but because there is some difference 
   between adjacent ADC values, in some cases more than 10 counts, a linear interpolation 
   between the two closest entries is performed to give a finer output resolution.
   */
   
  int _tempF;  // Intermediate results and final Temperature return value
  uint16_t ADC_Lo;   // The Lower ADC matching value
  uint16_t ADC_Hi;   // The Higher ADC matching value
  int Temp_Lo;  // The Lower whole-number matching temperature
  int Temp_Hi;  // The Higher whole-number matching temperature

  // raw ADC value from Thermistor voltage divider circuit
  uint16_t thermValue = intputVal;

  // Return dummy value if the sensor reading falls outside of the LUT
  if (thermValue < pgm_read_word_near(&LUT_Therm[0])) 
    _tempF = -999;  // Under-range dummy value
  else if (thermValue > pgm_read_word_near(&LUT_Therm[89]) )
    _tempF = 999;  // Over-range dummy value
  else {

    // if Sensor Value is within range...
    for (int i=0; i <= 89; i++){    // Step through the lookup table and look for a match
      if (pgm_read_word_near(&LUT_Therm[i]) > thermValue) {  // Find the closest Higher ADC value
        ADC_Hi = pgm_read_word_near(&LUT_Therm[i]);
        Temp_Hi = i - 20;      // Record the closest Higher whole-number temperature

        // Get the closest Lower whole-number temperature, taking the lower table boundary into account
        if (i != 0)  {
          ADC_Lo = pgm_read_word_near(&LUT_Therm[i-1]);
          Temp_Lo = i - 21;
        }
        else  {
          ADC_Lo = pgm_read_word_near(&LUT_Therm[i]);
          Temp_Lo = i - 20;
        }

        // Interpolate the temperature value for greater precision
        // Note: the Map function does not use floating-point math, so the integer values
        // of temperature are multiplied by 100, and the result is subsequently divided by 100

		// Tony-osp: changed formula to produce F readings instead of C, by changing multiplier to 180 (1.8*) and offset by 32.

         _tempF = int(map(thermValue, ADC_Lo, ADC_Hi, Temp_Lo*180, Temp_Hi*180)/100l)+32;
         break;  // exit for-next loop after the match is detected
      }
    }
  }
  return (_tempF);
}
