// nntp.cpp
// NNTP time update class.
// Author: Richard Zimmerman
// Copyright (c) 2013 Richard Zimmerman
//

#include "nntp.h"
#include "settings.h"
#include <Arduino.h>
#include <EthernetUdp.h>

static uint8_t cNntpSync = 0;

nntp::nntp(void) : m_nextSyncTime(0)
{
}


nntp::~nntp(void)
{
}

byte nntp::GetNetworkStatus(void)
{
	if( cNntpSync > 0 )return true;
	else               return false;
}

#ifdef HW_ENABLE_ETHERNET
// NTP time stamp is in the first 48 bytes of the message
#define NTP_PACKET_SIZE 48


static unsigned long sendNTPpacket(EthernetUDP & Udp, const IPAddress& address, byte * packetBuffer, uint16_t port = 123)
{
    memset(packetBuffer, 0, NTP_PACKET_SIZE); 
    // Initialize values needed to form NTP request
    // (see URL above for details on the packets)
    packetBuffer[0] = 0b11100011;   // LI, Version, Mode
    packetBuffer[1] = 0;     // Stratum, or type of clock
    packetBuffer[2] = 6;     // Polling Interval
    packetBuffer[3] = 0xEC;  // Peer Clock Precision
    // 8 bytes of zero for Root Delay & Root Dispersion
    packetBuffer[12]  = 49; 
    packetBuffer[13]  = 0x4E;
    packetBuffer[14]  = 49;
    packetBuffer[15]  = 52;

    // all NTP fields have been given values, now
    // you can send a packet requesting a timestamp: 	
    Udp.beginPacket(address, port); //NTP requests are to port 123
    Udp.write(packetBuffer,NTP_PACKET_SIZE);
    Udp.endPacket(); 
	return 0;
}

static unsigned long getNtpTime()
{
	trace(F("Syncing Time\n"));
	EthernetUDP Udp;
    if (!Udp.begin(8888))
	{
        trace(F("No Sockets Available!\n"));
		return 0;
	}
	byte packetBuffer[NTP_PACKET_SIZE]; //buffer to hold incoming and outgoing packets 
	// set all bytes in the buffer to 0
	sendNTPpacket(Udp, GetNTPIP(), packetBuffer); // send an NTP packet to a time server

	// wait to see if a reply is available
	int timeout = 20;  // try for 2 seconds
	while (true) {
		if ( Udp.parsePacket() ) {  
			//ShowSockStatus();
			// We've received a packet, read the data from it
			Udp.read(packetBuffer,NTP_PACKET_SIZE);  // read the packet into the buffer

			//the timestamp starts at byte 40 of the received packet and is four bytes,
			// or two words, long. First, esxtract the two words:

			unsigned long highWord = word(packetBuffer[40], packetBuffer[41]);
			unsigned long lowWord = word(packetBuffer[42], packetBuffer[43]);  
			// combine the four bytes (two words) into a long integer
			// this is NTP time (seconds since Jan 1 1900):
			unsigned long secsSince1900 = highWord << 16 | lowWord;  
			// now convert NTP time into everyday time:
			// Unix time starts on Jan 1 1970. In seconds, that's 2208988800:
			const unsigned long seventyYears = 2208988800UL;     
			// subtract seventy years:
			unsigned long epoch = secsSince1900 - seventyYears;  
			// print Unix time:
			trace(F("Unix time = %lu\n"), epoch);
			Udp.stop();
                        cNntpSync = 5;		// Success. Set counter.
											// Note: we allow up to 5 NNTP errors before flagging it as a lost connectivity

			return epoch;                        
		}
		if (!(timeout--) > 0)
		{
			trace(F("NTP Fail\n"));
			//  There's a bug with the W5100 that causes an ARP failure if we reuse a socket for UDP after
			//  using it for TCP.  To work around this bug, if we fail we'll try connecting to a different IP address.
			//  This should clear the ARP cache and we should be golden!.
			sendNTPpacket(Udp, GetGateway(), packetBuffer, 9990);  // 9990 is a random port.
			Udp.stop();

                        if( cNntpSync > 0 )		// NNTP sync failure. Decrease the counter.
							cNntpSync--;

			return 0;
		}
		delay(100);
	}
}

time_t nntp::LocalNow()
{
	return now() + GetNTPOffset()*3600;
}


void nntp::checkTime()
{
	if(m_nextSyncTime <= now()){
		time_t t = getNtpTime();
		if( t != 0)
		{
			t += GetNTPOffset()*3600;
			setTime(t);
		}
		m_nextSyncTime = now() + 300; //  300 = every 5 minutes
	}  
}

void nntp::flagCheckTime(void)
{
	m_nextSyncTime = now();  // sync time on the next opportunity

}

#endif //HW_ENABLE_ETHERNET
