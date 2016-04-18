/*
       Local UI (for local LCD and buttons) on SmartGarden

  This module is intended to be used with my multi-station environment monitoring and sprinklers control system (SmartGarden),
  as well as modified version of the OpenSprinkler code and with modified sprinkler control program sprinklers_pi.


This module handles local UI (LCD and four input buttons connected to the microcontroller). Local UI shows status and allows limited
control of the controller and individual valves.

The code is waitless (does not use delay()), but should be called from loop() sufficiently frequently to handle input and update the UI.




Creative Commons Attribution-ShareAlike 3.0 license
Copyright 2014 tony-osp (http://tony-osp.dreamwidth.org/)

*/

#include <ethernet.h>
#include "Defines.h"
#include "localUI.h"
#include "nntp.h"
#include "core.h"
#include "settings.h"
#include "sensors.h"

// Data members
byte OSLocalUI::osUI_State = OSUI_STATE_UNDEFINED;
byte OSLocalUI::osUI_Mode  = OSUI_MODE_UNDEFINED;
byte OSLocalUI::osUI_Page  = OSUI_PAGE_UNDEFINED;
byte OSLocalUI::display_board = 0;                                                      // currently displayed board


#ifdef USE_I2C_LCD
ST7036 OSLocalUI::lcd( LOCAL_UI_LCD_Y, LOCAL_UI_LCD_X, 0x78 );
#else
// and this is Mega with big LCD version
LiquidCrystal OSLocalUI::lcd(PIN_LCD_RS, PIN_LCD_EN, PIN_LCD_D4, PIN_LCD_D5, PIN_LCD_D6, PIN_LCD_D7);
#endif

extern  uint8_t		LastReceivedStationID;
extern	int16_t		LastReceivedRSSI;


inline uint8_t GetLastReceivedStationID(void)
{
	return LastReceivedStationID;
}

inline uint32_t GetLastReceivedTime(uint8_t stationID)
{
	return 	runState.sLastContactTime[stationID];
}

inline int16_t GetLastReceivedRSSI()
{
	return LastReceivedRSSI;
}

// Local forward declarations

byte GetNextStation(byte curr_station);
byte GetPrevStation(byte curr_station);

byte get_button_async(byte mode);

byte weekday_today(void);
int get_key(unsigned int input);
void buttons_loop(void);
void manual_station_on(byte sid, int ontimer);


// button functions

byte button_read_busy(byte pin_butt, byte waitmode, byte butt, byte is_holding);        // Wait for button
byte button_read(byte waitmode);                                                                         // Read button and returns button value 'OR'ed with flag bits
void ui_set_options(int oid);                                                                            // user interface for setting options during startup



// ====== UI defines ======

static char ui_anim_chars[3] = {'.', 'o', 'O'};
static char ui_anim_chars_1[3] = {2, 3, 4};
static char ui_anim_chars_2[3] = {4, 3, 2};

// Weekday display strings
prog_char str_day0[] PROGMEM = "Mon";
prog_char str_day1[] PROGMEM = "Tue";
prog_char str_day2[] PROGMEM = "Wed";
prog_char str_day3[] PROGMEM = "Thu";
prog_char str_day4[] PROGMEM = "Fri";
prog_char str_day5[] PROGMEM = "Sat";
prog_char str_day6[] PROGMEM = "Sun";

char* days_str[7] = {
  str_day0,
  str_day1,
  str_day2,
  str_day3,
  str_day4,
  str_day5,
  str_day6
};


// initialization. Intended to be called from setup()
//
// returns TRUE on success and FALSE otherwise
//
byte OSLocalUI::begin(void)
{
#ifdef USE_I2C_LCD
     lcd.init();         // I2C library uses different init conventions
	 lcd.clear();
#else
	 lcd.begin(LOCAL_UI_LCD_X, LOCAL_UI_LCD_Y);         // init LCD with desired dimensions
#endif

     osUI_State      = OSUI_STATE_SUSPENDED;
     osUI_Mode       = OSUI_MODE_HOME;                         // home screen by default
     osUI_Page       = 0;                                                           // first page by default (pages start form 0)

// define lcd custom characters
    byte lcd_custom_char[8] = {
      B00000,
      B10100,
      B01000,
      B10101,
      B00001,
      B00101,
      B00101,
      B10101
    };
#ifdef USE_I2C_LCD
	lcd.load_custom_character(1, lcd_custom_char);
#else
	lcd.createChar(1, lcd_custom_char);
#endif
    lcd_custom_char[1] = lcd_custom_char[2] = 0;
    lcd_custom_char[3]=1;
#ifdef USE_I2C_LCD
	lcd.load_custom_character(0, lcd_custom_char);
#else
	lcd.createChar(0, lcd_custom_char);
#endif

// Start/Stop animation chars

	memset(lcd_custom_char, 0, sizeof(lcd_custom_char));
	lcd_custom_char[6] = lcd_custom_char[5] = 0xFF;
#ifdef USE_I2C_LCD
	lcd.load_custom_character(2, lcd_custom_char);
#else
	lcd.createChar(2, lcd_custom_char);
#endif

	memset(lcd_custom_char, 0, sizeof(lcd_custom_char));
	lcd_custom_char[3] = lcd_custom_char[4] = 0xFF;
#ifdef USE_I2C_LCD
	lcd.load_custom_character(3, lcd_custom_char);
#else
	lcd.createChar(3, lcd_custom_char);
#endif

	memset(lcd_custom_char, 0, sizeof(lcd_custom_char));
	lcd_custom_char[1] = lcd_custom_char[2] = 0xFF;
#ifdef USE_I2C_LCD
	lcd.load_custom_character(4, lcd_custom_char);
#else
	lcd.createChar(4, lcd_custom_char);
#endif

// set button PINs mode

   pinMode(PIN_BUTTON_1, INPUT);
   pinMode(PIN_BUTTON_2, INPUT);
   pinMode(PIN_BUTTON_3, INPUT);
   pinMode(PIN_BUTTON_4, INPUT);
   digitalWrite(PIN_BUTTON_1, HIGH);
   digitalWrite(PIN_BUTTON_2, HIGH);
   digitalWrite(PIN_BUTTON_3, HIGH);
   digitalWrite(PIN_BUTTON_4, HIGH);

   return true;    // exit, status - success
}

// -- Operation --

// Main loop. Intended to be called regularly and frequently to handle input and UI. Typically this will be called from Arduino loop()
  byte OSLocalUI::loop(void)
  {
    if( osUI_State != OSUI_STATE_ENABLED ) return true; //UI update disabled, nothing to do - exit

// local UI is enabled, call appropriate handler

     return callHandler(0);
  }

  // Force UI refresh.
byte OSLocalUI::refresh(void)
{
   return callHandler(1);
}

byte OSLocalUI::callHandler(byte needs_refresh)
{
   if( osUI_Mode == OSUI_MODE_HOME )					return modeHandler_Home(needs_refresh);
        else if( osUI_Mode == OSUI_MODE_MANUAL )		return modeHandler_Manual(needs_refresh);
        else if( osUI_Mode == OSUI_MODE_STATUS )		return modeHandler_Status(needs_refresh);
        else if( osUI_Mode == OSUI_MODE_SETUP )			return modeHandler_Setup(needs_refresh);

        return false;   // incorrect UI mode - exit with failure
}


// Stop UI updates and input handling (useful when taking over the screen for custom output)
  byte OSLocalUI::suspend(void)
  {
        osUI_State = OSUI_STATE_SUSPENDED;
        return true;    // exit - success
  }

// Resume UI operation
  byte OSLocalUI::resume(void)
  {
        osUI_State = OSUI_STATE_ENABLED;

        return refresh();
  }

// Switch UI to desired Mode
  byte OSLocalUI::set_mode(char mode)
  {
        osUI_Mode = mode;
        osUI_Page = 0;

       return callHandler(2);    // call MODE handler, indicating that it needs to setup things.
  }


 // Home screen mode handler.
 // It is responsible for updating the UI and handling input keys. All operations are asynchronous - must return right away.
 // Typically it is called from loop(), but could be called from other places as well, usually to force UI refresh.
 // Parameter indicates whether UI refresh is required (e.g. if the screen was previously modified by some other code).
 // TRUE means that refresh is required, FALSE means nobody touched LCD since the last call to this handler and UI updates could be more targeted.
 //
 byte OSLocalUI::modeHandler_Home(byte forceRefresh)
 {
   static unsigned long old_millis = 0;

// assert
   if( osUI_Mode != OSUI_MODE_HOME )  return false;  // Basic protection to ensure current UI mode is actually HOME mode.

   if( forceRefresh == 2 ){     // entering this Mode

     display_board = GetMyStationID();         // set initial view to board 0;
//	 TRACE_INFO(F("GetMyStationID returned %d\n"), int(display_board));
//	 delay(3000);
   }

   char btn = get_button_async(0);

// handle input
   if( btn == BUTTON_MODE ){

       set_mode( OSUI_MODE_MANUAL ); // change mode to "view settings" which is the next mode
       return true;
   }
   else if( btn == BUTTON_CONFIRM ){

      if( GetRunSchedules() )  SetRunSchedules(false);  // schedules are currently enabled, disable it
      else   {

           if( ActiveZoneNum() != -1 ){    // schedules are disabled, but something is currently running (manual mode), turn it off

                runState.TurnOffZones();
           }
           else  {

               SetRunSchedules(true);  // schedules are currently disabled and no manual channels running, enable schedules
           }
      }

// Apply changes to stop/start schedules as required.
		  runState.ProcessScheduledEvents();

          forceRefresh = 1;             // force UI refresh to provide immediate visual feedback
   }
   else if( btn == BUTTON_UP ){

       display_board = GetNextStation(display_board);
	   forceRefresh = 1;
   }
   else if( btn == BUTTON_DOWN ){

       display_board = GetPrevStation(display_board);
       forceRefresh = 1;
   }

 // Show time and station status
// if 1 second has passed
  unsigned long  new_millis = millis();    // Note: we are using built-in Arduino millis() function instead of now() or time-zone adjusted LocalNow(), because it is a lot faster
                                                             // and for detecting second change it does not make any difference.

  if( ((new_millis - old_millis) >= 1000) || (forceRefresh != 0) ){   // update UI once a second, OR if explicit refresh is required

    old_millis = new_millis;
    lcd_print_time(0);       // print time

// Automatically switch to a board that has active zone running

	int zn = ActiveZoneNum();
	if( zn != -1 ){   // active zone will show up as a positive number
			
		ShortZone  zone;
		LoadShortZone(zn-1, &zone);	// note zone number conversion (it starts from 1)

		if( display_board != zone.stationID ){

			display_board = zone.stationID;		// if we are not on the right board switch to it now
		}
	}

    // process LCD display
    if(SHOW_MEMORY)
      lcd_print_memory(1);
    else
      lcd_print_station();
  }

  return true;
}

// Manual mode loop() handler

byte OSLocalUI::modeHandler_Manual(byte forceRefresh)
{
  static byte sel_manual_ch = 0;
  static unsigned long old_millis = 0;
  static byte man_state = 0;                    // this flag indicates the UI state within MANUAL mode. Valid states are:
                                                                        // 0 - initial screen, select the channel
                                                                        // 1 - entering number of minutes to run
  static byte num_min = 0;
  static byte max_ch = 0;

// assert
  if( osUI_Mode != OSUI_MODE_MANUAL )  return false;  // Basic protection to ensure current UI mode is actually HOME mode.

  if( forceRefresh == 2 ){   // entering MANUAL mode, refresh things

          lcd.clear();
		  LCD_SETCURSOR(lcd, 0, 0);
          lcd_print_pgm(PSTR("Manual Start:"));

          man_state = 0;
          sel_manual_ch = 0;

		  ShortStation	sStation;
		  LoadShortStation(display_board, &sStation);	// load station information
		  max_ch = sStation.numZoneChannels;

          forceRefresh = 1;    // need to update the screen
  }
  unsigned long new_millis = millis();

  char btn = get_button_async(1);    // Note: we allow Autorepeat in this mode, to help enter data quickly

  if( man_state == 0){          // select channel screen

         if( btn == BUTTON_UP ){

                if( sel_manual_ch < (max_ch-1) ) sel_manual_ch++;
                else							 sel_manual_ch = 0;
                forceRefresh = 1;
         }
         else if( btn == BUTTON_DOWN ){

            if( sel_manual_ch > 0 ) sel_manual_ch--;
                else                sel_manual_ch = max_ch-1;
                forceRefresh = 1;
         }
         else if( btn == BUTTON_MODE ){

            set_mode( OSUI_MODE_STATUS ); // change mode to "view settings" which is the next mode
            return true;
         }
         else if( btn == BUTTON_CONFIRM ){

            man_state = 1;              // channel selected, need to enter the number of minutes to run
                num_min = 0;            // when entering the "number of minutes" screen, start from 0 minutes

// let's display the number of minutes prompt
                lcd.clear();
				LCD_SETCURSOR(lcd, 0, 0);
                lcd_print_pgm(PSTR("Minutes to run:"));

                return  true;    // exit. Actual minutes display will happen on the next loop();
         }

		 if( ((new_millis - old_millis) >= 333) || (forceRefresh != 0) ){   // update UI three times a second (for blinking), OR if explicit refresh is required

			old_millis = new_millis;
			lcd_print_station('_', sel_manual_ch, ((new_millis/333)%2) ? '#':'_', max_ch);       // this will blink selected station
		 }
  }

  if( man_state == 1){          // enter the number of minutes to run

         if( btn == BUTTON_UP ){

                if( num_min < 99 ) num_min++;
                else               num_min = 0;
                forceRefresh = 1;
         }
         else if( btn == BUTTON_DOWN ){

            if( num_min > 0 ) num_min--;
                else          num_min = 99;
                forceRefresh = 1;
         }
         else if( btn == BUTTON_MODE ){

            set_mode( OSUI_MODE_STATUS ); // exit current mode, changing it to "view settings" which is the next mode
                return true;
         }
         else if( btn == BUTTON_CONFIRM ){
// start manual watering run on selected channel with selected number of minutes to run

                lcd.clear();
				LCD_SETCURSOR(lcd, 0, 0);
                lcd_print_pgm(PSTR("Starting Manual"));

				ShortStation	sStation;
				LoadShortStation(display_board, &sStation);	// load station information
				LCD_SETCURSOR(lcd, 0, 1);
                lcd_print_pgm(PSTR("Ch: "));    lcd_print_2digit(sel_manual_ch+sStation.startZone+1);    //note: actual useful zones are numbered from 1, adjust it for display
                lcd_print_pgm(PSTR(" Min: "));  lcd_print_2digit(num_min);

                delay(2000);

                if( num_min != 0 ){

//					TRACE_ERROR(F("Starting manual. Display_board %d, channel %d, zone %d, number of minutes %d\n"), display_board, sel_manual_ch, sel_manual_ch+sStation.startZone, num_min);

					runState.StartZone(100, display_board, sel_manual_ch, num_min );			// start required station in manual mode for num_min only if required time is != 0
//					manual_station_on((byte)(sel_manual_ch+sStation.startZone), num_min);        // start required station in manual mode for num_min only if required time is != 0
                }
                set_mode( OSUI_MODE_HOME ); // Manual watering started, exit current UI mode changing it to HOME
                return true;
     }
// now screen update and blinking cursor
     if( ((new_millis - old_millis) >= 333) || (forceRefresh != 0) ){   // update UI three times a second (for blinking), OR if explicit refresh is required

           old_millis = new_millis;

		   LCD_SETCURSOR(lcd, 0, 1);
           lcd_print_2digit(num_min);
           if( (new_millis/333)%2 ) lcd_print_pgm(PSTR("   "));
           else						lcd_print_pgm(PSTR("#  "));
     }

  }
  return true;
}

/*
  Local config viewer. Supports multiple pages

*/
byte OSLocalUI::modeHandler_Status(byte forceRefresh)
{
// assert
   if( osUI_Mode != OSUI_MODE_STATUS )  return false;  // Basic protection to ensure current UI mode is correct

// initial setup, start from page 0
   if( forceRefresh == 2 ){
	   
	   osUI_Page = 0;
       lcd.clear();
   }

   char btn = get_button_async(0);

// handle input
   if( btn == BUTTON_MODE ){

       set_mode( OSUI_MODE_SETUP); // next mode - SETUP
       return true;
   }
   else if( btn == BUTTON_UP ){

       if( osUI_Page < 4 ) osUI_Page++;
       else                osUI_Page = 0;

       forceRefresh = 1;
   }
   else if( btn == BUTTON_DOWN ){

       if( osUI_Page > 0 ) osUI_Page--;
       else                osUI_Page = 4;

       forceRefresh = 1;
   }

//   if( forceRefresh != 0 ){   // entering VIEWCONF mode, refresh things
   if( 1 ){   // always refresh

		  LCD_SETCURSOR(lcd, 0, 0);

          if( osUI_Page == 0 ){

             lcd_print_pgm(PSTR("Status: Version "));
//			 LCD_SETCURSOR(lcd, 0, 1);
			 lcd_print_line_clear_pgm(PSTR(VERSION),1);
          }
          else if( osUI_Page == 1 ){
             uint32_t ip = Ethernet.localIP();

             lcd_print_line_clear_pgm(PSTR("Status: IP"), 0);
#if HW_ENABLE_ETHERNET
			 LCD_SETCURSOR(lcd, 0, 1);
             lcd_print_ip((byte *)&ip);
#else // HW_ENABLE_ETHERNET
             lcd_print_line_clear_pgm(PSTR("No Ethernet"), 1);
#endif // HW_ENABLE_ETHERNET
          }
          else if( osUI_Page == 2 ){

             lcd_print_line_clear_pgm(PSTR("Status: Port"), 0);
#if HW_ENABLE_ETHERNET
			 LCD_SETCURSOR(lcd, 0, 1);
			 lcd.print(GetWebPort());
 			 lcd_print_pgm(PSTR("         "));
#else // HW_ENABLE_ETHERNET
             lcd_print_line_clear_pgm(PSTR("No Ethernet"), 1);
#endif // HW_ENABLE_ETHERNET
          }
          else if( osUI_Page == 3 ){
             lcd_print_line_clear_pgm(PSTR("Status: GW"), 0);
#if HW_ENABLE_ETHERNET
			 LCD_SETCURSOR(lcd, 0, 1);
             uint32_t ip = Ethernet.gatewayIP();
             lcd_print_ip((byte *)&ip);
#else // HW_ENABLE_ETHERNET
             lcd_print_line_clear_pgm(PSTR("No Ethernet"), 1);
#endif // HW_ENABLE_ETHERNET
          }
          else if( osUI_Page == 4 ){

			 if( GetLastReceivedStationID() == 255 ){
				lcd_print_line_clear_pgm(PSTR("Last received:"), 0);
				lcd_print_line_clear_pgm(PSTR("No signal"), 1);
			 }
			 else {

				lcd_print_pgm(PSTR("Last received:"));
				lcd_print_2digit(GetLastReceivedStationID());
				LCD_SETCURSOR(lcd, 0, 1);
				lcd_print_pgm(PSTR("T:-"));
				lcd_print_3digit((millis()-GetLastReceivedTime(GetLastReceivedStationID()))/1000);
				lcd_print_pgm(PSTR("s, -")); 
				lcd_print_3digit(-GetLastReceivedRSSI());
				lcd_print_pgm(PSTR("db  "));
			 }
          }
   }

  return true;
}

#ifndef HW_ENABLE_SD

// Setup pages/values are used only with hardcoded config. When SD card is available, device.ini is used instead.

prog_char str_sname1[] PROGMEM = "Station#";

#define SETUP_NUM_SETTINGS	1

struct SetupIndex
{
	char*		name;
	uint8_t		val;
	uint8_t		minval;
	uint8_t		maxval;
};

static SetupIndex setupIndex[SETUP_NUM_SETTINGS] = 
{   {str_sname1, 1, 1, 9},
};

// Helper funciton. This code separated into a helper funciton just for clarity.
inline void SetupLoadValues(void)
{
	setupIndex[0].val = GetMyStationID();
}

// Helper funciton. This code separated into a helper funciton just for clarity.
inline void SetupSaveValues(void)
{
	ResetEEPROM_NoSD( setupIndex[0].val );	// reset EEPROM with the new stationID
}
#endif // HW_ENABLE_SD


#ifndef HW_ENABLE_SD
byte OSLocalUI::modeHandler_Setup(byte forceRefresh)
{
  static unsigned long old_millis = 0;
  static uint16_t	curr_value;			// use 16bit variable to avoid problems with edge conditions
  static byte		state = 0;              // this flag indicates the UI state within Setup mode. Valid states are:
                                            // 0 - viewing conf, osUI_Page provides the index of parameter to display
                                            // 1 - editing setting, osUI_Page provides the index of the parameter we are editing

// assert
  if( osUI_Mode != OSUI_MODE_SETUP )  return false;  // Basic protection to ensure current UI mode is actually HOME mode.

setup_top:
  if( forceRefresh == 2 ){   // entering Setup mode, refresh things

          lcd.clear();
//		  LCD_SETCURSOR(lcd, 0, 0);
//		  lcd_print_pgm(PSTR("Entering Setup mode"));
//		  delay(2000);

          state = 0;
		  osUI_Page = 0;
		  SetupLoadValues();

          forceRefresh == 1;    // need to update the screen
  }
  unsigned long new_millis = millis();

  char btn = get_button_async(1);    // Note: we allow Autorepeat in this mode, to help enter data quickly

  if( state == 0)
  {          // view config

         if( btn == BUTTON_MODE ){

            set_mode( OSUI_MODE_HOME ); // exit Setup mode and switch back to Home
            return true;
         }
#if SETUP_NUM_SETTINGS > 1
         else if( btn == BUTTON_UP ){

                if( osUI_Page < (SETUP_NUM_SETTINGS-1) )	osUI_Page++;
                else									    osUI_Page = 0;
                forceRefresh = 1;
         }
         else if( btn == BUTTON_DOWN ){

				if( osUI_Page > 0 ) osUI_Page--;
                else                osUI_Page = (SETUP_NUM_SETTINGS-1);
                forceRefresh = 1;
         }
#endif // SETUP_NUM_SETTINGS > 1
         else if( btn == BUTTON_CONFIRM ){

				state = 1;									// Entering edit mode, osUI_Page is the index of the setting to edit
				curr_value = setupIndex[osUI_Page].val;   // copy current value from the index 

// display input prompt
                lcd.clear();
                LCD_SETCURSOR(lcd, 0, 0);
                lcd_print_pgm(PSTR("Edit:"));
                lcd_print_pgm(setupIndex[osUI_Page].name);

                return  true;    // exit. Actual current value display will happen on the next loop();
         }
// Display current page (value from the index)

		 if( forceRefresh )
		 {
			LCD_SETCURSOR(lcd, 0, 0);
			lcd_print_pgm(PSTR("Conf:"));
			lcd_print_pgm(setupIndex[osUI_Page].name);
			lcd_print_pgm(PSTR("          "));
			LCD_SETCURSOR(lcd, 0, 1);
			lcd_print_3digit(setupIndex[osUI_Page].val);
		 }
  }

  if( state == 1){          // edit current value

         if( btn == BUTTON_UP ){

				curr_value++;
                if( curr_value > setupIndex[osUI_Page].maxval ) curr_value = setupIndex[osUI_Page].minval;  // rollover

				forceRefresh = 1;
         }
         else if( btn == BUTTON_DOWN ){

				if( curr_value > setupIndex[osUI_Page].minval ) curr_value--;
				else											curr_value = setupIndex[osUI_Page].maxval;
                forceRefresh = 1;
         }
         else if( btn == BUTTON_MODE ){

				set_mode( OSUI_MODE_HOME ); // exit Setup mode and switch back to Home, discarding current value changes
				return true;
         }
         else if( btn == BUTTON_CONFIRM ){
// Save changes

				setupIndex[osUI_Page].val = curr_value;
				SetupSaveValues();

				LCD_SETCURSOR(lcd, 0, 0);
		        lcd_print_pgm(PSTR("Conf:"));
				lcd_print_pgm(setupIndex[osUI_Page].name);
				LCD_SETCURSOR(lcd, 0, 1);
		        lcd_print_pgm(PSTR("New value saved!"));

                delay(1000);
                
				state = 0;
                forceRefresh = 1;
  	            lcd.clear();

                goto setup_top;
     }
// now screen update and blinking cursor
     if( ((new_millis - old_millis) >= 333) || (forceRefresh != 0) ){   // update UI three times a second (for blinking), OR if explicit refresh is required

           old_millis = new_millis;

 		   LCD_SETCURSOR(lcd, 0, 1);
           lcd_print_3digit(curr_value);
           if( (new_millis/333)%2 ) lcd_print_pgm(PSTR("    "));
           else						lcd_print_pgm(PSTR("#   "));
     }

  }
  return true;
}
#else // HW_ENABLE_SD  - dummy Setup
byte OSLocalUI::modeHandler_Setup(byte forceRefresh)
{
	set_mode( OSUI_MODE_HOME ); // exit Setup mode and switch back to Home
	return true;
}
#endif // HW_ENABLE_SD




// various LCD output functions

 // Print station bits
void OSLocalUI::lcd_print_station(void) {

  LCD_SETCURSOR(lcd, 0, 1);
  if (display_board == 0) {
    lcd_print_pgm(PSTR("MC:"));  // Master controller is display as 'MC'
  }
  else {
    lcd_print_pgm(PSTR("S"));
    lcd.print((int)display_board, HEX);
    lcd_print_pgm(PSTR(":"));   // extension boards are displayed as E1, E2...
  }

  if (!GetRunSchedules() && (ActiveZoneNum() == -1) ) {    // "disabled" banner is displayed when schedules are disabled AND there are no currently running zones (to account for manual runs)
    lcd_print_line_clear_pgm(PSTR("-Disabled!-"), 1);
  }
  else {

	  ShortStation	sStation;
	  uint8_t		pos;
	  LoadShortStation(display_board, &sStation);	// load station information
	  uint8_t step = ((millis()/1000)%3);

	  for( pos=0; pos<sStation.numZoneChannels; pos++ )
	  {
		  register uint8_t  zst = GetZoneState(1+pos+sStation.startZone);	// note zone number correction - zones are numbered from 1.
		  if( zst == ZONE_STATE_OFF )
				lcd.print('_');    
		  else if( zst == ZONE_STATE_RUNNING )
				lcd.print(ui_anim_chars[step]); 
		  else if( zst == ZONE_STATE_STARTING )
				lcd.print(ui_anim_chars_1[step]); 
		  else if( zst == ZONE_STATE_STOPPING )
				lcd.print(ui_anim_chars_2[step]); 
	  }

	  for( pos=sStation.numZoneChannels; pos<8; pos++ )
			lcd_print_pgm(PSTR(" "));
  }
  lcd_print_pgm(PSTR("    "));
  LCD_SETCURSOR(lcd, 15, 1);
  lcd.write(nntpTimeServer.GetNetworkStatus()?0:1);
}

 // Print stations string using provided default char, and highlight specific station using provided alt char
void OSLocalUI::lcd_print_station(char def_c, byte sel_stn, char sel_c, byte max_ch) {
  
  LCD_SETCURSOR(lcd, 0, 1);
  if (display_board == 0) {
    lcd_print_pgm(PSTR("MC:"));  // Master controller is displayed as 'MC'
  }
  else {
    lcd_print_pgm(PSTR("S"));
    lcd.print((int)display_board, HEX);
    lcd_print_pgm(PSTR(":"));   // extension boards are displayed as E1, E2...
  }

  for (byte s=0; s<max_ch; s++) {
      lcd.print((s == sel_stn) ? sel_c : def_c);
  }
  lcd_print_pgm(PSTR("    "));
  LCD_SETCURSOR(lcd, 15, 1);
  lcd.write(nntpTimeServer.GetNetworkStatus()?0:1);
}


// Print a program memory string
void OSLocalUI::lcd_print_pgm(const prog_char * str) {
  uint8_t c;
  while((c=pgm_read_byte(str++))!= '\0') {
    OSLocalUI::lcd.print((char)c);
  }
}

// Print a program memory string to a given line with clearing
void OSLocalUI::lcd_print_line_clear_pgm(const prog_char * str, byte line) {
  LCD_SETCURSOR(lcd, 0, line);
  uint8_t c;
  int8_t cnt = 0;
  while((c=pgm_read_byte(str++))!= '\0') {
    OSLocalUI::lcd.print((char)c);
    cnt++;
  }
  for(; (16-cnt) >= 0; cnt ++) lcd_print_pgm(PSTR(" "));
}

void OSLocalUI::lcd_print_2digit(int v)
{
  lcd.print((int)(v/10));
  lcd.print((int)(v%10));
}

void OSLocalUI::lcd_print_3digit(int v)
{
  lcd.print((int)(v/100));
  lcd.print((int)((v%100))/10);
  lcd.print((int)(v%10));
}

// Print time to a given line
void OSLocalUI::lcd_print_time(byte line)
{
  time_t t=now();
  LCD_SETCURSOR(lcd, 0, line);
  lcd_print_2digit(hour(t));

  lcd_print_pgm( t%2 > 0 ? PSTR(":") : PSTR(" ") );                     // flashing ":" in the time display

  lcd_print_2digit(minute(t));
  lcd_print_pgm(PSTR("  "));
  lcd_print_pgm(days_str[weekday_today()]);
  lcd_print_pgm(PSTR(" "));
  lcd_print_2digit(month(t));
  lcd_print_pgm(PSTR("-"));
  lcd_print_2digit(day(t));

// if LCD size allows it, and if there are sensors configured
// show temperature and humidity
#if LOCAL_UI_LCD_X >= 20

  if( sensorsModule.fLCDSensors )
  {
	LCD_SETCURSOR(lcd, 16,0);
	if( sensorsModule.Temperature < 10 )
	{
		lcd_print_pgm(PSTR("  "));
	}
	else if( sensorsModule.Temperature < 100 )
	{
	    lcd_print_pgm(PSTR(" "));
	}
	lcd.print(sensorsModule.Temperature);
	lcd_print_pgm(PSTR("F"));

// show humidity
	LCD_SETCURSOR(lcd, 16,1);
	lcd_print_pgm(PSTR(" "));
	if( sensorsModule.Humidity < 10 )
		lcd_print_pgm(PSTR(" "));

	lcd.print(sensorsModule.Humidity);
	lcd_print_pgm(PSTR("%"));
  }

#endif // LOCAL_UI_LCD_X >= 20
}

int freeRam(void)
{
  extern int __heap_start, *__brkval;
  int v;
  return (int) &v - (__brkval == 0 ? (int) &__heap_start : (int) __brkval);
}

// Print free memory
void OSLocalUI::lcd_print_memory(byte line)
{
  LCD_SETCURSOR(lcd, 0, line);
  lcd_print_pgm(PSTR("Free RAM:        "));

  LCD_SETCURSOR(lcd, 9, line);
  lcd.print(freeRam());

  LCD_SETCURSOR(lcd, 15, line);
  lcd.write(nntpTimeServer.GetNetworkStatus()?0:1);
}

// print ip address
void OSLocalUI::lcd_print_ip(const byte *ip) {
  byte i;

  for (i=0; i<3; i++) {
    lcd.print((int)ip[i]);
    lcd_print_pgm(PSTR("."));
  }
  lcd.print((int)ip[i]);
}

 // Index of today's weekday (Monday is 0)
byte weekday_today() {
  return ((byte)weekday(now())+5)%7; // Time::weekday() assumes Sunday is 1
}

// Local wrapper for manual valve control.
// sid is the station number (from 1!), ontimer is the number of minutes to run
//

// this version uses QuickSchedule mechanism, allows setting run time as well as multi-valve runs
void manual_station_on(byte sid, int ontimer)
{
		uint8_t	n_zones = GetNumZones();
        if( sid >= n_zones ) return;    // basic protection, ensure that required zone number is within acceptable range

        // So, we first end any schedule that's currently running by turning things off then on again.
        ReloadEvents();

        if( ActiveZoneNum() != -1 ){    // something is currently running, turn it off

                runState.TurnOffZones();
        }

        for( byte n=0; n<n_zones; n++ ){

                quickSchedule.zone_duration[n] = 0;  // clear up QuickSchedule to zero out run time for all zones
        }

// set run time for required zone.
//
        quickSchedule.zone_duration[sid] = ontimer;

        runState.StartSchedule(true);
}

byte GetNextStation(byte curr_station)
{
	ShortStation	sStation;
	uint8_t			i = curr_station + 1;

loop:
	if( i >= (MAX_STATIONS-1) )
		i = 0;		// rollover to beginning

	LoadShortStation(i, &sStation );
	if( (sStation.stationFlags & STATION_FLAGS_VALID) && (sStation.stationFlags & STATION_FLAGS_ENABLED) )
		return i;

	i++;  
	if( i == curr_station )		// protection from a case when there are no valid stations 
		return curr_station;	// it should not really happen, but just in case.

	goto loop;
}

byte GetPrevStation(byte curr_station)
{
	ShortStation	sStation;
	uint8_t			i = curr_station;

loop:
	if( i == 0 )
		i = MAX_STATIONS-1;		// rollover to max
	i--;

	LoadShortStation(i, &sStation );
	if( (sStation.stationFlags & STATION_FLAGS_VALID) && (sStation.stationFlags & STATION_FLAGS_ENABLED) )
		return i;

	if( i == curr_station )		// protection from a case when there are no valid stations 
		return curr_station;	// it should not really happen, but just in case.

	goto loop;
}



