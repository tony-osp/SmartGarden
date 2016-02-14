// MoteinoRF.h
/*
        Moteino RF support for SmartGarden


Creative Commons Attribution-ShareAlike 3.0 license
Copyright 2015 tony-osp (http://tony-osp.dreamwidth.org/)

*/

#ifndef _MOTEINORF_h
#define _MOTEINORF_h

#if defined(ARDUINO) && ARDUINO >= 100
	#include "Arduino.h"
#else
	#include "WProgram.h"
#endif

#include "Defines.h"
#include "RFM69.h"

//#define FREQUENCY     RF69_433MHZ
//#define FREQUENCY     RF69_868MHZ
#define MOTEINORF_FREQUENCY       RF69_915MHZ //Match this with the version of your Moteino! (others: RF69_433MHZ, RF69_868MHZ)
//#define ENCRYPTKEY      "sampleEncryptKey" //has to be same 16 characters/bytes on all nodes, not more not less!
#define IS_RFM69HW    //uncomment only for RFM69HW! Leave out if you have RFM69W!

// ***TEMPORARY***
// RFM69 encryption key is statically defined here (16 characters)
#define MOTEINORF_ENCRYPTKEY	"SmartGarden v1.x"

class MoteinoRFClass
{
 public:
	MoteinoRFClass();
	void begin(void);
	void loop(void);

	bool	fMoteinoRFReady;			// Flag indicating that XBee is initialized and ready
private:

};

extern MoteinoRFClass MoteinoRF;
bool MoteinoRFSendPacket(uint8_t nStation, void *msg, uint8_t mSize);

#endif

