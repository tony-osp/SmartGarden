/*
        Remote Station Protocol support for SmartGarden system.


This module handles remote communication protocol. Initial version operates over Xbee link in API mode, but protocol itself is
generic and can operate over multiple transports.


Creative Commons Attribution-ShareAlike 3.0 license
Copyright 2014 tony-osp (http://tony-osp.dreamwidth.org/)

*/


#include "RProtocolMaster.h"
#include "core.h"
#include "settings.h"
#include "sensors.h"


// Local forward declarations


RProtocolMaster::RProtocolMaster()
{
	_SendMessage = 0;
}

bool RProtocolMaster::begin(void)
{
}


void RProtocolMaster::RegisterTransport(void *ptr)
{
	_SendMessage = (PTransportCallback)ptr;
}

//
// To reduce RAM overhead we declare some of the response handling functions as inline, since these functions are used just once,
//  and separation of this code into a function is done just for clarity/manageability reasons.
//

inline void MessageZonesReport( void *ptr )
{
	RMESSAGE_ZONES_REPORT	*pMessage = (RMESSAGE_ZONES_REPORT *)ptr;

//	trace(F("MessageZonesReport - processing message\n"));
//	trace(F("Header.Length=%d, FirstZone=%d, NumZones=%d\n"), (int)(pMessage->Header.Length), (int)(pMessage->FirstZone), (int)(pMessage->NumZones) );

	// parameters length check
	if( pMessage->Header.Length != (sizeof(RMESSAGE_ZONES_REPORT)-sizeof(RMESSAGE_HEADER)) )
	{
		trace(F("MessageZonesReport - bad parameters length\n"));
		return;		// Note: In RProtocol station does not respond to malformed/invalid packets, just ignore it
	}
	
// Parameters validity check.

// Note: since currently we have no more than 8 zones in the Remote station, we hardcode response size to 1 byte
	if( ((pMessage->FirstZone+pMessage->NumZones) > GetNumZones()) || (pMessage->Header.FromUnitID > MAX_REMOTE_STATIONS) )
	{
		trace(F("MessageZonesReport - wrong payload data.\n"));
		return;	
	}
// OK, payload seems to be valid.

	runState.ReportStationZonesStatus(pMessage->Header.FromUnitID, pMessage->ZonesData[0]);
}


inline void MessageSensorsReport( void *ptr )
{
	RMESSAGE_SENSORS_REPORT	*pMessage = (RMESSAGE_SENSORS_REPORT *)ptr;

//	trace(F("MessageSensorsReport - processing message from station %d\n"), (int)(pMessage->Header.FromUnitID));
//	trace(F("Header.Length=%d, FirstSensor=%d, NumSensors=%d\n"), (int)(pMessage->Header.Length), (int)(pMessage->FirstSensor), (int)(pMessage->NumSensors) );

	// parameters length check
	if( pMessage->Header.Length != ((sizeof(RMESSAGE_SENSORS_REPORT)-sizeof(RMESSAGE_HEADER)) + (pMessage->NumSensors-1)*2) )
	{
		trace(F("MessageSensorsReport - bad parameters length\n"));
		return;		
	}
	
// Basic parameters validity check.
	if( pMessage->Header.FromUnitID > MAX_REMOTE_STATIONS )
	{
		trace(F("MessageSensorsReport - wrong payload data.\n"));
		return;	
	}
// OK, payload seems to be valid.

	uint8_t  nSensors = pMessage->NumSensors;
	uint8_t	 nFirstSensor = pMessage->FirstSensor;

	for( uint8_t i=0; i<nSensors; i++ )
	{
//		trace(F("MessageSensorsReport - reporting sensor reading, station=%d, channel=%d\n"), (int)(pMessage->Header.FromUnitID), (int)(nFirstSensor+i));		
		sensorsModule.ReportSensorReading(pMessage->Header.FromUnitID, nFirstSensor+i, (int) (pMessage->SensorsData[i]));
//		trace(F("station=%d after return from ReportSensorReading\n"), (int)(pMessage->Header.FromUnitID));
	}
}
 
inline void MessageSystemRegistersReport( void *ptr )
{

}

inline void MessagePingReply( void *ptr, uint16_t netAddress, uint8_t rssi )
{
	RMESSAGE_PING_REPLY	*pMessage = (RMESSAGE_PING_REPLY *)ptr;

	// parameters length check
	if( pMessage->Header.Length != (sizeof(RMESSAGE_PING_REPLY)-sizeof(RMESSAGE_HEADER)) )
	{
		trace(F("MessagePingReply - bad parameters length\n"));
		return;		
	}
	
	trace(F("MessagePingReply - Received from station: %d, netAddress=%u, RSSI=%u\n"), (int)(pMessage->Header.FromUnitID), netAddress, (unsigned int)(rssi) );
}

inline void MessageEvtMasterReport( void *ptr )
{
	RMESSAGE_EVTMASTER_REPORT	*pMessage = (RMESSAGE_EVTMASTER_REPORT *)ptr;

	// parameters length check
	if( pMessage->Header.Length != (sizeof(RMESSAGE_EVTMASTER_REPORT)-sizeof(RMESSAGE_HEADER)) )
	{
		trace(F("MessageEvtMasterReport - bad parameters length\n"));
		return;		
	}
	
//	trace(F("MessageEvtMasterReport - Received from station: %d, EvtMaster=%d, NetAddr=%d\n"), (int)(pMessage->Header.FromUnitID), (int)(pMessage->MasterStationID), (int)(pMessage->MasterStationAddress) );
}

inline void MessageResponseError( void *ptr )
{
	RMESSAGE_RESPONSE_ERROR	*pMessage = (RMESSAGE_RESPONSE_ERROR *)ptr;

	// parameters length check
	if( pMessage->Header.Length != (sizeof(RMESSAGE_RESPONSE_ERROR)-sizeof(RMESSAGE_HEADER)) )
	{
		trace(F("MessageResponseError - bad parameters length\n"));
		return;		
	}
	
	trace(F("MessageResponseError - Received from station: %d, FailedFCode=%d, ExceptionCode=%d\n"), (int)(pMessage->Header.FromUnitID), (int)(pMessage->FailedFCode), (int)(pMessage->ExceptionCode) );
}


//
//      RProtocol packets processing routines - FCODE_ZONES_READ
//
//      Input:	- stationID for the station to query.
//				- transactionID to use (can be 0)
//
//		Output	- true if successful, false on failure. 
//
//		This request will request read of all requested channels. Typically it will be full set of channels on the station
//
//      The routine will generate valid FCODE_ZONES_READ packet and send it.
//
//      FCODE_ZONES_READ packet expects to receive two parameters in the Data area:
//
//              1.      FirstZone to read
//              2.      The total number of zones to read
//
bool RProtocolMaster::SendReadZonesStatus( uint8_t stationID, uint16_t transactionID )
{
        RMESSAGE_ZONES_READ  Message;

		if( _SendMessage == 0 )
			return false;					// protection - transport callback is not registered

		ShortStation	sStation;
		LoadShortStation(stationID, &sStation);

        Message.Header.ProtocolID = RPROTOCOL_ID;
		Message.Header.FCode = FCODE_ZONES_READ;
        Message.Header.ToUnitID = stationID;
        Message.Header.FromUnitID = MY_STATION_ID;
        Message.Header.Length = sizeof(RMESSAGE_ZONES_READ)-sizeof(RMESSAGE_HEADER);
		Message.Header.TransactionID = transactionID;

        Message.FirstZone = 0;
		Message.NumZones = 0x0FF;	// read all station zone channels

		return _SendMessage(sStation.networkAddress, (void *)(&Message), sizeof(Message));
}


//
//      RProtocol packets processing routines - FCODE_SYSREGISTERS_READ
//
//      Input:	- stationID for the station to query.
//				- start register
//				- number of registers to read
//				- transactionID to use (can be 0)
//
//		Output	- true if successful, false on failure. 
//
//		This request will request read of a group of system registers.
//		In SmartGarden system these registers represent various system settings and state (e.g. PAN ID, number of channels etc)
//
//      The routine will generate valid FCODE_SYSREGISTERS_READ packet and send it.
//
//      FCODE_SYSREGISTERS_READ expects to have two parameters in the Data area:
//
//              1.      FirstRegister to read 
//              2.      The total number of registers to read
//
bool RProtocolMaster::SendReadSystemRegisters( uint8_t stationID, uint8_t startRegister, uint8_t numRegisters, uint16_t transactionID )
{
        RMESSAGE_SYSREGISTERS_READ  Message;

		if( _SendMessage == 0 )
			return false;					// protection - transport callback is not registered

		ShortStation	sStation;
		LoadShortStation(stationID, &sStation);

        Message.Header.ProtocolID = RPROTOCOL_ID;
		Message.Header.FCode = FCODE_SYSREGISTERS_READ;
        Message.Header.ToUnitID = stationID;
        Message.Header.FromUnitID = MY_STATION_ID;
        Message.Header.Length = sizeof(RMESSAGE_SYSREGISTERS_READ)-sizeof(RMESSAGE_HEADER);
		Message.Header.TransactionID = transactionID;

        Message.FirstRegister = startRegister;
		Message.NumRegisters = numRegisters;	

		return _SendMessage(sStation.networkAddress, (void *)(&Message), sizeof(Message));
}


//
//      RProtocol packets processing routines - FCODE_SENSORS_READ
//
//      Input:	- stationID 
//				- transactionID
//
//		Output	- true for success and false for failure.
//
//		This request will request read all sensors for a station.
//		In SmartGarden system these registers represent sensors - Temperature, Pressure, Humidity etc
//
//      The routine will generate valid FCODE_SENSORS_READ packet and send it.
//
//      FCODE_SENSORS_READ expects to receive two parameters in the Data area:
//
//              1.      FirstSensor to read 
//              2.      The total number of sensors to read 
//						for this parameter we use "magic" value of 0x0FF, which means "read all sensors"
//
bool RProtocolMaster::SendReadSensors( uint8_t stationID, uint16_t transactionID )
{
        RMESSAGE_SENSORS_READ  Message;

		if( _SendMessage == 0 )
			return false;					// protection - transport callback is not registered

		ShortStation	sStation;
		LoadShortStation(stationID, &sStation);

        Message.Header.ProtocolID = RPROTOCOL_ID;
		Message.Header.FCode = FCODE_SENSORS_READ;
        Message.Header.ToUnitID = stationID;
        Message.Header.FromUnitID = MY_STATION_ID;
        Message.Header.Length = sizeof(RMESSAGE_SENSORS_READ)-sizeof(RMESSAGE_HEADER);
		Message.Header.TransactionID = transactionID;

        Message.FirstSensor = 0;
		Message.NumSensors = 0x0FF;	

		return _SendMessage(sStation.networkAddress, (void *)(&Message), sizeof(Message));
}


//
//      RProtocol packets processing routines - start/stop zone
//
//      Input:	- stationID 
//				- channel/zone to operate
//				- time to run (min), or 0 for Off
//				- transactionID
//
//		Output	- true for success and false for failure.
//
//      The routine will generate and send request packet.
//
//
bool RProtocolMaster::SendForceSingleZone( uint8_t stationID, uint8_t channel, uint16_t ttr, uint16_t transactionID )
{
        RMESSAGE_ZONES_SET  Message;

		if( _SendMessage == 0 )
			return false;					// protection - transport callback is not registered

		ShortStation	sStation;
		LoadShortStation(stationID, &sStation);

		trace(F("SendForceSingleZone - sending request\n"));

        Message.Header.ProtocolID = RPROTOCOL_ID;
		Message.Header.FCode = FCODE_ZONES_SET;
        Message.Header.ToUnitID = stationID;
        Message.Header.FromUnitID = MY_STATION_ID;
        Message.Header.Length = sizeof(RMESSAGE_ZONES_SET)-sizeof(RMESSAGE_HEADER);
		Message.Header.TransactionID = transactionID;

        Message.FirstZone = channel;
		Message.Ttr = ttr;
		Message.NumZones = 1;
		Message.ScheduleID = 0;
		Message.ZonesData[0] = 1 << channel;
		Message.Flags = RMESSAGE_FLAGS_ACK_STD;

		return _SendMessage(sStation.networkAddress, (void *)(&Message), sizeof(Message));
}


//
//	RProtocol packets processing routines - turn off all zones on the station
//
//      Input:	- stationID 
//				- transactionID
//
//		Note: currently we are sending all 8 channels in one go
//
//		Output	- true for success and false for failure.
//
//      The routine will generate and send request packet.
//
bool RProtocolMaster::SendTurnOffAllZones( uint8_t stationID, uint16_t transactionID )
{
        RMESSAGE_ZONES_SET  Message;

		if( _SendMessage == 0 )
			return false;					// protection - transport callback is not registered

		ShortStation	sStation;
		LoadShortStation(stationID, &sStation);

        Message.Header.ProtocolID = RPROTOCOL_ID;
		Message.Header.FCode = FCODE_ZONES_SET;
        Message.Header.ToUnitID = stationID;
        Message.Header.FromUnitID = MY_STATION_ID;
        Message.Header.Length = sizeof(RMESSAGE_ZONES_SET)-sizeof(RMESSAGE_HEADER);
		Message.Header.TransactionID = transactionID;

        Message.FirstZone = 0;
		Message.Ttr = 0;
		Message.NumZones = 0x0FF;
		Message.ScheduleID = 0;
		Message.ZonesData[0] = 0;
		Message.Flags = RMESSAGE_FLAGS_ACK_STD;

		return _SendMessage(sStation.networkAddress, (void *)(&Message), sizeof(Message));
}



//
//      RProtocol packets processing routines - SETNAME
//
//      Input:	- stationID 
//				- new name (standard null-terminated string)
//				- transactionID
//
//		Output	- true for success and false for failure.
//
//      The routine will generate and send request packet.
//
//      SETNAME (extended FCode - set name for the station) expects to receive two parameters:
//
//      uint8_t NumDataBytes    -       number of data bytes of the name
//      Name                    -       new station name (string in ASCII).
//                              Note: the string is not terminated since its length is provided in NumDataBytes
//                              Note2: the string can be empty
//
bool RProtocolMaster::SendSetName( uint8_t stationID, const char *str, uint16_t transactionID )
{
#ifdef notdef
		RMESSAGE_SETNAME  *pMessage;
		uint8_t			  buffer[sizeof(RMESSAGE_SETNAME)+MAX_STATTION_NAME_LENGTH];
		uint8_t			  nameLen;

		if( _SendMessage == 0 )
			return false;					// protection - transport callback is not registered

		nameLen = strlen(str);
		if( nameLen > MAX_STATTION_NAME_LENGTH )	// proposed name is too long 
			return false;

// this is variable length packet (due to variable name length, allocate memory dynamically

		pMessage = (RMESSAGE_SETNAME *)buffer;

		ShortStation	sStation;
		LoadShortStation(stationID, &sStation);

        pMessage->Header.ProtocolID = 0;					// ProtocolID is supposed to be 0
        pMessage->Header.UnitID = stationID;
        pMessage->Header.Length = nameLen + 3;						// data block of 1 byte, name, plus FCode and UnitID
		pMessage->Header.TransactionID = transactionID;

		pMessage->NumDataBytes = nameLen;
		memcpy(&(pMessage->Name), str, nameLen);

		return _SendMessage(sStation.networkAddress, (void *)pMessage, sizeof(RMESSAGE_SETNAME) + nameLen - 1);
#endif //notdef

		return false;
}

//
//      RProtocol packets processing routines - EVENTMASTER
//
//      Input:	- stationID 
//				- events level to report
//				- events mask (types of events to report)
//				- extended flags (zero for now)
//				- transactionID
//
//		Output	- true for success and false for failure.
//
//      The routine will generate and send request packet.
//
//      EVENTMASTER (extended FCode - set events master) expects to receive three parameters:
//
//      uint8_t          StationID;
//      uint16_t         EventsMask;                     // Bitmask of the events to report
//      uint16_t         TransactionID;
//
//
bool RProtocolMaster::SendRegisterEvtMaster( uint8_t stationID, uint8_t eventsMask, uint16_t transactionID)
{
        RMESSAGE_EVTMASTER_SET	Message;

		if( _SendMessage == 0 )
			return false;					// protection - transport callback is not registered

		ShortStation	sStation;
		LoadShortStation(stationID, &sStation);

//		trace(F("SendRegisterEventsMaster - sending request\n"));

        Message.Header.ProtocolID = RPROTOCOL_ID;
		Message.Header.FCode = FCODE_EVTMASTER_SET;
        Message.Header.ToUnitID = stationID;
        Message.Header.FromUnitID = MY_STATION_ID;
        Message.Header.Length = sizeof(RMESSAGE_EVTMASTER_SET)-sizeof(RMESSAGE_HEADER);
		Message.Header.TransactionID = transactionID;

        Message.EvtFlags = EVTMASTER_FLAGS_REGISTER_SELF | eventsMask;
		Message.MasterStationAddress = Message.MasterStationID = 0;
		Message.Flags = RMESSAGE_FLAGS_ACK_STD;

		return _SendMessage(sStation.networkAddress, (void *)(&Message), sizeof(Message));
}

//
//      RProtocol packets processing routines - FCODE_TIME_BROADCAST
//
//      Input:	- none
//
//		Output	- none
//
//      The routine will broadcast current time on PAN
//
void RProtocolMaster::SendTimeBroadcast(void)
{
	RMESSAGE_TIME_BROADCAST Message;

//	trace(F("Sending time broadcast\n"));

		if( _SendMessage == 0 )
			return;					// protection - transport callback is not registered

        Message.Header.ProtocolID = RPROTOCOL_ID;
		Message.Header.FCode = FCODE_TIME_BROADCAST;
        Message.Header.ToUnitID = 255;	// broadcast
        Message.Header.FromUnitID = MY_STATION_ID;
        Message.Header.Length = sizeof(RMESSAGE_TIME_BROADCAST)-sizeof(RMESSAGE_HEADER);
		Message.Header.TransactionID = 0;

		Message.timeNow = nntpTimeServer.LocalNow();

		_SendMessage(NETWORK_ADDRESS_BROADCAST, (void *)(&Message), sizeof(Message));
}


// Process new data frame coming from the network
// ptr			- pointer to the data block,
// len			- block length
// netAddress	- sender's RF network address
// rssi			- recieving singlal strength indicator
//
void RProtocolMaster::ProcessNewFrame(uint8_t *ptr, int len, uint16_t netAddress, uint8_t rssi)
{
        register RMESSAGE_GENERIC *pMessage = (RMESSAGE_GENERIC *)ptr;    // for convenience of interpreting the packet

// first of all let's validate MBAP header and length constrains

        if( len < sizeof(RMESSAGE_GENERIC) )   // minimum valid packet length 
        {
                trace(F("ProcessNewFrame - Bad packet received, too short\n"));
				return;
        }

        if( pMessage->Header.ProtocolID != RPROTOCOL_ID )
        {
                trace(F("ProcessNewFrame - Bad packet received, bad ProtocolID \n"));
                return;
        }

		if( len != (pMessage->Header.Length+sizeof(RMESSAGE_HEADER)) )		// overall packet length should equal header size + length field
        {
                trace(F("ProcessNewFrame - Bad packet received, length does not match\n"));
                return;
        }

// OK, the packet seems to be valid, let's parse and dispatch it.

//		trace(F("ProcessNewFrame - processing packet, FCode: %d\n"), pMessage->Header.FCode);

        switch( pMessage->Header.FCode )
        {
// Standard FCodes
                case FCODE_ZONES_REPORT:
								MessageZonesReport( ptr );
                                break;

                case FCODE_SENSORS_REPORT:
                                MessageSensorsReport( ptr );
                                break;

                case FCODE_SYSREGISTERS_REPORT:
                                MessageSystemRegistersReport( ptr );
                                break;

                case FCODE_PING_REPLY:
                                MessagePingReply( ptr, netAddress, rssi );
                                break;

                case FCODE_RESPONSE_ERROR:
                                MessageResponseError( ptr );
                                break;

                case FCODE_SCAN_REPLY:
//                                StationsScanResponse( ptr );
                                break;

				case FCODE_EVTMASTER_REPORT:
								MessageEvtMasterReport( ptr );
								break;

        }

        return;
}




RProtocolMaster rprotocol;
