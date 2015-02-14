// tftp.h
// tftp server class.
// Author: Richard Zimmerman
// Copyright (c) 2013 Richard Zimmerman
//

#pragma once
#include <Ethernet.h>
#include <SdFat.h>

class tftp
{
public:
	tftp(void);
	~tftp(void);
	bool Init();
	bool Poll();

private:
	void SendACK();
	void SendERR(int code, const char * msg);
	void SendBlock(byte * packetBuffer);

private:
	EthernetUDP m_udp;
	IPAddress m_remoteIP;
	int m_remotePort;
	int m_curBlock;
	unsigned long m_timeout;
	SdFile m_theFile;
	enum {MODE_BINARY, MODE_ASCII} m_xfer_mode;
};

