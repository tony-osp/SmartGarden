// MoteinoRF.cpp
/*
        Moteino RF support for SmartGarden


Creative Commons Attribution-ShareAlike 3.0 license
Copyright 2015 tony-osp (http://tony-osp.dreamwidth.org/)

*/

#include "MoteinoRF.h"
#include "settings.h"
#include "RProtocolMS.h"

//#define TRACE_LEVEL			7		// trace everything for this module
#include "port.h"

extern int16_t		LastReceivedRSSI;

RFM69 moteinoRF;


MoteinoRFClass::MoteinoRFClass()
{
	fMoteinoRFReady = false;
}

void MoteinoRFClass::begin()
{
	TRACE_INFO(F("Starting MoteinoRF, Node Addr:%u, PAN ID:%u\n"), uint16_t(GetMoteinoRFAddr()), uint16_t(NETWORK_MOTEINORF_DEFAULT_PANID));

	if( !IsMoteinoRFEnabled() ){

		SYSEVT_ERROR(F("MoteinoRF is not enabled, exiting"));
		return;					// init the system only if Moteino RF is enabled.
	}

	//bool initFlag = moteinoRF.initialize(MOTEINORF_FREQUENCY,GetMoteinoRFAddr(),GetMoteinoRFPANID());
	bool initFlag = moteinoRF.initialize(MOTEINORF_FREQUENCY,GetMoteinoRFAddr(),NETWORK_MOTEINORF_DEFAULT_PANID, false); // use the lib in non-interrupt mode

	TRACE_VERBOSE(F("MoteinoRF init returned %d\n"), int16_t(initFlag));
	if( initFlag == false )
	{
		SYSEVT_ERROR(F("MoteinoRF - cannot initialize RF module.\n"));
		return;
	}

#ifdef IS_RFM69HW
	moteinoRF.setHighPower(); //required only for RFM69HW!
#endif
	//moteinoRF.promiscuous(true);

	//moteinoRF.encrypt(MOTEINORF_ENCRYPTKEY);

	SetMoteinoRFFlags(GetMoteinoRFFlags() | NETWORK_FLAGS_ON);	// Mark MoteinoRF network as On
	fMoteinoRFReady = true;		// and set local readiness flag

	//rprotocol.RegisterARP((void *)&MoteinoRFARPUpdate);			// register ARP callback with the remote protocol
															// rprotocol will use it to report station=address associations

	return;

failed_ex1:
	return;
}





// Main packet send routine. 
//
//
//
bool MoteinoRFSendPacket(uint8_t nStation, void *msg, uint8_t mSize)
{
	if( !MoteinoRF.fMoteinoRFReady )	// check that MoteinoRF is initialized and ready
		return false;

	TRACE_INFO(F("MoteinoRF - sending packet to station %d, len %u\n"), nStation, uint16_t(mSize));

	//moteinoRF.send(RF69_BROADCAST_ADDR,msg, mSize);
	//return true;


	if( nStation == STATIONID_BROADCAST ) // broadcast
	{
		moteinoRF.send(RF69_BROADCAST_ADDR,msg, mSize);
	}
	else
	{
		bool retFlag = moteinoRF.sendWithRetry(nStation, msg, mSize, 3, 200); // 3 retries, 200ms wait time
		if(!retFlag)
		{
			TRACE_INFO(F("MoteinoRF - sendWithRetry returned %d\n"), int16_t(retFlag));
		}
	}

	return true;
}


// Main MoteinoRF loop poller. loop() should be called frequently, to allow processing of incoming packets
//
// Loop() will process incoming packets, will interpret remote protocol and will call appropriate handlers.
//


void MoteinoRFClass::loop(void)
{
	//TRACE_INFO(F("MoteinoRF - loop\n"));

	moteinoRF.loop();	// we are using RFM69 in non-interrupt mode

	if( moteinoRF.receiveDone() )
	{
		uint8_t		buf[RF69_MAX_DATA_LEN];
		uint8_t		buf_len = 0;
		uint8_t		senderID = moteinoRF.SENDERID;

		if( moteinoRF.DATALEN > 5 )
		{
			memcpy(buf, (uint8_t *)(moteinoRF.DATA), moteinoRF.DATALEN);
			buf_len = moteinoRF.DATALEN;
		}

		if( moteinoRF.ACKRequested() )
		{
			moteinoRF.sendACK();
			TRACE_VERBOSE(F("MoteinoRF - ACK requested, sending it.\n"));
		}

		if( buf_len > 5 ){

			TRACE_VERBOSE(F("MoteinoRF - received packet from %d, len=%u\n"), int16_t(senderID), uint16_t(buf_len));
			rprotocol.ProcessNewFrame(buf, buf_len, 0 );	// process incoming packet.
		}

		LastReceivedRSSI = moteinoRF.RSSI;	// update global RSSI tracker
	}
}



MoteinoRFClass MoteinoRF;

