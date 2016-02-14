// XBeeRF.cpp
/*
        XBee RF support for SmartGarden

Creative Commons Attribution-ShareAlike 3.0 license
Copyright 2014 tony-osp (http://tony-osp.dreamwidth.org/)
 
*/

#include "XBeeRF.h"
#include <XBee.h>
#include "settings.h"
#include "RProtocolMS.h"

//#define TRACE_LEVEL			6		// trace everything for this module
#include "port.h"


//// Global core XBee object
XBee	xbee;				


// Local forward declarations
bool XBeeSendPacket(uint8_t nStation, void *msg, uint8_t mSize);
bool XBeeARPUpdate(uint8_t nStation, uint8_t *pNetAddress);


// Start XBee network if enabled. 
//
// Note: It is assumed that EEPROM is already populated and valid.
//
XBeeRFClass::XBeeRFClass()
{
	fXBeeReady = false;
	for( uint8_t i=0; i<MAX_STATIONS; i++ )
	{
		arpTable[i].MSB = arpTable[i].LSB = 0;
	}
}

void XBeeRFClass::begin()
{
	TRACE_INFO(F("Starting XBee\n"));

	if( !IsXBeeEnabled() ){

		SYSEVT_ERROR(F("XBee is not enabled, exiting"));
		return;					// init the system only if XBee is enabled.
	}

	uint8_t		xbeePort = GetXBeePort();
	uint16_t	xbeeSpeed = GetXBeePortSpeed();

	TRACE_INFO(F("XBee port: %d, speed: %u\n"), xbeePort, xbeeSpeed);

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

	TRACE_INFO(F("Setting XBeeAddr: %X, PANID:%X, Chan:%X\n"), GetXBeeAddr(), GetXBeePANID(), (int)GetXBeeChan() );

	if( !sendAtCommandParam(PSTR("ID"), GetXBeePANID()) )
	{
		SYSEVT_ERROR(F("XBee init - failed to set Xbee PAN ID"));
		goto failed_ex1;
	}
#ifdef XBEE_TYPE_PRO900		// Xbee Pro 900 uses different channel selection commands
	if( !sendAtCommandParam(PSTR("HP"), GetXBeeChan()) )
	{
		SYSEVT_ERROR(F("XBee init - failed to set Xbee Chan"));
		goto failed_ex1;
	}
#else	// regular Xbee (2.4GHz)
	if( !sendAtCommandParam(PSTR("CH"), GetXBeeChan()) )
	{
		SYSEVT_ERROR(F("XBee init - failed to set Xbee Chan"));
		goto failed_ex1;
	}
#endif
	if( !sendAtCommand(PSTR("AC")) )
	{
		SYSEVT_ERROR(F("XBee init - failed to execute AC command"));
		goto failed_ex1;
	}

	SetXBeeFlags(GetXBeeFlags() | NETWORK_FLAGS_ON);	// Mark XBee network as On
	fXBeeReady = true;		// and set local readiness flag

	rprotocol.RegisterARP((void *)&XBeeARPUpdate);			// register ARP callback with the remote protocol
															// rprotocol will use it to report station=address associations

	return;

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
				SYSEVT_ERROR(F("sendAtCommand - Command return error code: %X"), atResponse.getStatus());
			}
		} 
		else {
			SYSEVT_ERROR(F("sendAtCommand - Expected AT response but got %X"), xbee.getResponse().getApiId());
		}   
	} 
	else 
	{
		// at command failed
		if (xbee.getResponse().isError()) 
		{
			SYSEVT_ERROR(F("sendAtCommand - Error reading packet.  Error code: %d"), xbee.getResponse().getErrorCode());
		} 
		else {
			SYSEVT_ERROR(F("sendAtCommand - No response from radio"));  
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
				SYSEVT_ERROR(F("sendAtCommandParam - Command return error code: %X"), atResponse.getStatus());
			}
		} 
		else {
			SYSEVT_ERROR(F("sendAtCommandParam - Expected AT response but got %X"), xbee.getResponse().getApiId());
		}   
	} 
	else 
	{
		// at command failed
		if (xbee.getResponse().isError()) 
		{
			SYSEVT_ERROR(F("sendAtCommandParam - Error reading packet.  Error code: %d"), xbee.getResponse().getErrorCode());
		} 
		else {
			SYSEVT_ERROR(F("sendAtCommandParam - No response from radio"));  
		}
	}

	return false;	// failure. successful exit was above
}

// ARP table maintenance callback
// RProtocol will call this callback to report station->address assocations
//
bool XBeeARPUpdate(uint8_t nStation, uint8_t *pNetAddress)
{
	if( nStation >= MAX_STATIONS ) return false;	// basic protection

	TRACE_INFO(F("Updating ARP table for station %d\n"), uint16_t(nStation) );

	// Note: Since we read network address directly from the input buffer it will be in big endian (XBee convention) and we need to convert it to little endian
	//       When sending we are going through the xbee library that will do endian conversion

	register uint8_t	 * buf4 = (uint8_t *)(&(XBeeRF.arpTable[nStation].MSB));
	buf4[0] = pNetAddress[3];
	buf4[1] = pNetAddress[2];
	buf4[2] = pNetAddress[1];
	buf4[3] = pNetAddress[0];

	buf4 = (uint8_t *)(&(XBeeRF.arpTable[nStation].LSB));

	buf4[0] = pNetAddress[7];
	buf4[1] = pNetAddress[6];
	buf4[2] = pNetAddress[5];
	buf4[3] = pNetAddress[4];

	return true;
}


// Main packet send routine. 
//
//


// It is impractical to pre-configure 64bit addresses, and it is not possible to set 64bit address - it is hardcoded in Xbee module.
//
// Instead, we are using dynamic discovery. First we send the packet using broadcast mode, relying on the RProtocol stack to filter out right packets,
//   and once remote station responds - we cache its 64bit address in arpTable and will subsequently use the cached 64bit address for directed send. 
// The reason for this is the preference to use targeted RF send commands (more efficient and more reliable), but until we know remote address we have to use 
//   broadcast mode.
//
bool XBeeSendPacket(uint8_t nStation, void *msg, uint8_t mSize)
{
	if( !XBeeRF.fXBeeReady )	// check that XBee is initialized and ready
		return false;

	TRACE_INFO(F("XBee - sending packet to station %d, len %u\n"), nStation, (unsigned int)mSize);

	static Tx64Request tx = Tx64Request();						// pre-allocated, static objects to avoid dynamic memory issues
	static XBeeAddress64  addrBroadcast = XBeeAddress64(0, 0xFFFF);
	static XBeeAddress64  addr64 = XBeeAddress64(0, 0);

	if( nStation < MAX_STATIONS )
	{
		if( XBeeRF.arpTable[nStation].LSB != 0 )
		{
			addr64.setLsb(XBeeRF.arpTable[nStation].LSB);
			addr64.setMsb(XBeeRF.arpTable[nStation].MSB);

			tx.setAddress64(addr64);					
			tx.setOption(0);

			TRACE_INFO(F("Sending message as Unicast to station %lX:%lX\n"), XBeeRF.arpTable[nStation].MSB, XBeeRF.arpTable[nStation].LSB );
		}
		else
		{
			tx.setAddress64(addrBroadcast);								// since our XBee objects are pre-allocated, set parameters on the existing objects
			tx.setOption(8);	// send multicast

			TRACE_INFO(F("Station not in ARP table, sending packet as broadcast to station %d\n"), uint16_t(nStation) );
		}
	}
	else  // intended to be broadcast packet.
	{
		tx.setAddress64(addrBroadcast);								// since our XBee objects are pre-allocated, set parameters on the existing objects
		tx.setOption(8);	// send multicast

		TRACE_INFO(F("Sending broadcast message\n") );
	}

	tx.setPayload((uint8_t *)msg);
	tx.setPayloadLength(mSize);

	XBeeRF.frameIDCounter++;	// increment rolling counter
	tx.setFrameId(0);			// set FrameID to 0, which means that XBee will not give us TX confirmation response.

    xbee.send(tx);

// No response from XBee is requested

	return true;
}


// Main XBee loop poller. loop() should be called frequently, to allow processing of incoming packets
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
				static	Rx16Response rx16 = Rx16Response();		// Note: we are using 16bit addressing
				
				xbee.getResponse().getRx16Response(rx16);

				uint8_t  *msg = rx16.getFrameData();
				uint8_t	 msg_len = rx16.getFrameDataLength();

				if( msg_len < 5 )
				{
					SYSEVT_ERROR(F("XBee.loop - incoming packet from station %d is too small"), rx16.getRemoteAddress16());
					return;
				}

				TRACE_VERBOSE(F("XBee.loop - processing packet from station %d"), rx16.getRemoteAddress16());
				rprotocol.ProcessNewFrame(msg+4, msg_len-4, 0 );	// process incoming packet.
																		// Note: we don't copy the packet, and just use pointer to the packet already in XBee library buffer
				return;
		} 
		else if( responseID == ZB_RX_RESPONSE )			// XBee Pro 900 uses this packet format when receiving 
		{											
				uint8_t  *msg = xbee.getResponse().getFrameData();
				uint8_t	 msg_len = xbee.getResponse().getFrameDataLength();

				static ZBRxResponse rx64 = ZBRxResponse();		// Note: we are using 64bit addressing
				xbee.getResponse().getZBRxResponse(rx64);
				
//				uint16_t  r16Addr = rx64.getRemoteAddress64().getLsb();

//				SYSEVT_ERROR(F("XBee.loop - received packed from %d, len=%d"), r16Addr, (int)msg_len);
				if( msg_len < 12 )
				{
					SYSEVT_ERROR(F("XBee.loop - incoming packet is too small"));
					return;
				}

				rprotocol.ProcessNewFrame(msg+11, msg_len-11, msg);		// process incoming packet.
																		// Note: we don't copy the packet, and just use pointer to the packet already in XBee library buffer
				return;
		} 

		else {
			SYSEVT_ERROR(F("XBee.loop - Unrecognized frame from XBee %d"), responseID);
			return;
		}   
	} 
	else if( xbee.getResponse().isError() ) 
	{
		SYSEVT_ERROR(F("XBee.loop - XBee reported error.  Error code: %d"), xbee.getResponse().getErrorCode());
		return;
	}

	// Note: XBee error status and current packet will be discarded on the new readPacket(), on the next loop() pass.
}

XBeeRFClass XBeeRF;

