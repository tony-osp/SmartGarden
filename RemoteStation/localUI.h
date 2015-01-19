/*
        Local UI (for local LCD and buttons) on SmartGarden

  This module is intended to be used with my multi-station environment monitoring and sprinklers control system (SmartGarden),
  as well as modified version of the OpenSprinkler code and with modified sprinkler control program sprinklers_pi.


This module handles local UI (LCD and four input buttons connected to the microcontroller). Local UI shows status and allows limited
control of the controller and individual valves.

The code is waitless (does not use delay()), but should be called from loop() sufficiently frequently to handle input and update the UI.



Copyright 2014 tony-osp (http://tony-osp.dreamwidth.org/)

Licensed under the Apache License, Version 2.0 (the "License");
you may not use this file except in compliance with the License.
You may obtain a copy of the License at

    http://www.apache.org/licenses/LICENSE-2.0

Unless required by applicable law or agreed to in writing, software
distributed under the License is distributed on an "AS IS" BASIS,
WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
See the License for the specific language governing permissions and
limitations under the License.

*/


#ifndef _OSLocalUI_h
#define _OSLocalUI_h

#if defined(ARDUINO) && ARDUINO >= 100
#include "Arduino.h"
#else
#include "WProgram.h"
#endif

#include <Time.h>

#define USE_I2C_LCD 1


#ifdef USE_I2C_LCD
#include "LCD_C0220BiZ.h"
#else
#include <LiquidCrystal.h>
#endif

#ifdef USE_I2C_LCD
// I2C LCD library uses reverse parameters order for setcursor
#define LCD_SETCURSOR(lcd, x, y)	lcd.setCursor(y,x)

#else
// Standard LCD library setcursor conventions
#define LCD_SETCURSOR(lcd, x, y)	lcd.setCursor(x,y)

#endif

// global states definitions

// Modes
// Modes numbering starts from zero, and we could have up to 255 Modes, with the mode value of 255 reserved as "undefined"
#define OSUI_MODE_UNDEFINED             255             // undefined
#define OSUI_MODE_HOME                  0               // Home screen
#define OSUI_MODE_MANUAL                1               // Manual mode
#define OSUI_MODE_VIEWCONF              2               // View Config mode
#define OSUI_MODE_SETUP                 3               // Setup mode

// Pages
// Pages numbering starts from zero, and each Mode has its own set of pages
#define OSUI_PAGE_UNDEFINED             255


// Overall UI state
#define OSUI_STATE_UNDEFINED    255
#define OSUI_STATE_SUSPENDED    0
#define OSUI_STATE_ENABLED              1

// LCD

#define SHOW_MEMORY 0

//This is my large-screen LCD on MEGA

#define PIN_LCD_D4         24    // LCD d4 pin - default = 2
#define PIN_LCD_D5         25    // LCD d5 pin - default = 21
#define PIN_LCD_D6         26    // LCD d6 pin - default = 22
#define PIN_LCD_D7         27    // LCD d7 pin - default = 23
#define PIN_LCD_RS         22    // LCD rs pin - default = 19
#define PIN_LCD_EN         23    // LCD enable pin - default = 18

// LCD size definitions

#ifdef USE_I2C_LCD

// I2C LCD has 20x2 characters 
#define LOCAL_UI_LCD_X          20
#define LOCAL_UI_LCD_Y          2
#else

#define LOCAL_UI_LCD_X          16
#define LOCAL_UI_LCD_Y          2
#endif

//Buttons
// switch which input method to use - 1==Analog, undefined - Digital buttons
//#define ANALOG_KEY_INPUT  1

#ifdef ANALOG_KEY_INPUT
#define KEY_ANALOG_CHANNEL 1
#endif

 #define PIN_BUTTON_1      A0    // button 1
 #define PIN_BUTTON_2      A2    // button 2
 #define PIN_BUTTON_3      A1    // button 3
 #define PIN_BUTTON_4      A3    // button 4

#define KEY_DEBOUNCE_DELAY   50
#define KEY_HOLD_DELAY       1200
#define KEY_REPEAT_INTERVAL  200

//#define PIN_INVERTED_BUTTONS  1

// ====== Button Defines ======
#define BUTTON_1            0x01
#define BUTTON_2            0x02
#define BUTTON_3            0x04
#define BUTTON_4            0x08

// button status values
#define BUTTON_NONE         0x00  // no button pressed
#define BUTTON_MASK         0x0F  // button status mask
#define BUTTON_FLAG_HOLD    0x80  // long hold flag
#define BUTTON_FLAG_DOWN    0x40  // down flag
#define BUTTON_FLAG_UP      0x20  // up flag



// Button meaning

// nothing pressed
#define BUTTON_NONE   0

#define BUTTON_MODE             BUTTON_1
#define BUTTON_UP                       BUTTON_2
// note: button 3 is the optional button
#define BUTTON_DOWN             BUTTON_3
#define BUTTON_CONFIRM          BUTTON_4

class OSLocalUI {
public:

  // ====== Member Functions ======
  // -- Setup --
  static byte begin(void);                              // initialization. Intended to be called from setup()

    // -- Operation --
  static byte loop(void);                               // Main loop. Intended to be called regularly and frequently to handle input and UI. Normally this will be called from Arduino loop()
  static byte refresh(void);                            // Force UI refresh.
  static byte suspend(void);                            // Stop UI updates and input handling (useful when taking over the screen for custom output)
  static byte resume(void);                                             // Resume UI operation

  static byte set_mode(char mode);              // Switch UI to desired Mode
  static void lcd_print_pgm(PGM_P PROGMEM str);
  static void lcd_print_line_clear_pgm(PGM_P PROGMEM str, byte line);    // Print a program memory string to a given line with clearing
  static void lcd_print_2digit(int v);

// Data
#ifdef USE_I2C_LCD
  static ST7036 lcd;
#else
  static LiquidCrystal lcd;             // Main LCD object. We have to expose this object to allow custom code access to LCD bypassing UI
#endif
  static byte osUI_State;
  static byte osUI_Mode;
  static byte osUI_Page;

  static void lcd_print_memory(byte line);              // Print free memory

private:

// internal stuff

  static byte display_board;                                                    // currently displayed board

  static void lcd_print_time(byte line);                // Print time to a given line
  static void lcd_print_ip(const byte *ip);             // print ip address and port
  static void lcd_print_station(byte line, char c);
  static void lcd_print_station(byte line, char def_c, byte sel_stn, char sel_c);


// Mode handlers.
// Normally called from loop(), parameter:   0 - regular loop call, no special handling
//                                                                  1 - force UI refresh     (i.e. screen may be corrupted)
//                                                                  2 - initial mode entry, setup things and paint the screen
//
  static byte modeHandler_Home(byte forceRefresh);
  static byte modeHandler_Manual(byte forceRefresh);
  static byte modeHandler_Viewconf(byte forceRefresh);
  static byte modeHandler_Setup(byte forceRefresh);

  static byte callHandler(byte needs_refresh);

};

extern OSLocalUI localUI;

#endif

