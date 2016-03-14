// MoteinoRF.cpp
/*
        Moteino RF support for SmartGarden


Creative Commons Attribution-ShareAlike 3.0 license
Copyright 2015-2016 tony-osp (http://tony-osp.dreamwidth.org/)


March 2016 - adding packet duplication protection. 
Sometimes RF packet is delivered, but ACK is lost due to interference etc, 
and the sender node will send another copy of the same packet which will cause packet duplicaion on the receiving end.

The protection from packet duplication is based on adding packet sequence number. It is essentially a counter of packets, inserted as the first byte in the RF packet payload.
If the packet is successfully acknowledged, the counter is increased by 1, if the packet is retried - retry transmissions have the same sequence number, allowing receiving end to skip duplicates.
To avoid false positives and to allow reliable start of comms, sending station starts with the sequence number of 0 after reboot, and counts until 254, then resetting to 0.
Sequence number of 255 is prohibited.

Each station keeps two lists, MAX_STATIONS size each - one is the array of sequence numbers used for sending to stations on the list, 
another one is the list of last known sequence numbers received from stations on the list.

The above mechanism is intended for point-to-point transmissions, not broadcasts, because it relies on ACK to proceed to the next sequence number.


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

	// initialize packet sequence counters
	for( uint8_t i=0; i<MAX_STATIONS; i++ )
	{
		uNextSNumber[i] = 0;	
		uLastReceivedSNumber[i] = 255;	// reserved counter value, used to synchronize state
	}

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

	if( nStation == STATIONID_BROADCAST ) // broadcast
	{
		TRACE_VERBOSE(F("MoteinoRF - sending broadcast packet, len %u\n"), uint16_t(mSize));
		moteinoRF.send(RF69_BROADCAST_ADDR,msg, mSize);
	}
	else
	{
		uint8_t		buf[RF69_MAX_DATA_LEN];

		if( nStation < MAX_STATIONS )						// first byte of the packet is the sequence number
			buf[0] = MoteinoRF.uNextSNumber[nStation];		// we keep track of sequence numbers per station
		else
			buf[0] = 255;

		TRACE_VERBOSE(F("MoteinoRF - sending packet to station %u, SN:%u, len %u\n"), uint16_t(nStation), uint16_t(buf[0]), uint16_t(mSize));
		memcpy(buf+1, msg, mSize);	// because sequence number is added as the first byte in the transmission, need to allocate new buffer and copy data to it (TODO: fill fix it later)

		bool retFlag = moteinoRF.sendWithRetry(nStation, buf, mSize+1, 3, 200); // 3 retries, 200ms wait time
		if(retFlag)
		{
			// packet successfully sent and acknowledged, increase sequence counter for this destination
			if( MoteinoRF.uNextSNumber[nStation] == 254 )	MoteinoRF.uNextSNumber[nStation] = 0;
			else											MoteinoRF.uNextSNumber[nStation]++;
		}
		else
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

	if( moteinoRF.receiveDone() )
	{
		uint8_t		buf[RF69_MAX_DATA_LEN];
		uint8_t		buf_len = 0;
		uint8_t		senderID = moteinoRF.SENDERID;
		uint8_t		targetID = moteinoRF.TARGETID;

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

		LastReceivedRSSI = moteinoRF.RSSI;	// update global RSSI tracker
		if( buf_len > 5 )	
		{
			if( targetID == RF69_BROADCAST_ADDR )	// broadcast messages don't have sequence numbers
			{
				TRACE_VERBOSE(F("MoteinoRF - received broadcast packet from %d, len=%u\n"), int16_t(senderID), uint16_t(buf_len-1));
				rprotocol.ProcessNewFrame(buf, buf_len, 0 );	// process incoming packet.
			}
			else if( senderID < MAX_STATIONS )	// basic protection to ensure we will not have an overflow. We handle packets only from senders with acceptable addresses (within MAX_STATION)
			{
				if( buf[0] != MoteinoRF.uLastReceivedSNumber[senderID] ) // make sure this is not a duplicate packet
				{
					if( buf[0] != 255 ) // valid sequence numbers are from 0 to 254
						MoteinoRF.uLastReceivedSNumber[senderID] = buf[0];	// update last received serial number from that source

					TRACE_VERBOSE(F("MoteinoRF - received packet from %d, SN=%u, len=%u\n"), int16_t(senderID), uint16_t(buf[0]), uint16_t(buf_len-1));
					rprotocol.ProcessNewFrame(buf+1, buf_len-1, 0 );	// process incoming packet. Note: we strip out sequence number
				}
				else
				{
					TRACE_VERBOSE(F("MoteinoRF - received duplicate packet from %d, SN=%u\n"), int16_t(senderID), uint16_t(buf[0]));
				}
			}
		}
	}
}



MoteinoRFClass MoteinoRF;

