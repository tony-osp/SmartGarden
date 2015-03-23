// XBeeRF.cpp
/*
        XBee RF support for SmartGarden

  This module is intended to be used with my SmartGarden multi-station monitoring and sprinkler control system.


This module handles XBee RF communication in SmartGarden system.


 Creative Commons Attribution-ShareAlike 3.0 license
 (c) 2015 Tony-osp (tony-osp.dreamwidth.org)

*/

#include "XBeeRF.h"
#include <XBee.h>
#include "settings.h"
#include "port.h"
#include "RProtocolSlave.h"
#include "localUI.h"
extern OSLocalUI localUI;

#define XBEE_TYPE_PRO900 1	// we are using XBee Pro 900 module

XBeeRFClass XBeeRF;

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

#else  // Mega or Moteino Mega (1284p)
#if defined(__AVR_ATmega1284P__) || defined(__AVR_ATmega1284p__)

		Serial1.begin(xbeeSpeed); xbee.setSerial(Serial1);	// on Moteino Mega we are using Serial1

#else // Uno or equivalent

		Serial.begin(xbeeSpeed); xbee.setSerial(Serial);	// on Uno always uses Serial

#endif // Moteino Mega (1284p)
#endif // Mega or Moteino Mega

// Now set XBee PAN ID, address and other parameters

	uint16_t  panID = NETWORK_XBEE_PANID_HIGH << 8;		// high 8 bit of XBee PAN ID are fixed, low 8 bit are configurable
	panID += GetXBeePANID();

	trace(F("Setting XBeeAddr: %X, PANID:%X, Chan:%X\n"), GetXBeeAddr(), panID, (int)GetXBeeChan() );

	localUI.lcd_print_line_clear_pgm(PSTR("XBee start"), 0);

#ifndef XBEE_TYPE_PRO900		// Pro 900 does not support MY addressing command
	if( !sendAtCommandParam(PSTR("MY"), GetXBeeAddr()) )
	{
		trace(F("XBee init - failed to set Xbee Addr\n"));
		goto failed_ex1;
	}
#endif
	if( !sendAtCommandParam(PSTR("ID"), panID ) )
	{
		trace(F("XBee init - failed to set Xbee PAN ID\n"));
		goto failed_ex1;
	}
#ifdef XBEE_TYPE_PRO900		// Xbee Pro 900 uses different channel selection commands
	if( !sendAtCommandParam(PSTR("HP"), GetXBeeChan()) )
	{
		trace(F("XBee init - failed to set Xbee Chan\n"));
		goto failed_ex1;
	}
#else	// regular Xbee (2.4GHz)
	if( !sendAtCommandParam(PSTR("CH"), GetXBeeChan()) )
	{
		trace(F("XBee init - failed to set Xbee Chan\n"));
		goto failed_ex1;
	}
#endif
	if( !sendAtCommand(PSTR("AC")) )
	{
		trace(F("XBee init - failed to execute AC command\n"));
		goto failed_ex1;
	}

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
	return;

failed_ex1:
	localUI.lcd_print_line_clear_pgm(PSTR("XBee failed."), 1);
	delay(5000);
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

#ifdef XBEE_TYPE_PRO900	// XBee pro uses slightly different format and conventions
//
// For XBee Pro 900 we cannot use 16bit addressing, but 64bit addressing is too must hassle to setup - have to 
//   either pre-configure 64bit addresses for all stations, or have to use some form of dynamic discovery.
// Instead, we are using broadcast mode, relying on the RProtocol stack to filter out right packets.
//
bool XBeeSendPacket(uint16_t netAddress, void *msg, size_t mSize)
{
	if( !XBeeRF.fXBeeReady )	// check that XBee is initialized and ready
		return false;

	trace(F("XBee - sending packet to station %d, len %d\n"), netAddress, mSize);

	static Tx64Request tx = Tx64Request();	
	static XBeeAddress64  addrBroadcast = XBeeAddress64(0, 0xFFFF);

	tx.setAddress64(addrBroadcast);		// since our XBee objects are pre-allocated, set parameters on the existing objects
	tx.setOption(8);					// send multicast
	tx.setPayload((uint8_t *)msg);
	tx.setPayloadLength(mSize);

	XBeeRF.frameIDCounter++;	// increment rolling counter
	tx.setFrameId(0);

    XBeeRF.xbee.send(tx);

// No response from XBee is requested

	return true;
}
#else	// regular (2.4HGz XBee)
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
#endif	//XBEE_TYPE_PRO900

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

		if( xbee.getResponse().isError() )
		{
			trace(F("XBee.loop - got packet, but it is flagged as error\n"));
			return;
		}
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
		else if( responseID == ZB_RX_RESPONSE )			// XBee Pro 900 uses this packet format when receiving 
		{											
				uint8_t  *msg = xbee.getResponse().getFrameData();
				uint8_t	 msg_len = xbee.getResponse().getFrameDataLength();

				static ZBRxResponse rx64 = ZBRxResponse();		// Note: we are using 64bit addressing
				xbee.getResponse().getZBRxResponse(rx64);
				
				uint16_t  r16Addr = rx64.getRemoteAddress64().getLsb();

//				trace(F("XBee.loop - received packed from %d, len=%d\n"), r16Addr, (int)msg_len);
				if( msg_len < 12 )
				{
					trace(F("XBee.loop - incoming packet from station %d is too small\n"), r16Addr);
					return;
				}

				rprotocol.ProcessNewFrame(msg+11, msg_len-11, r16Addr, 0);	// process incoming packet.
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



