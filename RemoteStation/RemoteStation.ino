// SmartGarden system - multi-station environment monitoring and irrigation sprinklers control
//
// Author: Tony-osp (tony-osp.dreamwidth.org)
// Copyright (c) 2014 Tony-osp
//


//#include <MinimumSerial.h>
#include "settings.h"
#include "core.h"
#include <SPI.h>
#include <EEPROM.h>
#include <Time.h>

#include <sfe_bmp180.h>      // BMP180 pressure/temp sensor
#include <DHT.h>

#include <Wire.h>
#include <LCD_C0220BiZ.h>      //  This is I2C-connected LCD support library

// Common modules shared by both Remote and Master stations

#include "localUI.h"
#include "SGSensors.h"
#include "port.h"
#include <XBee.h>
#include "SGRProtocol.h"
#include "XBeeRF.h"
#include "sdlog.h"

OSLocalUI localUI;

void setup() {
    Serial.begin(57600); 
    trace_setup(Serial);
    trace(F("Start!\n"));

    localUI.begin();
    localUI.lcd_print_line_clear_pgm(PSTR("Connecting..."), 1);

	XBeeRF.begin();

	if (IsFirstBoot())
		ResetEEPROM();

   
   localUI.set_mode(OSUI_MODE_HOME);  // set to HOME mode, page 0
   localUI.resume();
}

void loop() {
    mainLoop();
    localUI.loop();
	XBeeRF.loop();
}

