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


#include "localUI.h"
#include "core.h"
#include "settings.h"
#include "SGSensors.h"

// External references
extern Sensors sensorsModule;


// Data members
byte OSLocalUI::osUI_State = OSUI_STATE_UNDEFINED;
byte OSLocalUI::osUI_Mode  = OSUI_MODE_UNDEFINED;
byte OSLocalUI::osUI_Page  = OSUI_PAGE_UNDEFINED;
byte OSLocalUI::display_board = 0;                                                      // currently displayed board


// this is 1284p version
//LiquidCrystal OSLocalUI::lcd(23, 22, 27, 26, 25, 24);

#ifdef USE_I2C_LCD
ST7036 OSLocalUI::lcd( LOCAL_UI_LCD_Y, LOCAL_UI_LCD_X, 0x78 );
#else
// and this is Mega with big LCD version
LiquidCrystal OSLocalUI::lcd(PIN_LCD_RS, PIN_LCD_EN, PIN_LCD_D4, PIN_LCD_D5, PIN_LCD_D6, PIN_LCD_D7);
#endif

// Local forward declarations


byte get_button_async(byte mode);
bool getNetworkStatus(void);
bool isStationEnabled(void);
void disableStation(void);
void enableStation(void);


byte weekday_today(void);
int get_key(unsigned int input);
void buttons_loop(void);
void manual_station_on(byte sid, int ontimer);
void manual_station_off(byte sid);


// button functions

byte button_read_busy(byte pin_butt, byte waitmode, byte butt, byte is_holding);        // Wait for button
byte button_read(byte waitmode);                                                        // Read button and returns button value 'OR'ed with flag bits
void ui_set_options(int oid);                                                           // user interface for setting options during startup



// ====== UI defines ======

static char ui_anim_chars[3] = {'.', 'o', 'O'};

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
     osUI_Mode       = OSUI_MODE_HOME;                  // home screen by default
     osUI_Page       = 0;                               // first page by default (pages start form 0)

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
	lcd_custom_char[1]=0;
    lcd_custom_char[2]=0;
    lcd_custom_char[3]=1;
#ifdef USE_I2C_LCD
	lcd.load_custom_character(0, lcd_custom_char);
#else
	lcd.createChar(0, lcd_custom_char);
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

// Main loop. Intended to be called regularly and frequently to handle input and UI. Normally this will be called from Arduino loop()
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
   if( osUI_Mode == OSUI_MODE_HOME )                    return modeHandler_Home(needs_refresh);
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

     display_board = 0;         // set initial view to board 0;

   }

   char btn = get_button_async(0);

// handle input
   if( btn == BUTTON_MODE ){

       set_mode( OSUI_MODE_MANUAL ); // change mode to "view settings" which is the next mode
       return true;
   }
   else if( btn == BUTTON_CONFIRM ){

	   if( isStationEnabled() )
		   disableStation();
	   else
		   enableStation();

       forceRefresh = 1;             // force UI refresh to provide immediate visual feedback
   }

 // Show time and station status
// if 1 second has passed
  unsigned long  new_millis = millis();    // Note: we are using built-in Arduino millis() function instead of now() or time-zone adjusted LocalNow(), because it is a lot faster
                                                             // and for detecting second change it does not make any difference.

  if( ((new_millis - old_millis) >= 1000) || (forceRefresh != 0) ){   // update UI once a second, OR if explicit refresh is required

    old_millis = new_millis;
    lcd_print_time(0);       // print time

    // process LCD display
    if(SHOW_MEMORY)
      lcd_print_memory(1);
    else
      lcd_print_station(1, ui_anim_chars[now()%3]);
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

// assert
  if( osUI_Mode != OSUI_MODE_MANUAL )  return false;  // Basic protection to ensure current UI mode is actually HOME mode.

  if( forceRefresh == 2 ){   // entering MANUAL mode, refresh things

          lcd.clear();
		  LCD_SETCURSOR(lcd, 0, 0);
          lcd_print_pgm(PSTR("Manual Start:"));

          man_state = 0;
          sel_manual_ch = 0;
          forceRefresh == 1;    // need to update the screen
  }
  unsigned long new_millis = millis();

  char btn = get_button_async(1);    // Note: we allow Autorepeat in this mode, to help enter data quickly

  if( man_state == 0){          // select channel screen

         if( btn == BUTTON_UP ){

                if( sel_manual_ch < 7 ) sel_manual_ch++;
                else                            sel_manual_ch = 0;
                forceRefresh = 1;
         }
         else if( btn == BUTTON_DOWN ){

            if( sel_manual_ch > 0 ) sel_manual_ch--;
                else                            sel_manual_ch = 7;
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

     if( ((new_millis - old_millis) >= 1000) || (forceRefresh != 0) ){   // update UI once a second (for blinking), OR if explicit refresh is required

//     if( (last_time != curr_time) || (forceRefresh != 0) ) {  // update UI once a second (for blinking), OR if explicit refresh is required

       old_millis = new_millis;
       lcd_print_station(1, '_', sel_manual_ch, (now()%2) ? '#':'_');       // this will blink selected station
     }
  }

 min_to_run_sel:
  if( man_state == 1){          // enter the number of minutes to run

         if( btn == BUTTON_UP ){

                if( num_min < 99 ) num_min++;
                else                       num_min = 0;
                forceRefresh = 1;
         }
         else if( btn == BUTTON_DOWN ){

            if( num_min > 0 ) num_min--;
                else                      num_min = 99;
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

				LCD_SETCURSOR(lcd, 0, 1);
                lcd_print_pgm(PSTR("Ch: "));    lcd_print_2digit(sel_manual_ch+1);    //note: actual useful zones are numbered from 1, adjust it for display
                lcd_print_pgm(PSTR(" Min: "));  lcd_print_2digit(num_min);

                delay(2000);

                if( num_min != 0 )
                      manual_station_on((byte)(sel_manual_ch), num_min);        // start required station in manual mode for num_min only if required time is != 0
				else
					manual_station_off((byte)(sel_manual_ch));

                set_mode( OSUI_MODE_HOME ); // Manual watering started, exit current UI mode changing it to HOME
                return true;
     }
// now screen update and blinking cursor
     if( ((new_millis - old_millis) >= 1000) || (forceRefresh != 0) ){   // update UI once a second (for blinking), OR if explicit refresh is required
//     if( (last_time != curr_time) || (forceRefresh !=0) ) {  // update UI once a second (for blinking), OR if explicit refresh is required

           old_millis = new_millis;

 		   LCD_SETCURSOR(lcd, 0, 1);
           lcd_print_2digit(num_min);
           if( now()%2 ) lcd_print_pgm(PSTR("   "));
           else          lcd_print_pgm(PSTR("#  "));
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

       set_mode( OSUI_MODE_SETUP); // change mode to Setup
       return true;
   }
   else if( btn == BUTTON_UP ){

       if( osUI_Page < 3 ) osUI_Page++;
       else                         osUI_Page = 0;

       forceRefresh = 1;
   }
   else if( btn == BUTTON_DOWN ){

       if( osUI_Page > 0 ) osUI_Page--;
       else                         osUI_Page = 3;

       forceRefresh = 1;
   }

//   if( forceRefresh ){   // entering viewconf model
   if( 1 ){   // always refresh

		  LCD_SETCURSOR(lcd, 0, 0);

          if( osUI_Page == 0 ){

             lcd_print_pgm(PSTR("Status: Version"));
			 LCD_SETCURSOR(lcd, 0, 1);
             lcd_print_pgm(PSTR(VERSION));
          }
          else if( osUI_Page == 1 ){

			 if( GetLastReceivedStationID() == 255 ){
				lcd_print_pgm(PSTR("Last received     "));
				LCD_SETCURSOR(lcd, 0, 1);
				lcd_print_pgm(PSTR("No signal       "));
			 }
			 else {

				lcd_print_pgm(PSTR("Last received:"));
				lcd_print_2digit(GetLastReceivedStationID());
				LCD_SETCURSOR(lcd, 0, 1);
				lcd_print_pgm(PSTR(" RSSI:"));
				lcd_print_3digit(GetLastReceivedRssi(GetLastReceivedStationID()));
				lcd_print_pgm(PSTR(" T:-"));
				lcd_print_3digit((millis()-GetLastReceivedTime(GetLastReceivedStationID()))/1000);
			 }
          }
          else if( osUI_Page == 2 ){

             lcd_print_pgm(PSTR("Status page 2"));
			 LCD_SETCURSOR(lcd, 0, 1);
          }
          else if( osUI_Page == 3 ){

             lcd_print_pgm(PSTR("Status page 3"));
			 LCD_SETCURSOR(lcd, 0, 1);
          }
   }

  return true;
}

// Setup pages/values 
prog_char str_sname1[] PROGMEM = "Station#";
prog_char str_sname2[] PROGMEM = "PAN ID";
prog_char str_sname3[] PROGMEM = "XBee Chan.";

#define SETUP_NUM_SETTINGS	3

struct SetupIndex
{
	char*		name;
	uint8_t		val;
	uint8_t		minval;
	uint8_t		maxval;
};

static SetupIndex setupIndex[SETUP_NUM_SETTINGS] = 
{   {str_sname1, 1, 1, 9},
	{str_sname2, 1, 1, 254},
	{str_sname3, 7, 0, 23}
};

// Helper funciton. This code separated into a helper funciton just for clarity.
inline void SetupLoadValues(void)
{
	setupIndex[0].val = GetMyStationID();
	setupIndex[1].val = GetXBeePANID();
	setupIndex[2].val = GetXBeeChan();
}

// Helper funciton. This code separated into a helper funciton just for clarity.
inline void SetupSaveValues(void)
{
	SetMyStationID(setupIndex[0].val);
	SetXBeeAddr(setupIndex[0].val);			// by convention XBee address equals StationID
	SetXBeePANID(setupIndex[1].val);
	SetXBeeChan(setupIndex[2].val);
}


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

         if( btn == BUTTON_UP ){

                if( osUI_Page < (SETUP_NUM_SETTINGS-1) )	osUI_Page++;
                else										osUI_Page = 0;
                forceRefresh = 1;
         }
         else if( btn == BUTTON_DOWN ){

				if( osUI_Page > 0 ) osUI_Page--;
                else                osUI_Page = (SETUP_NUM_SETTINGS-1);
                forceRefresh = 1;
         }
         else if( btn == BUTTON_MODE ){

            set_mode( OSUI_MODE_HOME ); // exit Setup mode and switch back to Home
            return true;
         }
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




// various LCD output functions

 // Print station bits
void OSLocalUI::lcd_print_station(byte line, char c)
{
  LCD_SETCURSOR(lcd, 0, line);

  if( GetMyStationID() == 0) {
    lcd_print_pgm(PSTR("MC:"));  // Master controller is display as 'MC', but we should really never see it here (retaining code for compat)
  }
  else {
    lcd_print_pgm(PSTR("S"));	// show our StationID
    lcd.print((int)GetMyStationID());
    lcd_print_pgm(PSTR(":"));   // extension boards are displayed as E1, E2...
  }

  if (!isStationEnabled() ) {    // "disabled" banner is displayed when schedules are disabled AND there are no currently running zones (to account for manual runs)
    lcd_print_line_clear_pgm(PSTR("-Disabled!-"), 1);
  }
  else {
    for (byte s=0; s<8; s++) {
      lcd.print(isZoneOn(1+s+display_board*8) ? (char)c : '_');    // note zone number correction - zones are numbered from 1.
    }
  }
  lcd_print_pgm(PSTR("    "));
  LCD_SETCURSOR(lcd, 14, 1);
  lcd.write(getNetworkStatus()?0:1);
}

 // Print stations string using provided default char, and highlight specific station using provided alt char
void OSLocalUI::lcd_print_station(byte line, char def_c, byte sel_stn, char sel_c) {
  LCD_SETCURSOR(lcd, 0, line);

  if (display_board == 0) {
    lcd_print_pgm(PSTR("MC:"));  // Master controller is display as 'MC'
  }
  else {
    lcd_print_pgm(PSTR("E"));
    lcd.print((int)display_board);
    lcd_print_pgm(PSTR(":"));   // extension boards are displayed as E1, E2...
  }

  for (byte s=0; s<8; s++) {
      lcd.print((s == sel_stn) ? sel_c : def_c);
  }
  lcd_print_pgm(PSTR("    "));
  LCD_SETCURSOR(lcd, 14, 1);
  lcd.write(getNetworkStatus()?0:1);
}


// Print a program memory string
void OSLocalUI::lcd_print_pgm(PGM_P PROGMEM str) {
  uint8_t c;
  while((c=pgm_read_byte(str++))!= '\0') {
    OSLocalUI::lcd.print((char)c);
  }
}

// Print a program memory string to a given line with clearing
void OSLocalUI::lcd_print_line_clear_pgm(PGM_P PROGMEM str, byte line) {
  LCD_SETCURSOR(lcd, 0, line);
  uint8_t c;
  int8_t cnt = 0;
  while((c=pgm_read_byte(str++))!= '\0') {
    OSLocalUI::lcd.print((char)c);
    cnt++;
  }
  for(; (12-cnt) >= 0; cnt ++) lcd_print_pgm(PSTR(" "));
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
  lcd_print_pgm(PSTR(" "));
  lcd_print_pgm(days_str[weekday_today()]);
  lcd_print_pgm(PSTR(" "));
  lcd_print_2digit(month(t));
  lcd_print_pgm(PSTR("-"));
  lcd_print_2digit(day(t));

// show temperature
  LCD_SETCURSOR(lcd, 16,line);
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

  LCD_SETCURSOR(lcd, 14, line);
  lcd.write(getNetworkStatus()?0:1);
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
  return ((byte)weekday()+5)%7; // Time::weekday() assumes Sunday is 1
}

// Local wrapper for manual valve control.
// sid is the station number (from 1!), ontimer is the number of minutes to run
//
void manual_station_on(byte sid, int ontimer)
{
        if( sid >= GetNumZones() ) return;    // basic protection, ensure that required zone number is within acceptable range

        if( ActiveZoneNum() != -1 ){    // something is currently running, turn it off

			runState.StopAllZones();
		}


// set run time for required zone.
//
	runState.StartZone(true, 100, sid+1, ontimer);
}

void manual_station_off(byte sid)
{
	runState.StopAllZones();
}



static bool _isStationEnabled = true;

// Local wrapper for getting station enable/disable status
//
bool isStationEnabled(void)
{
  return _isStationEnabled;
}

// Local wrappers for enabling/disabling the status
//
void disableStation(void)
{

  _isStationEnabled = false;
}


void enableStation(void)
{

  _isStationEnabled = true;
}

