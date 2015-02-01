// XBeeRF.cpp
/*
        XBee RF support for SmartGarden

  This module is intended to be used with my SmartGarden multi-station monitoring and sprinkler control system.


This module handles XBee RF communication in SmartGarden system.



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

#include "XBeeRF.h"
#include <XBee.h>
#include "settings.h"
#include "port.h"
#include "RProtocolSlave.h"
#include "localUI.h"
extern OSLocalUI localUI;



// Local forward declarations
bool XBeeSendPacket(uint16_t netAddress, void *msg, size_t mSize);


// Start XBee network if enabled. 
//
// Note: It is assumed that EEPROM is already populated and valid.
//
XBeeRFClass::XBeeRFClass()
{
	fXBeeReady = false;
}

void XBeeRFClass::begin()
{
	trace(F("Starting XBee\n"));

	if( !IsXBeeEnabled() ){

		trace(F("XBee is not enabled, exiting\n"));
		return;					// init the system only if XBee is enabled.
	}

	uint8_t		xbeePort = GetXBeePort();
	uint16_t	xbeeSpeed = GetXBeePortSpeed();

	trace(F("XBee port: %d, speed: %u\n"), xbeePort, xbeeSpeed);

	xbee = XBee();

// Set XBee port and speed
#if defined(__AVR_ATmega1280__) || defined(__AVR_ATmega2560__)
	if( xbeePort == 0 )
	{
		Serial.begin(xbeeSpeed); xbee.setSerial(Serial);
	}
	else if( xbeePort == 1 )
	{
		Serial1.begin(xbeeSpeed); xbee.setSerial(Serial1);
	}
	else if( xbeePort == 2 )
	{
		Serial2.begin(xbeeSpeed); xbee.setSerial(Serial2);
	}
	else if( xbeePort == 3 )
	{
		Serial3.begin(xbeeSpeed); xbee.setSerial(Serial3);
	}
#else  // Mega
		Serial.begin(xbeeSpeed); xbee.setSerial(Serial);	// on Uno always uses Serial
#endif // Mega
// Now set XBee PAN ID, address and other parameters

	uint16_t  panID = NETWORK_XBEE_PANID_HIGH << 8;		// high 8 bit of XBee PAN ID are fixed, low 8 bit are configurable
	panID += GetXBeePANID();

	trace(F("Setting XBeeAddr: %X, PANID:%X, Chan:%X\n"), GetXBeeAddr(), panID, (int)GetXBeeChan() );

	localUI.lcd_print_line_clear_pgm(PSTR("XBee start"), 0);

	if( !sendAtCommandParam(PSTR("MY"), GetXBeeAddr()) )	goto failed_ex1;
	if( !sendAtCommandParam(PSTR("ID"), panID) )			goto failed_ex1;
	if( !sendAtCommandParam(PSTR("CH"), GetXBeeChan()) )	goto failed_ex1;
	if( !sendAtCommand(PSTR("AC")) )						goto failed_ex1;

	SetXBeeFlags(GetXBeeFlags() | NETWORK_FLAGS_ON);	// Mark XBee network as On
	fXBeeReady = true;		// and set local readiness flag

	localUI.lcd_print_line_clear_pgm(PSTR("XBee ready!"), 0);
	delay(1000);

	rprotocol.begin();
	rprotocol.myUnitID = GetMyStationID();

//	rprotocol.RegisterTransport((void*)&XBeeSendPacket);	// register transport Send routine with the remote protocol
//															// rprotocol will use it to send wire packets

// For some mysterios reasons passing callback function as an argumet is rejected by the linker for AtMega328 (not Mega!) but is OK for direct assignment
	rprotocol._SendMessage = (PTransportCallback)&XBeeSendPacket;

failed_ex1:
	return;
}




bool XBeeRFClass::sendAtCommand(const char *cmd_pstr) 
{
	uint8_t	  cmd[2];

	cmd[0] = pgm_read_byte(cmd_pstr);
	cmd[1] = pgm_read_byte(cmd_pstr+1);

	AtCommandRequest atRequest = AtCommandRequest(cmd);
	AtCommandResponse atResponse = AtCommandResponse();

	xbee.send(atRequest);

	if( xbee.readPacket(3000) )		// wait up to 3 seconds for the status response
	{
		// got a response! 
		if( xbee.getResponse().getApiId() == AT_COMMAND_RESPONSE )	// it should be an AT command response
		{  
			xbee.getResponse().getAtCommandResponse(atResponse);
			if( atResponse.isOk() ) 
			{
				return true;	// exit - success
			} 
			else 
			{
				trace(F("sendAtCommand - Command return error code: %X\n"), atResponse.getStatus());
			}
		} 
		else {
			trace(F("sendAtCommand - Expected AT response but got %X\n"), xbee.getResponse().getApiId());
		}   
	} 
	else 
	{
		// at command failed
		if (xbee.getResponse().isError()) 
		{
			trace(F("sendAtCommand - Error reading packet.  Error code: %d\n"), xbee.getResponse().getErrorCode());
		} 
		else {
			trace(F("sendAtCommand - No response from radio\n"));  
		}
	}

	return false;	// failure. successful exit was above
}

bool	XBeeRFClass::sendAtCommandParam(const char *cmd_pstr, uint8_t param8)
{
	return sendAtCommandParam(cmd_pstr, &param8, 1);
}

// Note: XBee uses big endian convention (MSB first), while Arduino is little endian system.
// We need to swap bytes.

bool	XBeeRFClass::sendAtCommandParam(const char *cmd_pstr, uint16_t param16)
{
	uint8_t	 buf2[2];
	register uint8_t	 * pbuf = (uint8_t *)(&param16);
	buf2[0] = pbuf[1];
	buf2[1] = pbuf[0];

	return sendAtCommandParam(cmd_pstr, buf2, 2);
}

// Note: XBee uses big endian convention (MSB first), while Arduino is little endian system.
// We need to reorder bytes.
bool	XBeeRFClass::sendAtCommandParam(const char *cmd_pstr, uint32_t param32)
{
	uint8_t	 buf4[4];
	register uint8_t	 * pbuf = (uint8_t *)(&param32);
	buf4[0] = pbuf[3];
	buf4[1] = pbuf[2];
	buf4[2] = pbuf[1];
	buf4[3] = pbuf[0];

	return sendAtCommandParam(cmd_pstr, buf4, 4);
}


bool XBeeRFClass::sendAtCommandParam(const char *cmd_pstr, uint8_t *param, uint8_t param_len) 
{
	uint8_t	  cmd[2];

	cmd[0] = pgm_read_byte(cmd_pstr);
	cmd[1] = pgm_read_byte(cmd_pstr+1);

	AtCommandRequest atRequest = AtCommandRequest(cmd, param, param_len);
	AtCommandResponse atResponse = AtCommandResponse();

	xbee.send(atRequest);

	if( xbee.readPacket(3000) )		// wait up to 3 seconds for the status response
	{
		// got a response! 
		if( xbee.getResponse().getApiId() == AT_COMMAND_RESPONSE )	// it should be an AT command response
		{  
			xbee.getResponse().getAtCommandResponse(atResponse);
			if( atResponse.isOk() ) 
			{
				return true;		// exit - success
			} 
			else 
			{
				trace(F("sendAtCommandParam - Command return error code: %X\n"), atResponse.getStatus());
			}
		} 
		else {
			trace(F("sendAtCommandParam - Expected AT response but got %X\n"), xbee.getResponse().getApiId());
		}   
	} 
	else 
	{
		// at command failed
		if (xbee.getResponse().isError()) 
		{
			trace(F("sendAtCommandParam - Error reading packet.  Error code: %d\n"), xbee.getResponse().getErrorCode());
		} 
		else {
			trace(F("sendAtCommandParam - No response from radio\n"));  
		}
	}

	return false;	// failure. successful exit was above
}

// Main packet send routine. 
//
//

bool XBeeSendPacket(uint16_t netAddress, void *msg, size_t mSize)
{
	if( !XBeeRF.fXBeeReady )	// check that XBee is initialized and ready
		return false;

	trace(F("XBee - sending packet to station %d, len %d\n"), netAddress, mSize);

	Tx16Request tx = Tx16Request(netAddress, (uint8_t *)msg, mSize);	
	TxStatusResponse txStatus = TxStatusResponse();

	XBeeRF.frameIDCounter++;	// increment rolling counter
	tx.setFrameId(XBeeRF.frameIDCounter);

    XBeeRF.xbee.send(tx);

	freeMemory();

// We don't read XBee resonse here, instead response is delivered to the common receive routine (together with incoming data packets).

	return true;
}

// Mail XBee loop poller. loop() should be called frequently, to allow processing of incoming packets
//
// Loop() will process incoming packets, will interpret remote protocol and will call appropriate handlers.
//
// Data is coming from XBee via serial port, and is buffered there (I think buffer length is 64 bytes), actual data pickup and interpretation
// happens in the loop() funciton.
//
// Note: Both incoming RF packets and send responses are picked up here.

void XBeeRFClass::loop(void)
{
	xbee.readPacket();	// read packet with no wait

	if( xbee.getResponse().isAvailable() )
	{
		// got a response! 
		register uint8_t	responseID = xbee.getResponse().getApiId();

		if(  responseID == TX_STATUS_RESPONSE )			// if we received TX status response
		{  
				// ignore TX responses (for now)
				return;		// exit 
		} 
		else if( responseID == RX_16_RESPONSE )			// actual data packet coming from a remote station
		{											
				Rx16Response rx16 = Rx16Response();		// Note: we are using 16bit addressing
				xbee.getResponse().getRx16Response(rx16);

				uint8_t  *msg = rx16.getFrameData();
				uint8_t	 msg_len = rx16.getFrameDataLength();

				if( msg_len < 5 )
				{
					trace(F("XBee.loop - incoming packet from station %d is too small\n"), rx16.getRemoteAddress16());
					return;
				}

				trace(F("XBee.loop - processing packet from station %d\n"), rx16.getRemoteAddress16());
				rprotocol.ProcessNewFrame(msg+4, msg_len-4, rx16.getRemoteAddress16(), rx16.getRssi());	// process incoming packet.
																		// Note: we don't copy the packet, and just use pointer to the packet already in XBee library buffer
				return;
		} 
		else {
			trace(F("XBee.loop - Unrecognized frame from XBee %d\n"), responseID);
			return;
		}   
	} 
	else if( xbee.getResponse().isError() ) 
	{
		trace(F("XBee.loop - XBee reported error.  Error code: %d\n"), xbee.getResponse().getErrorCode());
		return;
	}

	// Note: XBee error status and current packet will be discarded on the new readPacket(), on the next loop() pass.
}


XBeeRFClass XBeeRF;

