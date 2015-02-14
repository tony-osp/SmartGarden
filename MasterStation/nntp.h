// nntp.h
// NNTP time update class.
// Author: Richard Zimmerman
// Copyright (c) 2013 Richard Zimmerman
//

#pragma once
#include <Time.h>
#include <DateTime.h>
#include <DateTimeStrings.h>

class nntp
{
public:
	nntp(void);
	~nntp(void);
	time_t LocalNow();
	void checkTime();
        uint8_t GetNetworkStatus();
        
private:
	time_t m_nextSyncTime;
};

