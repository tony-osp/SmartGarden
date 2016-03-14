/*
        Remote Station Protocol support for SmartGarden system.


This module handles remote communication protocol. Initial version operates over Xbee link in API mode, but protocol itself is
generic and can operate over multiple transports.


Creative Commons Attribution-ShareAlike 3.0 license
Copyright 2014-2016 tony-osp (http://tony-osp.dreamwidth.org/)

*/


#include "RProtocolMS.h"
#include "core.h"
#include "settings.h"
#include "sensors.h"
#include "XBeeRF.h"
#include "MoteinoRF.h"

//#define TRACE_LEVEL			7		// trace everything for this module
#include "port.h"


uint8_t		LastReceivedStationID;
uint32_t	LastReceivedTime;
int16_t		LastReceivedRSSI;


// Local forward declarations
inline uint16_t		getSingleSensor(uint8_t regAddr);

// 
// Generic network "send packet" routine, used by all protocol messages.
//
// If new types of transport are added, appropriate handler needs to be added to this routine.
//
inline bool SendNetworkPacket(uint8_t stationID, void *pMessage, uint8_t mSize )
{
#ifdef HW_ENABLE_XBEE
		XBeeSendPacket(stationID, pMessage, mSize);
#endif //HW_ENABLE_XBEE

#ifdef HW_ENABLE_MOTEINORF
		MoteinoRFSendPacket(stationID, pMessage, mSize);
#endif //HW_ENABLE_MOTEINORF

		return true;
}


RProtocolMaster::RProtocolMaster()
{
	_ARPAddressUpdate = 0;
}

bool RProtocolMaster::begin(void)
{
	return true;
}


void RProtocolMaster::RegisterARP(void *ptr)
{
	TRACE_INFO(F("RProtocol - registering ARP callback\n"));
	_ARPAddressUpdate = (PARPCallback)ptr;
}

// Local helper routines
//
// Helper funciton - read single holding register. 
// This routine is used to get actual system data (represented in holding registers).
//
//  Input - RProtocol address of the required register (addresses start from 0).
//
inline uint16_t	getSingleSystemRegister(uint8_t regAddr)
{
	switch( regAddr )
	{
		case MREGISTER_STATION_TYPE:
									return  DEFAULT_STATION_TYPE;
	
		case MREGISTER_HARDWARE_VERSION:
									return  SG_HARDWARE;

		case MREGISTER_FIRMWARE_VERSION:
									return  SG_FIRMWARE_VERSION;
	
		case MREGISTER_SLAVE_ID:
									return  GetMyStationID();
	
		case MREGISTER_PAN_ID:
									return  GetXBeePANID();
	
		case MREGISTER_STATION_STATUS:
									return   RPROTOCOL_STATUS_ENABLED;			
	
		case MREGISTER_TIMEDATE_LOW:
									return   now() & 0xFFFF;
	
		case MREGISTER_TIMEDATE_HIGH:
									return   (now()>>16) & 0xFFFF;
	
		case MREGISTER_MAX_DURATION:
									return   DEFAULT_MAX_DURATION;
	
	default:
			if( (regAddr >= MREGISTER_ZONE_COUNTDOWN) && (regAddr < (MREGISTER_ZONE_COUNTDOWN+ GetNumZones())) )
			{
			// this is request to read specific zone countdown counter
			
					return 0;		// stub for now
			}
			else	// default response
			{
					return 0;
			}
	}
}

//
// Helper funciton - read single input register. 
// This routine is used to get actual sensors (represented by input registers).
//
//  Input - RProtocol address of the required register (addresses start from 0).
//
inline uint16_t		getSingleSensor(uint8_t sensorID)
{
	if( sensorID >= GetNumSensors() ) 
	{
		SYSEVT_ERROR(F("getSingleSensorRegister - error, wrong sensorID %d"), sensorID);
		return 0;
	}

	return uint16_t(sensorsModule.SensorsList[sensorID].lastReading & 0xFFFF);
}

//
//	packets processing routines - Read Zones Status
//
//	Input - pointer to the input packet (packet includes header)
// 			It is assumed that basic input packet structure is already validated
//
//			Second parameter is the sender network address, it will be used to send response.
//	
//	The routine will process input, execute required action(s), and will generate output packet
//	using common helpers
//
//	read zones status expects to receive two parameters in the Data area:
//
//		1.	First zone# to read
//		2.	The total number of zones to read
//
inline void MessageZonesRead( void *ptr )
{
	register RMESSAGE_ZONES_READ	*pMessage = (RMESSAGE_ZONES_READ *)ptr;

	// parameters length check
	if( pMessage->Header.Length != (sizeof(RMESSAGE_ZONES_READ)-sizeof(RMESSAGE_HEADER)) )
	{
		SYSEVT_ERROR(F("MessageZonesRead - bad parameters length"));
		return;		// Note: In RProtocol station does not respond to malformed/invalid packets, just ignore it
	}
	
	if( pMessage->Header.ToUnitID >= MAX_STATIONS )
	{
			SYSEVT_ERROR(F("MessageZonesRead - wrong station ID"));
			rprotocol.SendErrorResponse(pMessage->Header.TransactionID, pMessage->Header.ToUnitID, pMessage->Header.FromUnitID, FCODE_ZONES_READ, 2);
			return;																					// unit ID, FCode=1 and Exception Code=2 (Illegal Data Address)
	}
	
	{
		ShortStation	sStation;
		LoadShortStation(pMessage->Header.ToUnitID, &sStation);
		if( !(sStation.stationFlags & STATION_FLAGS_VALID) || !(sStation.stationFlags & STATION_FLAGS_RSTATUS) )
		{
			SYSEVT_ERROR(F("MessageZonesRead - station is not valid or not enabled for remote query"));
			rprotocol.SendErrorResponse(pMessage->Header.TransactionID, pMessage->Header.ToUnitID, pMessage->Header.FromUnitID, FCODE_ZONES_READ, 2);
			return;																					// unit ID, FCode=1 and Exception Code=2 (Illegal Data Address)
		}
		if( (pMessage->FirstZone == 0) && (pMessage->NumZones == 0x0FF) )		// check for the special "magic" number of 0x0FF, which means "read all zones"
			pMessage->NumZones =  sStation.numZoneChannels;

		if( ((pMessage->FirstZone+pMessage->NumZones) >  sStation.numZoneChannels) || (pMessage->NumZones < 1)  )
		{
			SYSEVT_ERROR(F("MessageZonesRead - wrong data address or the number of zones to read"));
	
			rprotocol.SendErrorResponse(pMessage->Header.TransactionID, pMessage->Header.ToUnitID, pMessage->Header.FromUnitID, FCODE_ZONES_READ, 2);		// send error response with the same transaction ID, 
			return;																					// unit ID, FCode=1 and Exception Code=2 (Illegal Data Address)
		}
	}

// OK, parameters are valid. Get the data and send response

	rprotocol.SendZonesReport(pMessage->Header.TransactionID, pMessage->Header.ToUnitID, pMessage->Header.FromUnitID, pMessage->FirstZone, pMessage->NumZones);
	return;
}

//
//	RProtocol packets processing routines - FCODE_ZONES_SET
//
//	Input - pointer to the input packet 
// 			It is assumed that basic input packet structure is already validated
//	
//	The routine will process input, execute required action(s), and will generate output packet
//	using common helpers
//
//	Expects to receive  parameters in the message:
//
//		1.	First zone 
//		2.	New coil state to write (2 bytes). 0x00 means Off, other value means - minutes to run
//
inline void MessageZonesSet( void *ptr )
{
	register RMESSAGE_ZONES_SET	*pMessage = (RMESSAGE_ZONES_SET *)ptr;

	// parameters length check
	if( pMessage->Header.Length != (sizeof(RMESSAGE_ZONES_SET) - sizeof(RMESSAGE_HEADER)) )		// we assume that this station has no more than 8 zones, so zones flags will be in one byte
	{
		SYSEVT_ERROR(F("MessageZonesSet - bad parameters length"));
		return;		// Note: In RProtocol station does not respond to malformed/invalid packets, just ignore it
	}
	
// Parameters validity check. 

	if( pMessage->Header.ToUnitID >= MAX_STATIONS )  //Check bounds
	{
		SYSEVT_ERROR(F("MessageZonesSet - requested station number is outside of bounds"));
	
		rprotocol.SendErrorResponse(pMessage->Header.TransactionID, pMessage->Header.ToUnitID, pMessage->Header.FromUnitID, pMessage->Header.FCode, 2);	// send error response with the same transaction ID, 
		return;																						// unit ID, Exception Code=2 (Illegal Data Address)
	}

	{
		ShortStation	sStation;

		LoadShortStation(pMessage->Header.ToUnitID, &sStation);

		if( !(sStation.stationFlags & STATION_FLAGS_ENABLED) || !(sStation.stationFlags & STATION_FLAGS_VALID) || !(sStation.stationFlags & STATION_FLAGS_RCONTROL) )
		{
			SYSEVT_ERROR(F("MessageZonesSet - station %d is not enabled, flags=%d"), uint16_t(pMessage->Header.ToUnitID), uint16_t(sStation.stationFlags) );
	
			rprotocol.SendErrorResponse(pMessage->Header.TransactionID, pMessage->Header.ToUnitID, pMessage->Header.FromUnitID, pMessage->Header.FCode, 2);	// send error response with the same transaction ID, 
			return;																						// unit ID, Exception Code=2 (Illegal Data Address)
		}

		if(	((pMessage->NumZones+pMessage->FirstZone) >  sStation.numZoneChannels) || (pMessage->NumZones < 1) )
		{
			SYSEVT_ERROR(F("MessageZonesSet - wrong parameters. NumZones=%d, FirstZone=%d, numZoneChannels=%d"), uint16_t(pMessage->NumZones), uint16_t(pMessage->FirstZone), uint16_t(sStation.numZoneChannels) );
	
			rprotocol.SendErrorResponse(pMessage->Header.TransactionID, pMessage->Header.ToUnitID, pMessage->Header.FromUnitID, pMessage->Header.FCode, 2);	// send error response with the same transaction ID, 
			return;																						// unit ID, Exception Code=2 (Illegal Data Address)
		}
	}

// OK, parameters are valid. Execute action and send response

	if( pMessage->Ttr == 0 ) 
	{
		runState.RemoteStopAllZones();	
	}
	else
	{
		for( uint8_t i=pMessage->FirstZone; i<(pMessage->FirstZone+pMessage->NumZones); i++ )
		{
			if( pMessage->ZonesData[0] & (1 << i) )
				runState.RemoteStartZone(pMessage->ScheduleID, pMessage->Header.ToUnitID, i, pMessage->Ttr); 
		}
	}

	// All done, check the type of response requested and send response.

	if( pMessage->Flags & RMESSAGE_FLAGS_ACK_BRIEF ) 
		rprotocol.SendOKResponse(pMessage->Header.TransactionID, pMessage->Header.ToUnitID, pMessage->Header.FromUnitID, pMessage->Header.FCode);

	if( pMessage->Flags & RMESSAGE_FLAGS_ACK_REPORT ) 
		rprotocol.SendZonesReport(pMessage->Header.TransactionID, pMessage->Header.ToUnitID, pMessage->Header.FromUnitID, pMessage->FirstZone, pMessage->NumZones);
	
	return;
}


// Helper routine - send Zones report

bool RProtocolMaster::SendZonesReport(uint8_t transactionID, uint8_t fromUnitID, uint8_t toUnitID, uint8_t firstZone, uint8_t numZones)
{		
		if( fromUnitID >= MAX_STATIONS )
		{
			SYSEVT_ERROR(F("SendZonesReport - wrong station number"));
			return false;																										// unit ID, Exception Code=2 (Illegal Data Address)
		}
	
		//TRACE_VERBOSE(F("SendZonesReport - entering\n"));

		RMESSAGE_ZONES_REPORT ReportMessage;

		ReportMessage.Header.ProtocolID = RPROTOCOL_ID;
		ReportMessage.Header.FCode = FCODE_ZONES_REPORT;
		ReportMessage.Header.FromUnitID = fromUnitID;
		ReportMessage.Header.ToUnitID = toUnitID;
		ReportMessage.Header.TransactionID = transactionID;
		ReportMessage.Header.Length = 4;	// we assume that we have no more than 8 zones, hence status data is 1 byte
// Note: since currently we have no more than 8 zones in the Remote station, we hardcode response size

		uint8_t			zonesStatus = 0;
		uint8_t			stationFlags = 0;
		
		{ // limit scope of ShortStation to reduce memory footprint when calling transport handlers
			ShortStation	sStation;

			LoadShortStation(fromUnitID, &sStation);	// load station information

			stationFlags = sStation.stationFlags;
			if( stationFlags & STATION_FLAGS_ENABLED )
			{
				if( (firstZone+numZones) > sStation.numZoneChannels )
				{
					SYSEVT_ERROR(F("SendZonesReport - wrong zones input parameters. firstZone=%d, numZones=%d, stationID=%d, numZoneChannels=%d"), uint16_t(firstZone), uint16_t(numZones),uint16_t(fromUnitID),uint16_t(sStation.numZoneChannels) );
					return false;																										// unit ID, Exception Code=2 (Illegal Data Address)
				}

				for( uint8_t pos=firstZone; pos<(firstZone+numZones); pos++ )
				{
					register uint8_t  zst = GetZoneState(1+pos+sStation.startZone);	// note zone number correction - zones are numbered from 1.

					if( (zst == ZONE_STATE_RUNNING) || (zst == ZONE_STATE_STARTING) )
					{
						zonesStatus |= 1 << pos;
					}
				}
			}
			else
			{
				TRACE_VERBOSE(F("SendZonesReport - station is not enabled\n"));
			}
		}

		ReportMessage.StationFlags = ZONES_REPFLAG_STATION_ENABLED;
		ReportMessage.FirstZone = firstZone;
		ReportMessage.NumZones = numZones;
		ReportMessage.ZonesData[0] = zonesStatus;

		return SendNetworkPacket(toUnitID, (void *)(&ReportMessage), sizeof(ReportMessage) );	// send response with requested zones status bits
}


// Helper routine - send Sensors report

bool RProtocolMaster::SendSensorsReport(uint8_t transactionID, uint8_t fromUnitID, uint8_t toUnitID, uint8_t firstSensor, uint8_t numSensors)
{
	if( ((firstSensor+numSensors) > GetNumSensors()) || (numSensors < 1) )
	{
		SYSEVT_ERROR(F("SendSensorsReport - wrong input parameters, firstSensor=%d, numSensors=%d"), uint16_t(firstSensor), uint16_t(numSensors));
		return false;																										// unit ID, Exception Code=2 (Illegal Data Address)
	}
	
//	SYSEVT_ERROR(F("Sending sensors data to station %d"), uint16_t(toUnitID));

	uint8_t		outbuf[sizeof(RMESSAGE_SENSORS_REPORT)+(MAX_SENSORS-1)*2];
	RMESSAGE_SENSORS_REPORT *pReportMessage = (RMESSAGE_SENSORS_REPORT*) outbuf;

	pReportMessage->Header.ProtocolID = RPROTOCOL_ID;
	pReportMessage->Header.FCode = FCODE_SENSORS_REPORT;
	pReportMessage->Header.FromUnitID = fromUnitID;
	pReportMessage->Header.ToUnitID = toUnitID;
	pReportMessage->Header.TransactionID = transactionID;
	pReportMessage->Header.Length = 2+numSensors*2;	

	for( uint8_t i=0; i<numSensors; i++ )
	{
		pReportMessage->SensorsData[i] = getSingleSensor(i+firstSensor);
	}

	pReportMessage->FirstSensor = firstSensor;
	pReportMessage->NumSensors = numSensors;

	return SendNetworkPacket(toUnitID, (void *)(outbuf), sizeof(RMESSAGE_SENSORS_REPORT)+(numSensors-1)*2 );

}


// Helper routine - send Sensors report

bool RProtocolMaster::SendSystemRegisters(uint8_t transactionID, uint8_t fromUnitID, uint8_t toUnitID, uint8_t firstRegister, uint8_t numRegisters)
{
	if( (firstRegister+numRegisters) > MODBUSMAP_SYSTEM_MAX )
	{
		SYSEVT_ERROR(F("SendSystemRegisters - wrong input parameters"));
		return false;																										// unit ID, Exception Code=2 (Illegal Data Address)
	}
	uint8_t		outbuf[sizeof(RMESSAGE_SYSREGISTERS_REPORT)+MODBUSMAP_SYSTEM_MAX*2];
	RMESSAGE_SYSREGISTERS_REPORT *pReportMessage = (RMESSAGE_SYSREGISTERS_REPORT*) outbuf;

	pReportMessage->Header.ProtocolID = RPROTOCOL_ID;
	pReportMessage->Header.FCode = FCODE_SYSREGISTERS_REPORT;
	pReportMessage->Header.FromUnitID = fromUnitID;
	pReportMessage->Header.ToUnitID = toUnitID;
	pReportMessage->Header.TransactionID = transactionID;
	pReportMessage->Header.Length = 2+numRegisters;	

	
	for( uint8_t i=0; i<numRegisters; i++ )
	{
		pReportMessage->RegistersData[i] = getSingleSystemRegister(i+firstRegister);
	}

	pReportMessage->FirstRegister = firstRegister;
	pReportMessage->NumRegisters = numRegisters;

	return SendNetworkPacket(toUnitID, (void *)(outbuf), sizeof(RMESSAGE_SYSREGISTERS_REPORT)+numRegisters*2 );
}

// Helper routine - notify Master of the system event

bool RProtocolMaster::NotifySysEvent(uint8_t eventType, uint32_t timeStamp, uint16_t seqID, uint8_t flags, uint8_t eventDataLength, uint8_t *eventData)
{
	if( !(GetEvtMasterFlags() & EVTMASTER_FLAGS_REPORT_SYSTEM) )
		return false;	// no Master registered to receive System notifications

	if( eventDataLength > SYSEVENT_MAX_STRING_LENGTH )
	{
		SYSEVT_ERROR(F("SendSysEvent - too big payload"));
		return false;																				
	}
	TRACE_VERBOSE(F("NotifySysEvent - evtDataLength:%u, str:'%s'\n"), eventDataLength, eventData );

	uint8_t		outbuf[sizeof(RMESSAGE_SYSEVT_REPORT)+SYSEVENT_MAX_STRING_LENGTH];
	RMESSAGE_SYSEVT_REPORT *pReportMessage = (RMESSAGE_SYSEVT_REPORT*) outbuf;

	pReportMessage->Header.ProtocolID = RPROTOCOL_ID;
	pReportMessage->Header.FCode = FCODE_SYSEVT_REPORT;
	pReportMessage->Header.FromUnitID = GetMyStationID();
	pReportMessage->Header.ToUnitID = GetEvtMasterStationID();
	pReportMessage->Header.TransactionID = 0;
	pReportMessage->Header.Length = eventDataLength + sizeof(RMESSAGE_SYSEVT_REPORT)-sizeof(RMESSAGE_HEADER);

	memcpy(&(pReportMessage->EvtString), eventData, size_t(eventDataLength) );

	pReportMessage->TimeStamp = timeStamp;
	pReportMessage->SeqID = seqID;
	pReportMessage->Flags = flags;
	pReportMessage->EventType = eventType;
	pReportMessage->NumDataBytes = eventDataLength;

	return SendNetworkPacket(pReportMessage->Header.ToUnitID, (void *)(outbuf), sizeof(RMESSAGE_SYSEVT_REPORT)+eventDataLength );
}


// Helper routine - send EvtMaster registration report

bool RProtocolMaster::SendEvtMasterReport(uint8_t transactionID, uint8_t fromUnitID, uint8_t toUnitID)
{
	RMESSAGE_EVTMASTER_REPORT Message;

	Message.Header.ProtocolID = RPROTOCOL_ID;
	Message.Header.FCode = FCODE_EVTMASTER_REPORT;
	Message.Header.FromUnitID = fromUnitID;
	Message.Header.ToUnitID = toUnitID;
	Message.Header.TransactionID = transactionID;
	Message.Header.Length = sizeof(RMESSAGE_EVTMASTER_REPORT)-sizeof(RMESSAGE_HEADER);	

#ifndef SG_STATION_MASTER	// we send remote master notifications only if this station is not a master by itself
	Message.EvtFlags = GetEvtMasterFlags();
	Message.MasterStationID = GetEvtMasterStationID();
#else
	Message.EvtFlags = 0;
	Message.MasterStationID = 0;
#endif
	Message.MasterStationAddress = 0;

	return SendNetworkPacket(toUnitID, (void *)(&Message), sizeof(Message) );
}


// Helper routine - send Ping reply

bool RProtocolMaster::SendPingReply(uint8_t transactionID, uint8_t fromUnitID, uint8_t toUnitID, uint32_t cookie)
{
	RMESSAGE_PING_REPLY Message;

	Message.Header.ProtocolID = RPROTOCOL_ID;
	Message.Header.FCode = FCODE_PING_REPLY;
	Message.Header.FromUnitID = fromUnitID;
	Message.Header.ToUnitID = toUnitID;
	Message.Header.TransactionID = transactionID;
	Message.Header.Length = sizeof(RMESSAGE_PING_REPLY)-sizeof(RMESSAGE_HEADER);	

	Message.cookie = cookie;	

	return SendNetworkPacket(toUnitID, (void *)(&Message), sizeof(Message) );
}
	


// Helper function - send generic OK response
bool RProtocolMaster::SendOKResponse(uint8_t transactionID, uint8_t fromUnitID, uint8_t toUnitID, uint8_t FCode)
{
	RMESSAGE_RESPONSE_OK  ResponseMessage;

	ResponseMessage.Header.ProtocolID = RPROTOCOL_ID;
	ResponseMessage.Header.FCode = FCODE_RESPONSE_OK;
	ResponseMessage.Header.FromUnitID = fromUnitID;
	ResponseMessage.Header.ToUnitID = toUnitID;
	ResponseMessage.Header.TransactionID = transactionID;
	ResponseMessage.Header.Length = 1;

	ResponseMessage.SuccessFCode = FCode;
	return SendNetworkPacket(toUnitID, (void *)(&ResponseMessage), sizeof(ResponseMessage) );
}

//
// Common helper - send error response packet
//
bool RProtocolMaster::SendErrorResponse(uint8_t transactionID, uint8_t fromUnitID, uint8_t toUnitID, uint8_t fCode, uint8_t errorCode)	// send error response with the same transaction ID, 
{																									// unit ID, FCode=1 and Exception Code=2 (Illegal Data Address)
	RMESSAGE_RESPONSE_ERROR	Message;

	Message.Header.ProtocolID = RPROTOCOL_ID;
	Message.Header.TransactionID = transactionID;
	Message.Header.FCode = FCODE_RESPONSE_ERROR;
	Message.Header.FromUnitID = fromUnitID;
	Message.Header.ToUnitID = toUnitID;
	Message.Header.Length = 2;

	Message.FailedFCode = fCode;
	Message.ExceptionCode = errorCode;
	return SendNetworkPacket(toUnitID, (void *)(&Message), sizeof(Message) );
}



//
// To reduce RAM overhead we declare some of the response handling functions as inline, since these functions are used just once,
//  and separation of this code into a function is done just for clarity/manageability reasons.
//

inline void MessageZonesReport( void *ptr )
{
	RMESSAGE_ZONES_REPORT	*pMessage = (RMESSAGE_ZONES_REPORT *)ptr;

	TRACE_VERBOSE(F("MessageZonesReport - processing message\n"));
	TRACE_VERBOSE(F("Header.Length=%d, FirstZone=%d, NumZones=%d\n"), (int)(pMessage->Header.Length), (int)(pMessage->FirstZone), (int)(pMessage->NumZones) );

	// parameters length check
	if( pMessage->Header.Length != (sizeof(RMESSAGE_ZONES_REPORT)-sizeof(RMESSAGE_HEADER)) )
	{
		SYSEVT_ERROR(F("MessageZonesReport - bad parameters length"));
		return;		// Note: In RProtocol station does not respond to malformed/invalid packets, just ignore it
	}
	
// Parameters validity check.

// Note: since currently we have no more than 8 zones in the Remote station, we hardcode response size to 1 byte
	if( ((pMessage->FirstZone+pMessage->NumZones) > GetNumZones()) || (pMessage->Header.FromUnitID > MAX_REMOTE_STATIONS) )
	{
		SYSEVT_ERROR(F("MessageZonesReport - wrong payload data"));
		return;	
	}
// OK, payload seems to be valid.

	runState.ReportStationZonesStatus(pMessage->Header.FromUnitID, pMessage->ZonesData[0]);
}


inline void MessageSensorsReport( void *ptr )
{
	RMESSAGE_SENSORS_REPORT	*pMessage = (RMESSAGE_SENSORS_REPORT *)ptr;

//	SYSEVT_ERROR(F("MessageSensorsReport - processing message from station %d\n"), (int)(pMessage->Header.FromUnitID));
//	SYSEVT_ERROR(F("Header.Length=%d, FirstSensor=%d, NumSensors=%d\n"), (int)(pMessage->Header.Length), (int)(pMessage->FirstSensor), (int)(pMessage->NumSensors) );

	// parameters length check
	if( pMessage->Header.Length != ((sizeof(RMESSAGE_SENSORS_REPORT)-sizeof(RMESSAGE_HEADER)) + (pMessage->NumSensors-1)*2) )
	{
		SYSEVT_ERROR(F("MessageSensorsReport - bad parameters length"));
		return;		
	}
	
// Basic parameters validity check.
	if( pMessage->Header.FromUnitID > MAX_REMOTE_STATIONS )
	{
		SYSEVT_ERROR(F("MessageSensorsReport - wrong payload data."));
		return;	
	}
// OK, payload seems to be valid.

	uint8_t  nSensors = pMessage->NumSensors;
	uint8_t	 nFirstSensor = pMessage->FirstSensor;

	for( uint8_t i=0; i<nSensors; i++ )
	{
//		SYSEVT_ERROR(F("MessageSensorsReport - reporting sensor reading, station=%d, channel=%d\n"), (int)(pMessage->Header.FromUnitID), (int)(nFirstSensor+i));		
		sensorsModule.ReportSensorReading(pMessage->Header.FromUnitID, nFirstSensor+i, (int) (pMessage->SensorsData[i]));
//		SYSEVT_ERROR(F("station=%d after return from ReportSensorReading\n"), (int)(pMessage->Header.FromUnitID));
	}
}
 
inline void MessageSystemRegistersReport( void *ptr )
{

}

//
// Remote SysEvent message
//

inline void MessageSysEvtReport( void *ptr )
{
	RMESSAGE_SYSEVT_REPORT	*pMessage = (RMESSAGE_SYSEVT_REPORT *)ptr;

	TRACE_VERBOSE(F("MessageSysEvtReport - processing message\n"));

// Parameters validity check.

	if( pMessage->NumDataBytes > SYSEVENT_MAX_STRING_LENGTH )
	{
		SYSEVT_ERROR(F("MessageSysEvtReport - payload data is too long (%u)"), uint16_t(pMessage->NumDataBytes));
		return;	
	}
// OK, payload seems to be valid.

	uint8_t		evtBuf[SYSEVENT_MAX_STRING_LENGTH+1];
	memcpy(evtBuf, (void *)(pMessage->EvtString), pMessage->NumDataBytes);
	evtBuf[pMessage->NumDataBytes] = 0; // we need to bull-terminate the string since incoming message has string without termination (but with explicit length)

	syslog_evt(pMessage->EventType, F("RemoteEvent: %s"), evtBuf);
}



inline void MessagePingReply( void *ptr )
{
	RMESSAGE_PING_REPLY	*pMessage = (RMESSAGE_PING_REPLY *)ptr;

	// parameters length check
	if( pMessage->Header.Length != (sizeof(RMESSAGE_PING_REPLY)-sizeof(RMESSAGE_HEADER)) )
	{
		SYSEVT_ERROR(F("MessagePingReply - bad parameters length"));
		return;		
	}
	
	SYSEVT_ERROR(F("MessagePingReply - Received from station: %d"), (int)(pMessage->Header.FromUnitID) );
}

inline void MessageEvtMasterReport( void *ptr )
{
	RMESSAGE_EVTMASTER_REPORT	*pMessage = (RMESSAGE_EVTMASTER_REPORT *)ptr;

	// parameters length check
	if( pMessage->Header.Length != (sizeof(RMESSAGE_EVTMASTER_REPORT)-sizeof(RMESSAGE_HEADER)) )
	{
		SYSEVT_ERROR(F("MessageEvtMasterReport - bad parameters length"));
		return;		
	}
	
//	SYSEVT_ERROR(F("MessageEvtMasterReport - Received from station: %d, EvtMaster=%d, NetAddr=%d\n"), (int)(pMessage->Header.FromUnitID), (int)(pMessage->MasterStationID), (int)(pMessage->MasterStationAddress) );
}

inline void MessageResponseError( void *ptr )
{
	RMESSAGE_RESPONSE_ERROR	*pMessage = (RMESSAGE_RESPONSE_ERROR *)ptr;

	// parameters length check
	if( pMessage->Header.Length != (sizeof(RMESSAGE_RESPONSE_ERROR)-sizeof(RMESSAGE_HEADER)) )
	{
		SYSEVT_ERROR(F("MessageResponseError - bad parameters length"));
		return;		
	}
	
	SYSEVT_ERROR(F("MessageResponseError - Received from station: %d, FailedFCode=%d, ExceptionCode=%d"), (int)(pMessage->Header.FromUnitID), (int)(pMessage->FailedFCode), (int)(pMessage->ExceptionCode) );
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

        Message.Header.ProtocolID = RPROTOCOL_ID;
		Message.Header.FCode = FCODE_ZONES_READ;
        Message.Header.ToUnitID = stationID;
        Message.Header.FromUnitID = GetMyStationID();
        Message.Header.Length = sizeof(RMESSAGE_ZONES_READ)-sizeof(RMESSAGE_HEADER);
		Message.Header.TransactionID = transactionID;

        Message.FirstZone = 0;
		Message.NumZones = 0x0FF;	// read all station zone channels

		return SendNetworkPacket(stationID, (void *)(&Message), sizeof(Message) );
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

        Message.Header.ProtocolID = RPROTOCOL_ID;
		Message.Header.FCode = FCODE_SYSREGISTERS_READ;
        Message.Header.ToUnitID = stationID;
        Message.Header.FromUnitID = MY_STATION_ID;
        Message.Header.Length = sizeof(RMESSAGE_SYSREGISTERS_READ)-sizeof(RMESSAGE_HEADER);
		Message.Header.TransactionID = transactionID;

        Message.FirstRegister = startRegister;
		Message.NumRegisters = numRegisters;	

		return SendNetworkPacket(stationID, (void *)(&Message), sizeof(Message) );
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

        Message.Header.ProtocolID = RPROTOCOL_ID;
		Message.Header.FCode = FCODE_SENSORS_READ;
        Message.Header.ToUnitID = stationID;
        Message.Header.FromUnitID = MY_STATION_ID;
        Message.Header.Length = sizeof(RMESSAGE_SENSORS_READ)-sizeof(RMESSAGE_HEADER);
		Message.Header.TransactionID = transactionID;

        Message.FirstSensor = 0;
		Message.NumSensors = 0x0FF;	

		return SendNetworkPacket(stationID, (void *)(&Message), sizeof(Message) );
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

		//TRACE_VERBOSE(F("SendForceSingleZone - sending request\n"));

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

		return SendNetworkPacket(stationID, (void *)(&Message), sizeof(Message) );
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

		return SendNetworkPacket(stationID, (void *)(&Message), sizeof(Message) );
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

		return SendNetworkPacket(stationID, (void *)(&Message), sizeof(Message) );
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

//		SYSEVT_ERROR(F("SendRegisterEventsMaster - sending request\n"));

        Message.Header.ProtocolID = RPROTOCOL_ID;
		Message.Header.FCode = FCODE_EVTMASTER_SET;
        Message.Header.ToUnitID = stationID;
        Message.Header.FromUnitID = MY_STATION_ID;
        Message.Header.Length = sizeof(RMESSAGE_EVTMASTER_SET)-sizeof(RMESSAGE_HEADER);
		Message.Header.TransactionID = transactionID;

        Message.EvtFlags = EVTMASTER_FLAGS_REGISTER_SELF | eventsMask;
		Message.MasterStationAddress = Message.MasterStationID = 0;
		Message.Flags = RMESSAGE_FLAGS_ACK_STD;

		return SendNetworkPacket(stationID, (void *)(&Message), sizeof(Message) );
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
inline void SendTimeBroadcastInt(void)
{
	RMESSAGE_TIME_BROADCAST Message;

		TRACE_INFO(F("Sending time broadcast\n"));

        Message.Header.ProtocolID = RPROTOCOL_ID;
		Message.Header.FCode = FCODE_TIME_BROADCAST;
        Message.Header.ToUnitID = STATIONID_BROADCAST;	// broadcast
        Message.Header.FromUnitID = MY_STATION_ID;
        Message.Header.Length = sizeof(RMESSAGE_TIME_BROADCAST)-sizeof(RMESSAGE_HEADER);
		Message.Header.TransactionID = 0;

		Message.timeNow = now();

		SendNetworkPacket(STATIONID_BROADCAST, (void *)(&Message), sizeof(Message) );
}

//
//	Client packets
//
//
//	RProtocol packets processing routines - FCODE_TIME_BROADCAST
//
//	Input - pointer to the input packet 
// 			It is assumed that basic input packet structure is already validated
//	
//	FCODE_TIME_BROADCAST expects to receive one parameter - timeNow (4 bytes)
//	Note: FCODE_TIME_BROADCAST message is broadcast, and does not have valid ToUnitID (it will be 0x0FF).
//  
//  Stations are not expected to respond to this message
//
inline void MessageTimeBroadcast( void *ptr )
{
	register RMESSAGE_TIME_BROADCAST  *pMessage = (RMESSAGE_TIME_BROADCAST *)ptr;

//	SYSEVT_ERROR(F("MessageTimeBroadcast - entering\n"));

// parameters length check
	if( pMessage->Header.Length != (sizeof(RMESSAGE_TIME_BROADCAST)-sizeof(RMESSAGE_HEADER)) )
	{
		SYSEVT_ERROR(F("MessageTimeBroadcast - bad parameters length"));
		return;		// Note: In RProtocol station does not respond to malformed/invalid packets, just ignore it
	}

// OK, message seems to be valid, set new time

	setTime((time_t) (pMessage->timeNow));
	nntpTimeServer.SetLastUpdateTime();
}

//
//	packets processing routines - Read EvtMaster Status
//
//	Input - pointer to the input packet (packet includes header)
// 			It is assumed that basic input packet structure is already validated
//
//			Second parameter is the sender network address, it will be used to send response.
//	
//	The routine will process input, execute required action(s), and will generate output packet
//	using common helpers
//
//	read EvtMaster registration status expects to receive no parameters in the Data area.
//
inline void MessageEvtMasterRead( void *ptr )
{
	register RMESSAGE_EVTMASTER_READ	*pMessage = (RMESSAGE_EVTMASTER_READ *)ptr;

// no parameters

	rprotocol.SendEvtMasterReport(pMessage->Header.TransactionID, pMessage->Header.ToUnitID, pMessage->Header.FromUnitID);
}

//
//	packets processing routines - Set EvtMaster Status
//
//	Input - pointer to the input packet (packet includes header)
// 			It is assumed that basic input packet structure is already validated
//
//			Second parameter is the sender network address, it will be used to send response.
//	
//	The routine will process input, execute required action(s), and will generate output packet
//	using common helpers
//
//
inline void MessageEvtMasterSet( void *ptr )
{
	register RMESSAGE_EVTMASTER_SET	*pMessage = (RMESSAGE_EVTMASTER_SET *)ptr;

	// parameters length check
	if( pMessage->Header.Length != (sizeof(RMESSAGE_EVTMASTER_SET) - sizeof(RMESSAGE_HEADER)) )
	{
		SYSEVT_ERROR(F("MessageEvtMasterSet - bad parameters length"));
		return;		
	}
	
// OK, parameters are valid. Execute action and send response

//	SYSEVT_ERROR(F("MessageEvtMasterSet - Setting EvtMaster, stationID=%d\n"), uint8_t(pMessage->Header.FromUnitID));

	SetEvtMasterFlags(pMessage->EvtFlags);
	if( pMessage->EvtFlags & EVTMASTER_FLAGS_REGISTER_SELF )	// flag indicating that we should register sender of this message as the master
	{
		SetEvtMasterStationID(pMessage->Header.FromUnitID);
//		SetEvtMasterStationAddress(netAddress);
	}
	else
	{
		SetEvtMasterStationID(pMessage->MasterStationID);
//		SetEvtMasterStationAddress(pMessage->MasterStationAddress);
	}

	// All done, check the type of response requested and send response.

	if( pMessage->Flags & RMESSAGE_FLAGS_ACK_BRIEF ) 
		rprotocol.SendOKResponse(pMessage->Header.TransactionID, pMessage->Header.ToUnitID, pMessage->Header.FromUnitID, pMessage->Header.FCode);

	if( pMessage->Flags & RMESSAGE_FLAGS_ACK_REPORT ) 
		rprotocol.SendEvtMasterReport(pMessage->Header.TransactionID, pMessage->Header.ToUnitID, pMessage->Header.FromUnitID);
}

//
//	RProtocol packets processing routines - FCODE_SENSORS_READ
//
//	Input - pointer to the input packet 
// 			and sender network address
//	
//	The routine will process input, execute required action(s), and will generate output packet
//	using common helpers
//
//
inline void MessageSensorsRead( void *ptr )
{
	register RMESSAGE_SENSORS_READ	*pMessage = (RMESSAGE_SENSORS_READ *)ptr;

	// parameters length check
	if( pMessage->Header.Length != (sizeof(RMESSAGE_SENSORS_READ)-sizeof(RMESSAGE_HEADER)) )
	{
		SYSEVT_ERROR(F("MessageSensorsRead - bad parameters length"));
		return;		// Note: In RProtocol station does not respond to malformed/invalid packets, just ignore it
	}

	if( (pMessage->FirstSensor == 0) && (pMessage->NumSensors == 0x0FF) )	// check for the special "magic" value of 0xFF, which means "read all sensors"
		pMessage->NumSensors = GetNumSensors();

// Parameters validity check. 

	if( (pMessage->FirstSensor+pMessage->NumSensors) > GetNumSensors() )
	{
		SYSEVT_ERROR(F("MessageSensorsRead - wrong input parameters"));
	
		rprotocol.SendErrorResponse(pMessage->Header.TransactionID, pMessage->Header.ToUnitID, pMessage->Header.FromUnitID, pMessage->Header.FCode, 2);	// send error response with the same transaction ID, 
		return;																										// unit ID, Exception Code=2 (Illegal Data Address)
	}
// OK, parameters are valid. Get the data and send response

	rprotocol.SendSensorsReport(pMessage->Header.TransactionID, pMessage->Header.ToUnitID, pMessage->Header.FromUnitID, pMessage->FirstSensor, pMessage->NumSensors);

	return;
}

//
//	packets processing routines - Ping
//
//	Input - pointer to the input packet (packet includes header)
// 			It is assumed that basic input packet structure is already validated
//
//			Second parameter is the sender network address, it will be used to send response.
//	
//	The routine will process input and will reply with an output packet
//
//	Ping expects to receive one parameter in the Data area:
//
//		1.	Cookie
//
inline void MessagePing( void *ptr )
{
	register RMESSAGE_PING	*pMessage = (RMESSAGE_PING *)ptr;

	// parameters length check
	if( pMessage->Header.Length != (sizeof(RMESSAGE_PING)-sizeof(RMESSAGE_HEADER)) )
	{
		SYSEVT_ERROR(F("MessagePing - bad parameters length"));
		return;		
	}
	
	rprotocol.SendPingReply(pMessage->Header.TransactionID, pMessage->Header.ToUnitID, pMessage->Header.FromUnitID, pMessage->cookie);

	return;
}

//
//	RProtocol packets processing routines - SCAN
//
//	Input - pointer to the input packet (packet includes MBAP header)
// 			It is assumed that basic input packet structure is already validated
//	
//	The routine will process input, execute required action(s), and will generate output packet
//	using common helpers
//
//	SCAN (extended FCode - discover stations on RF network) expects to receive no parameters.
//	Note: SCAN message is broadcast, and does not have valid UnitID.
//  
//  Stations are expected to respond to this message with SCAN_REPLY. 
//
//	Note: the sequence of SCAN->SCAN_REPLY does not follow standard RProtocol response conventions - 
//		  standard RProtocol response quotes the same FCode as the request, however in the SCAN case the station 
//		  is replying wiht SCAN_REPLY - because the request in this case is broadcast but response is unicast (direct response to the sender).
//
inline void MessageStationsScan( void *ptr )
{
	register RMESSAGE_SCAN	*pMessage = (RMESSAGE_SCAN *)ptr;

//	rprotocol.SendResponse(pMessage->Header.TransactionID, rprotocol.myUnitID, FCODE_SCAN_REPLY, 0, 0);	// send confirmation response 
	
	return;
}


// Process new data frame coming from the network
// ptr			- pointer to the data block,
// len			- block length
// netAddress	- sender's RF network address
// rssi			- recieving singlal strength indicator
//
void RProtocolMaster::ProcessNewFrame(uint8_t *ptr, int len, uint8_t *pNetAddress )
{
        register RMESSAGE_GENERIC *pMessage = (RMESSAGE_GENERIC *)ptr;    // for convenience of interpreting the packet

// first of all let's validate MBAP header and length constrains

        if( (unsigned int)len < sizeof(RMESSAGE_GENERIC) )   // minimum valid packet length 
        {
                SYSEVT_ERROR(F("ProcessNewFrame - Bad packet received, too short"));
				return;
        }

        if( pMessage->Header.ProtocolID != RPROTOCOL_ID )
        {
                SYSEVT_ERROR(F("ProcessNewFrame - Bad packet received, bad ProtocolID"));
                return;
        }

		if( (unsigned int)len != (pMessage->Header.Length+sizeof(RMESSAGE_HEADER)) )		// overall packet length should equal header size + length field
        {
                SYSEVT_ERROR(F("ProcessNewFrame - Bad packet received, length does not match"));
				TRACE_ERROR(F("\n Transport pkt len: %u, header len:%u, expected payload len:%u\n"), uint16_t(len), uint16_t(sizeof(RMESSAGE_HEADER)), uint16_t(pMessage->Header.Length) );
                return;
        }

// OK, the packet seems to be valid

		LastReceivedStationID = pMessage->Header.FromUnitID;

// first update timestamp of the last contact for the station

		if( pMessage->Header.FromUnitID < MAX_STATIONS ){	// basic protection check to ensure we don't go outside of range
			
			runState.sLastContactTime[pMessage->Header.FromUnitID] = millis();
			runState.iLastReceivedRSSI[pMessage->Header.FromUnitID] = LastReceivedRSSI;
		}
// and report station->address association to ARP (if registered)

		if( (_ARPAddressUpdate != 0) && (pNetAddress != 0) ) _ARPAddressUpdate(pMessage->Header.FromUnitID, pNetAddress);

		TRACE_INFO(F("ProcessNewFrame - processing packet, FCode: %d\n"), pMessage->Header.FCode);

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

				case FCODE_SYSEVT_REPORT:
								MessageSysEvtReport( ptr );
								break;

                case FCODE_PING_REPLY:
                                MessagePingReply( ptr);
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

				case FCODE_PING:	
								MessagePing( ptr );
								break;

//
// Packets used when acting as a client
//
#ifdef SG_RF_TIME_CLIENT
				case FCODE_TIME_BROADCAST:
								MessageTimeBroadcast( ptr );
								break;
#endif //SG_RF_TIME_CLIENT
#ifdef SG_STATION_SLAVE
				case FCODE_ZONES_READ:	
								MessageZonesRead( ptr );
								break;
	
				case FCODE_ZONES_SET:	
								MessageZonesSet( ptr );
								break;

				case FCODE_EVTMASTER_READ:
								MessageEvtMasterRead( ptr );
								break;

				case FCODE_EVTMASTER_SET:
								MessageEvtMasterSet( ptr );
								break;

				case FCODE_SENSORS_READ:	
								MessageSensorsRead( ptr );
								break;

				case FCODE_SCAN:	
								MessageStationsScan( ptr );
								break;
	
#endif //SG_STATION_SLAVE	

				default: 
								SYSEVT_ERROR(F("Unknown packet received"));
								break;
		}

        return;
}


//
//	Remote station requests
//

//
// Turn On/Off channels
//

bool RProtocolMaster::ChannelOn( uint8_t stationID, uint8_t chan, uint8_t ttr )
{
	{   // limit scope of sStation declaration to save memory during subsequent call
		ShortStation	sStation;

		TRACE_INFO(F("RProtocol - ChannelOn, stationID=%d, channel=%d, ttr=%d\n"), (int)stationID, (int)chan, (int)ttr);

		if( stationID >= MAX_STATIONS )
		{
			SYSEVT_ERROR(F("RProtocol ChannelOn - stationID outside of range"));
			return false;
		}

		LoadShortStation(stationID, &sStation);
		if( !(sStation.stationFlags & STATION_FLAGS_VALID) || !(sStation.stationFlags & STATION_FLAGS_ENABLED) )
		{
			SYSEVT_ERROR(F("RProtocol ChannelOn - station %d is not enabled"), (int)stationID);
			return false;
		}

	}

// OK, everything seems to be good. Send command.

	return SendForceSingleZone( stationID, chan, ttr, 0 );
}

bool RProtocolMaster::ChannelOff( uint8_t stationID, uint8_t chan )
{
	return ChannelOn( stationID, chan, 0 );
}

bool RProtocolMaster::AllChannelsOff(uint8_t stationID)
{
	{
		ShortStation	sStation;

		if( stationID >= MAX_STATIONS )
		{
			SYSEVT_ERROR(F("XBee AllChannelsOff - stationID outside of range"));
			return false;
		}

		LoadShortStation(stationID, &sStation);
		if( !(sStation.stationFlags & STATION_FLAGS_VALID) || !(sStation.stationFlags & STATION_FLAGS_ENABLED) )
		{
			SYSEVT_ERROR(F("XBee AllChannelsOff - station %d is not enabled"), (int)stationID);
			return false;
		}

	}

// OK, everything seems to be valid. Send command.

	return SendTurnOffAllZones( stationID, 0 );
}

void RProtocolMaster::SendTimeBroadcast(void)
{
	static  time_t	nextBroadcastTime = 0;

	if(nextBroadcastTime <= now()){

		SendTimeBroadcastInt();

		nextBroadcastTime = now() + 60; //  every 60 seconds
	}  

}

bool RProtocolMaster::PollStationSensors(uint8_t stationID)
{
	{
		ShortStation	sStation;

		if( stationID >= MAX_STATIONS )
		{
			SYSEVT_ERROR(F("XBee PollStationSensors - stationID outside of range"));
			return false;
		}

		LoadShortStation(stationID, &sStation);
		if( !(sStation.stationFlags & STATION_FLAGS_VALID) || !(sStation.stationFlags & STATION_FLAGS_ENABLED) )
		{
			SYSEVT_ERROR(F("XBee PollStationSensors - station %d is not enabled"), (int)stationID);
			return false;
		}

		if( (sStation.networkID != NETWORK_ID_XBEE) && (sStation.networkID != NETWORK_ID_MOTEINORF) )
		{
			SYSEVT_ERROR(F("XBee PollStationSensors - station %d is of a wrong type (not XBee or RFM69)"), (int)stationID);
			return false;
		}
	}

	TRACE_INFO(F("PollStationSensors - sending request to station %d\n"), stationID);

// OK, everything seems to be ready. Send command.

	return SendReadSensors( stationID, 0 );
}

bool RProtocolMaster::SubscribeEvents( uint8_t stationID )
{
	{
		ShortStation	sStation;

		if( stationID >= MAX_STATIONS ) 
			return false;		// basic protection

		LoadShortStation(stationID, &sStation);
		if( !(sStation.stationFlags & STATION_FLAGS_ENABLED) || ((sStation.networkID != NETWORK_ID_XBEE) && (sStation.networkID != NETWORK_ID_MOTEINORF)) )
			return false;
	}
	return SendRegisterEvtMaster( stationID, EVTMASTER_FLAGS_REPORT_ALL, 0);
}



// Main RProtocol poller. loop() should be called frequently, to allow processing of incoming packets
//
// Loop() will call appropriate transports.
//


void RProtocolMaster::loop(void)
{
#ifdef HW_ENABLE_XBEE
		XBeeRF.loop();
#endif //HW_ENABLE_XBEE

#ifdef HW_ENABLE_MOTEINORF
		MoteinoRF.loop();
#endif //HW_ENABLE_MOTEINORF
}



RProtocolMaster rprotocol;
