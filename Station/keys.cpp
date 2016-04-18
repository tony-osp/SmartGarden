/*
  Input handling for SmartGarden local UI.

This module handles waitless input for 4 buttons connected as digital or analog input. Digital input is basically buttons directly
connected to Arduino input pins (specific pins assignment is defined in Defines.h), analog input is Freetronics-style (multiple buttons
are connected to the same analog input pin and produce certain voltages). Input selection is defined by ANALOG_KEY_INPUT compile-time define.




Creative Commons Attribution-ShareAlike 3.0 license
Copyright 2014 tony-osp (http://tony-osp.dreamwidth.org/)

*/


#include "Defines.h"
//#define TRACE_LEVEL			6		// all info
#include "port.h"
#include "localUI.h"

// local forward declarations


#ifdef ANALOG_KEY_INPUT

// Analogue buttons version

byte get_keys_now(void)
{
  int val = analogRead(KEY_ANALOG_CHANNEL);

  if( val < 0 )   return BUTTON_NONE; // strange input value, treat it as NONE

  if( val < 30 )  return BUTTON_CONFIRM;  // this is Right Key
//  if( val < 150 ) return BUTTON_UP;
  if( val < 360 ) return BUTTON_DOWN;
  if( val < 535 ) return BUTTON_UP;       // Left key is used as Up because the native Up key is broken
  if( val < 760 ) return BUTTON_MODE;

  return BUTTON_NONE; // strange input value, treat it as NONE
}
#else //ANALOG_KEY_INPUT


byte get_keys_now(void)
{
   byte  new_buttons = 0;
   
#ifdef PIN_INVERTED_BUTTON1
   if( digitalRead(PIN_BUTTON_1) != 0 ) new_buttons |= BUTTON_1;
#else //PIN_INVERTED_BUTTON1
   if( digitalRead(PIN_BUTTON_1) == 0 ) new_buttons |= BUTTON_1;
#endif //PIN_INVERTED_BUTTON1

#ifdef PIN_INVERTED_BUTTON2
   if( digitalRead(PIN_BUTTON_2) != 0 ) new_buttons |= BUTTON_2;
#else //PIN_INVERTED_BUTTON2
   if( digitalRead(PIN_BUTTON_2) == 0 ) new_buttons |= BUTTON_2;
#endif //PIN_INVERTED_BUTTON2

#ifdef PIN_INVERTED_BUTTON3
   if( digitalRead(PIN_BUTTON_3) != 0 ) new_buttons |= BUTTON_3;
#else //PIN_INVERTED_BUTTON3
   if( digitalRead(PIN_BUTTON_3) == 0 ) new_buttons |= BUTTON_3;
#endif //PIN_INVERTED_BUTTON3

#ifdef PIN_INVERTED_BUTTON4
   if( digitalRead(PIN_BUTTON_4) != 0 ) new_buttons |= BUTTON_4;
#else //PIN_INVERTED_BUTTON4
   if( digitalRead(PIN_BUTTON_4) == 0 ) new_buttons |= BUTTON_4;
#endif //PIN_INVERTED_BUTTON4

   //TRACE_INFO(F("Input buttons: %x\n"), new_buttons);

   return new_buttons;
}

#endif //ANALOG_KEY_INPUT

/*
   Main key input function. It is used to poll and read input buttons, behavior depends on mode:

   0    -       Basic input mode. In this mode function returns only key_down events, no hold or auto-repeat
   1    -       In this mode function returns key_down events and handles auto-repeat.


*/

byte get_button_async(byte mode)
{
  byte          key;
  static byte   oldkey=BUTTON_NONE;
  static byte   autorepeat_sent = 0;   // internal flag to track auto-repeat

  static byte   state = 0;             // internal state
                                       // 0 - initial default, nothing pressed
                                       //
                                       // 1 - key pressed, waiting for debounce timeout.
                                       //     old_millis will hold timestamp, oldkey will hold the old key value
                                       //
                                       // 2 - key pressed, debounce check OK, return key value, oldkey will hold the old key value and
                                       //     old_millis will have the timestamp (used for HOLD detection)

   static unsigned long old_millis = 0;
   static unsigned long autorepeat_millis = 0;


  if( state == 0 )              // initial state, nothing was pressed before
  {
    key = get_keys_now();       // read key and convert it into standardized key values. Note: get_keys() returns immediate key state

    if( key == BUTTON_NONE ) return key;        // nothing
// new key detected
        state = 1;
        oldkey = key;
        old_millis = millis();

        return BUTTON_NONE;             // no keypress for now, but internal state changed
  }
  else if( state == 1 )         // we already had prior key press and are waiting for debounce
  {
    if( (millis() - old_millis) < KEY_DEBOUNCE_DELAY ) return BUTTON_NONE;    // we are waiting for KEY_DEBOUNCE_DELAY (which is normally 50ms)
// delay expired, check new value

    key = get_keys_now(); // read key and convert it into standardized key values. Note: get_keys() returns immediate key state
    if( key != oldkey ){  // different key value, clear up things

          state = 0;    // reset state
          oldkey = BUTTON_NONE;
          return BUTTON_NONE;           // debounce check failed, return NONE
        }
// key == oldkey, so we can accept it

        state = 2;  // key accepted, but now we will be watching for HOLD time
        return key;
  }
  else if( state == 2 )         // key was previously accepted and we are watching for HOLD
  {
    key = get_keys_now(); // read key and convert it into standardized key values. Note: get_keys() returns immediate key state

    if( key != oldkey )   // something changed, release old key. NOTE: any changes in keys will be treated as key release
    {
          state = 0;
          oldkey = BUTTON_NONE;

          return BUTTON_NONE;
    }
// key == oldkey, so we continue to hold

    if( mode == 0 ){    // basic mode, no auto-repeat. Simply return NONE

        return BUTTON_NONE;
    }
    else if( mode == 1 ){    // autorepeat mode

       if( (millis() - old_millis) < KEY_HOLD_DELAY ) return BUTTON_NONE;     // no auto-repeat until HOLD timeout expires

// Auto-repeat
       if( autorepeat_sent == 0 ){      // auto-repeat not sent yet, setup things


          autorepeat_sent = 1;
          autorepeat_millis = millis();

          return key;
       }
       else {   // auto-repeat in progress

          if( (millis() - autorepeat_millis) > KEY_REPEAT_INTERVAL ) autorepeat_sent = 0;  // trigger new autorepeat key on the next poll

          return BUTTON_NONE;
       }

    }


    return BUTTON_NONE;  // wrong mode, treat it as if it was basic mode
  }
// we should not really get here - it could happen only if state is invalid. Reset the state

  state = 0; oldkey = BUTTON_NONE;

  return BUTTON_NONE;
}




