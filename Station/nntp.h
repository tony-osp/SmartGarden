// nntp.h
// NNTP time update class.
// Author: Richard Zimmerman
// Copyright (c) 2013 Richard Zimmerman
//

#pragma once
#include <Time.h>
//#include <DateTime.h>
//#include <DateTimeStrings.h>

class nntp
{
public:
	nntp(void);
	~nntp(void);
	void checkTime();
    uint8_t GetNetworkStatus();
	void SetLastUpdateTime(void);
	void flagCheckTime(void);

        
};

