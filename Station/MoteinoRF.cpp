// MoteinoRF.cpp
/*
        Moteino RF support for SmartGarden


Creative Commons Attribution-ShareAlike 3.0 license
Copyright 2015 tony-osp (http://tony-osp.dreamwidth.org/)

*/

#include "MoteinoRF.h"
#include "settings.h"
#include "port.h"


RFM69 moteinoRF;


MoteinoRFClass::MoteinoRFClass()
{
	fMoteinoRFReady = false;
}

void MoteinoRFClass::begin()
{
	TRACE_INFO(F("Starting MoteinoRF\n"));

	if( !IsMoteinoRFEnabled() ){

		SYSEVT_ERROR(F("MoteinoRF is not enabled, exiting"));
		return;					// init the system only if Moteino RF is enabled.
	}

	moteinoRF.initialize(MOTEINORF_FREQUENCY,GetMoteinoRFAddr(),GetMoteinoRFPANID());
#ifdef IS_RFM69HW
	moteinoRF.setHighPower(); //required only for RFM69HW!
#endif
	moteinoRF.encrypt(MOTEINORF_ENCRYPTKEY);

	SetMoteinoRFFlags(GetMoteinoRFFlags() | NETWORK_FLAGS_ON);	// Mark MoteinoRF network as On
	fMoteinoRFReady = true;		// and set local readiness flag

	//rprotocol.RegisterARP((void *)&XBeeARPUpdate);			// register ARP callback with the remote protocol
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

	TRACE_INFO(F("MoteinoRF - sending packet to station %d, len %u\n"), nStation, (unsigned int)mSize);


	return true;
}


// Main MoteinoRF loop poller. loop() should be called frequently, to allow processing of incoming packets
//
// Loop() will process incoming packets, will interpret remote protocol and will call appropriate handlers.
//


void MoteinoRFClass::loop(void)
{

}



MoteinoRFClass MoteinoRF;

