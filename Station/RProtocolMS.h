/*
        Remote Station Protocol support for Sprinklers system.

  This module is intended to be used with my SmartGarden system, as well as my modified version of the sprinklers_avr code.


This module handles remote communication protocol. Initial version operates over Xbee link in API mode, but protocol itself is
generic and can operate over multiple transports.


Creative Commons Attribution-ShareAlike 3.0 license
Copyright 2014 tony-osp (http://tony-osp.dreamwidth.org/)

*/

#ifndef _RPROTOCOLMASTER_h
#define _RPROTOCOLMASTER_h

#if defined(ARDUINO) && ARDUINO >= 100
#include "Arduino.h"
#else
#include "WProgram.h"
#endif

#include <Time.h>
#include "port.h"
#include <inttypes.h>


#include "SGRProtocol.h"        // wire protocol definitions

typedef bool (*PTransportCallback)(uint8_t nStation, void *msg, uint8_t mSize);
typedef bool (*PARPCallback)(uint8_t nStation, uint8_t *pNetAddress);

class RProtocolMaster {

public:
                RProtocolMaster();
				bool    begin(void);
				void	loop(void);

				void	RegisterARP(void *ptr);

			// Remote stations commands

				bool	ChannelOn( uint8_t stationID, uint8_t chan, uint8_t ttr);
				bool	ChannelOff( uint8_t stationID, uint8_t chan);
				bool	AllChannelsOff(uint8_t stationID);
				bool	PollStationSensors(uint8_t stationID);
				bool	SubscribeEvents( uint8_t stationID );


				bool	SendReadZonesStatus( uint8_t stationID, uint16_t transactionID );
				bool	SendReadSystemRegisters( uint8_t stationID, uint8_t startRegister, uint8_t numRegisters, uint16_t transactionID );
				bool	SendReadSensors( uint8_t stationID, uint16_t transactionID );
				bool	SendForceSingleZone( uint8_t stationID, uint8_t channel, uint16_t ttr, uint16_t transactionID );
				bool	SendTurnOffAllZones( uint8_t stationID, uint16_t transactionID );
				bool	SendSetName( uint8_t stationID, const char *str, uint16_t transactionID );
				bool	SendRegisterEvtMaster( uint8_t stationID, uint8_t eventsMask, uint16_t transactionID);

				void	ProcessNewFrame(uint8_t *ptr, int len, uint8_t *pNetAddress);

				void	SendTimeBroadcast(void);

// Client routines
				bool SendZonesReport(uint8_t transactionID, uint8_t fromUnitID, uint8_t toUnitID, uint8_t firstZone, uint8_t numZones);
				bool SendSensorsReport(uint8_t transactionID, uint8_t fromUnitID, uint8_t toUnitID, uint8_t firstSensor, uint8_t numSensors);
				bool SendSystemRegisters(uint8_t transactionID, uint8_t fromUnitID, uint8_t toUnitID, uint8_t firstRegister, uint8_t numRegisters);
				bool SendEvtMasterReport(uint8_t transactionID, uint8_t fromUnitID, uint8_t toUnitID);
				bool SendPingReply(uint8_t transactionID, uint8_t fromUnitID, uint8_t toUnitID, uint32_t cookie);
				bool SendOKResponse(uint8_t transactionID, uint8_t fromUnitID, uint8_t toUnitID, uint8_t FCode);
				bool SendErrorResponse(uint8_t transactionID, uint8_t fromUnitID, uint8_t toUnitID, uint8_t fCode, uint8_t errorCode);

				bool NotifySysEvent(uint8_t eventType, uint32_t timeStamp, uint16_t seqID, uint8_t flags, uint8_t eventDataLength, uint8_t *eventData);

private:
// ARP address update
				PARPCallback		_ARPAddressUpdate;
};

// Modbus holding registers area size
//
// Currently we support up to 8 zones (can be changed if necessary)
#define         MODBUSMAP_SYSTEM_MAX   (MREGISTER_ZONE_COUNTDOWN + 8)

extern RProtocolMaster rprotocol;


#endif  //_RPROTOCOLMASTER_h
