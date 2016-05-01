/*

SmartGarden system.

This project was inspired by Ray's OpenSprinkler project, and by Sprinklers_pi program from Richard Zimmerman

Most of the code is written by Tony-osp (http://tony-osp.dreamwidth.org/),
Some of the modules (nntp, tftp and few other pieces) came from Sprinklers_pi code and are covered by Richard's (c),

*/
#include "MoteinoRF.h"
#include "Defines.h"
#include "port.h"

#include <SdVolume.h>
#include <SdStream.h>
#include <SdSpi.h>
#include <SdInfo.h>
#include <SdFile.h>
#include <SdFatUtil.h>
#include <SdFatmainpage.h>
#include <SdFatConfig.h>
#include <SdBaseFile.h>
#include <Sd2Card.h>
#include <ostream.h>
#include <istream.h>
#include <iostream.h>
#include <ios.h>
#include <bufstream.h>
#include <ArduinoStream.h>

#include "settings.h"
#include "core.h"
#include <Ethernet.h>
#include <SPI.h>
#include <EEPROM.h>
#include <Time.h>
#include <SDFat.h>
#include <LiquidCrystal.h>
#include "LCD_C0220BiZ.h"
#include "LocalUI.h"
#include "sdlog.h"
#include <SFE_BMP180.h>
#include <DHT.h>
#include <Wire.h>
#include "LocalBoard.h"
#include <IniFile.h>
#include "RProtocolMS.h"

#ifdef SG_WDT_ENABLED
#include <avr/wdt.h>
#include "SgWdt.h"
#endif // SG_WDT_ENABLED

#ifdef HW_ENABLE_XBEE
#include <XBee.h>
#include "XBeeRF.h"
#endif //HW_ENABLE_XBEE

#ifdef HW_ENABLE_MOTEINORF
#include "RFM69.h"
#include "MoteinoRF.h"
#endif //HW_ENABLE_MOTEINORF

#include "TimerOne.h"

OSLocalUI localUI;

#ifdef HW_ENABLE_SD
SdFat sd;
#endif //HW_ENABLE_SD

#ifdef HW_ENABLE_ETHERNET
byte mac[] = {0xDE, 0xAD, 0xBE, 0xEF, 0xFE, 0xAD};
#endif //HW_ENABLE_ETHERNET

void	RegisterRemoteEvents(void);


void setup() {

#ifdef SG_WDT_ENABLED
	wdt_disable();
#endif // SG_WDT_ENABLED

	trace_setup(Serial, 115200);    
	TRACE_CRIT(F("Start!\n"));

    localUI.begin();

#ifdef HW_ENABLE_ETHERNET

	pinMode(SS, OUTPUT); digitalWrite(SS, HIGH);	// prep SDI port and turn Off Moteino Mega receiver (it is sitting on SS)
	pinMode(SD_SS, OUTPUT); digitalWrite(SD_SS, HIGH);
	pinMode(W5500_SS, OUTPUT); digitalWrite(W5500_SS, HIGH);
#endif //HW_ENABLE_ETHERNET

    
	if (IsFirstBoot())
	{
// Note: To reset EEPROM we need to initialize SD card. But we must initialize Ethernet first, otherwise SD card will not work.
//		 To get around this dependency we initialize Ethernet with a dummy address and initialize SD card. 
//		 Anyway after EEPROM reset we will have to reboot the controller.

#ifdef HW_ENABLE_ETHERNET
		Ethernet.begin(mac, IPAddress(1,1,1,2), INADDR_NONE, IPAddress(1,1,1,1), IPAddress(255,255,255,0));
#ifdef HW_ENABLE_SD
#ifdef SD_USE_CUSOM_SS
		if (!sd.begin(SD_SS, SPI_FULL_SPEED) )  
#else //SD_USE_CUSOM_SS
	if (!sd.begin(4, SPI_HALF_SPEED) ) 
#endif //SD_USE_CUSOM_SS
		{
			SYSEVT_ERROR(F("Could not Initialize SDCard"));
			localUI.lcd_print_line_clear_pgm(PSTR("EEPROM Corrupted"), 0);
			localUI.lcd_print_line_clear_pgm(PSTR("SD CARD FAILURE!"), 1);
			delay(10000);
			sysreset();
		}
#endif //HW_ENABLE_SD
#endif //HW_ENABLE_ETHERNET
		ResetEEPROM();	// note: ResetEEPROM will also reset the controller.
	}

#ifdef HW_ENABLE_XBEE
    localUI.lcd_print_line_clear_pgm(PSTR("XBee RF init..."), 1);
	XBeeRF.begin();
#endif //HW_ENABLE_XBEE

#ifdef HW_ENABLE_MOTEINORF
    localUI.lcd_print_line_clear_pgm(PSTR("MoteinoRF init..."), 1);
	MoteinoRF.begin();
#endif //HW_ENABLE_XBEE

#ifdef HW_ENABLE_ETHERNET

	//pinMode(SS, OUTPUT); digitalWrite(SS, HIGH);	// prep SDI port and turn Off Moteino Mega receiver (it is sitting on SS)
	//pinMode(SD_SS, OUTPUT); digitalWrite(SD_SS, HIGH);
	pinMode(W5500_SS, OUTPUT); digitalWrite(W5500_SS, HIGH);

	localUI.lcd_print_line_clear_pgm(PSTR("Ethernet init..."), 1);
	// start the Ethernet connection and the server:
	if( GetIsDHCP() )
		Ethernet.begin(mac);
	else
		Ethernet.begin(mac, GetIP(), INADDR_NONE, GetGateway(), GetNetmask());
	TRACE_CRIT(F("Assigned IP:")); Serial.println(Ethernet.localIP());

    // give the Ethernet shield time to set up:
    delay(1000);    

#endif //HW_ENABLE_ETHERNET

#ifdef HW_ENABLE_SD
#ifdef SD_USE_CUSOM_SS
	if (!sd.begin(SD_SS, SPI_FULL_SPEED)) 
#else //SD_USE_CUSOM_SS
	if (!sd.begin(4, SPI_HALF_SPEED)) 
#endif //SD_USE_CUSOM_SS
	{
		SYSEVT_ERROR(F("Could not Initialize SDCard"));
	}
	else
	{
		TRACE_INFO(F("SDCard init success\n"));
	}
#endif //HW_ENABLE_SD

#ifdef SG_WDT_ENABLED
	SgWdtBegin();
#endif // SG_WDT_ENABLED

	RegisterRemoteEvents();
}

void loop() {
    mainLoop();
    localUI.loop();
	rprotocol.loop();

#ifdef SG_WDT_ENABLED
	SgWdtReset();
#endif // SG_WDT_ENABLED
}


// Go through remote stations and register myself as EvtMaster. Called as a part of the initialization sequence.
//
// (p.s. Really should have a separate remote stations manager, but can put it here for now)
//
void	RegisterRemoteEvents(void)
{
		for( uint8_t i=1; i<MAX_STATIONS; i++ )		// iterate through stations starting from 1, since station 0 is always local
		{
			rprotocol.SubscribeEvents( i );			// we are relying on the station enable and network type check inside SubscribeEvents()
		}
}
