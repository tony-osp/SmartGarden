// tftp.cpp
// tftp server class.
// Author: Richard Zimmerman
// Copyright (c) 2013 Richard Zimmerman
//
// Tony-osp: fixed existing file overwrite bug when new file is smaller than the old one, also changed debug output to trace() and
//           converted SendERR to use PSTR to conserve RAM on Arduino.

#include "tftp.h"
#include <SdFat.h>
#include "port.h"

// TODO:  There's a big problem here.  The W5100 only has 4 sockets available.  In version 1 of this library I reused the
//  same socket for the client and the server, but I "moved" the socket to a new port.  So it sort of behaved just like a
//  normal TFTP client.  This had an unfortuate side effect that when you were transferring more than one file, the client
//  would send the next request to port 69 before the W5100 had a chance to tear down the old client port and start listening
//  on port 69, so it got missed.
//  So, I changed things so I don't change the port number.  I always send out of port 69.  This isn't really normal TFTP
//  behavior, but it seems to work.  However, there can be some serious bugs introduced here if another client tries to 
//  do a TFTP transer at the same time as the first.  

// timeout in ms
#define TIMEOUT 1000

tftp::tftp(void) : m_timeout(0), m_curBlock(0)
{
}

tftp::~tftp(void)
{
	m_udp.stop();
}

bool tftp::Init()
{
	m_timeout = 0;
	if (!m_udp.begin(69)) {
		trace(F("No Sockets Available!\n"));
		return false;
	}
	return true;
}

#define TFTP_PACKET_SIZE 516

bool tftp::Poll()
{
	char packetBuffer[TFTP_PACKET_SIZE]; //buffer to hold incoming and outgoing packets 
	if ( m_udp.parsePacket() ) {  
		int size = m_udp.read(packetBuffer,TFTP_PACKET_SIZE);  // read the packet into the buffer
		int opcode = (packetBuffer[0] << 8) + packetBuffer[1];
		switch (opcode) {
		case 0x01: // RRQ
			{
				trace(F("RRQ\n"));
				if (m_timeout)
				{
					trace(F("Busy\n"));
					return false;
				}
				m_remoteIP = m_udp.remoteIP();
				m_remotePort = m_udp.remotePort();
				const char * fname = packetBuffer+2;
				const char * mode = packetBuffer+strlen(fname)+3;
				Serial.println(fname);
				m_curBlock = 1;
				if (!m_theFile.open(fname, O_READ))
					SendERR(1, PSTR("File Not Found"));
				else
					SendBlock((byte*)packetBuffer);
				break;
			}
		case 0x02: // WRQ
			{
				trace(F("WRQ\n"));
				// Check to see we don't already have a client on the line
				if (m_timeout)
				{
					trace(F("Busy\n"));
					return false;
				}
				m_remoteIP = m_udp.remoteIP();
				m_remotePort = m_udp.remotePort();
				const char * fname = packetBuffer+2;
				const char * mode = packetBuffer+strlen(fname)+3;
				Serial.println(fname);
				m_curBlock = 0;
				if (!m_theFile.open(fname, O_WRITE | O_CREAT | O_TRUNC))
					SendERR(6, PSTR("File Open ERR"));
				else
					SendACK();
				break;
			}
		case 0x03: // DATA
			{
				//Serial.println("DATA");
				if (!m_timeout)
				{
					trace(F("Data w/o Init\n"));
					return 0;
				}
				const int blocknum = (packetBuffer[2] << 8) + packetBuffer[3];
				if (blocknum != m_curBlock+1)
				{
					SendERR(0, PSTR("Invalid Block"));
					return 0;
				}
				m_curBlock++;
				SendACK();
				m_theFile.write(packetBuffer+4, size-4);

				// Check if this is the last packet
				if ((size-4) < 512)
				{
					m_theFile.close();
					m_timeout = 0;
					//Init();
				}
				break;
			}
		case 0x04: // ACK
			{
				if (!m_timeout)
				{
					trace(F("ACK w/o Init\n"));
					return 0;
				}
				const int blocknum = (packetBuffer[2] << 8) + packetBuffer[3];
				if (blocknum != m_curBlock)
				{
					SendERR(0, PSTR("Invalid Block"));
					return 0;
				}
				m_curBlock++;
				SendBlock((byte*)packetBuffer);
				break;
			}
		case 0x05: // ERROR
			{
				trace(F("ERR\n"));
				break;
			}
		default:
			SendERR(4, PSTR("Invalid OP"));
		}
	} // if parsePacket
	else if (m_timeout && (millis() > (m_timeout + TIMEOUT)))
	{
		trace(F("Timeout\n"));
		m_theFile.close();
		m_timeout = 0;
	}
	return true;
}

// Send an ACK packet to the client
void tftp::SendACK()
{
	byte packetBuffer[4] = {0x00, 0x04, 0x00, 0x00};
	packetBuffer[2] = m_curBlock >> 8;
	packetBuffer[3] = m_curBlock & 0x00FF;
	m_udp.beginPacket(m_remoteIP, m_remotePort);
	m_udp.write(packetBuffer,4);
	m_udp.endPacket();
	m_timeout = millis();
}

// Send an Error packet back to the client
void tftp::SendERR(int code, const char * msg)
{
	byte packetBuffer[35] = {0x00, 0x05};
	packetBuffer[2] = code >> 8;
	packetBuffer[3] = code & 0x00FF;
	strncpy_P((char*)packetBuffer+4,msg, sizeof(packetBuffer) - 4);
	Serial.println(msg);
	m_udp.beginPacket(m_remoteIP, m_remotePort);
	m_udp.write(packetBuffer,5+strlen(msg));
	m_udp.endPacket();
	m_timeout = 0;
}

// Send a block of data back to the client.
//  NOTE:  we pass in our buffer here to preserve stack space.
void tftp::SendBlock(byte * packetBuffer)
{
	packetBuffer[0] = 0x00;
	packetBuffer[1] = 0x03;
	packetBuffer[2] = m_curBlock >> 8;
	packetBuffer[3] = m_curBlock & 0x00FF;
	const int size = m_theFile.read(packetBuffer+4, 512);
	m_udp.beginPacket(m_remoteIP, m_remotePort);
	m_udp.write(packetBuffer, size+4);
	m_udp.endPacket();
	m_timeout = millis();
	if (size < 512)
	{
		m_theFile.close();
		m_timeout = 0;
	}
}
