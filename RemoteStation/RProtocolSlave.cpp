/*
        Remote Station Protocol support for Sprinklers system.

  This module is intended to be used with my modified version of the OpenSprinkler code, as well as
  with modified sprinkler control program sprinklers_pi.


This module handles remote communication protocol. Initial version operates over Xbee link in API mode, but protocol itself is 
generic and can operate over multiple transports.

The protocol is based on Modbus-TCP, for details please see separate document.


 Creative Commons Attribution-ShareAlike 3.0 license
 Copyright 2014 tony-osp (http://tony-osp.dreamwidth.org/)


*/


#include "RProtocolSlave.h"
#include "core.h"
#include "settings.h"
#include "SGSensors.h"


extern Sensors sensorsModule;


// Local static (ugly!)
static unsigned long		tLastSync = millis()-DEFAULT_MAX_OFFLINE_TDELTA;		// reset timestamp of the last sync time


// Local forward declarations

//void SendErrorResponse(uint16_t netAddress, uint16_t transactionID, uint8_t fCode, uint8_t errorCode);
//void SendResponse(uint16_t transactionID, uint8_t unitID, uint8_t fCode, uint8_t *dataPtr, size_t dataSize);
uint8_t getZonesStatus(void);
void 	setStationName( uint8_t *str, uint8_t length );
void	registerEventsMaster( uint8_t eventsLevel, uint8_t eventsMask, uint8_t extFlags );

inline uint16_t		getSingleSystemRegister(uint8_t regAddr);
inline bool			setSingleSystemRegister(uint8_t regAddr, uint16_t value);
inline uint16_t		getSingleSensor(uint8_t regAddr);


bool RProtocolSlave::begin(void)
{
;
}


// Helper routine - send Zones report

bool RProtocolSlave::SendZonesReport(uint16_t netAddress, uint16_t transactionID, uint8_t toUnitID, uint8_t firstZone, uint8_t numZones)
{		
		if( (firstZone+numZones) >  GetNumZones() )
		{
			trace(F("SendZonesReport - wrong input parameters\n"));
			return false;																										// unit ID, Exception Code=2 (Illegal Data Address)
		}
	
		RMESSAGE_ZONES_REPORT ReportMessage;

		ReportMessage.Header.ProtocolID = RPROTOCOL_ID;
		ReportMessage.Header.FCode = FCODE_ZONES_REPORT;
		ReportMessage.Header.FromUnitID = rprotocol.myUnitID;
		ReportMessage.Header.ToUnitID = toUnitID;
		ReportMessage.Header.TransactionID = transactionID;
		ReportMessage.Header.Length = 4;	// we assume that we have no more than 8 zones, hence status data is 1 byte
// Note: since currently we have no more than 8 zones in the Remote station, we hardcode response size

		uint8_t	zonesStatus = 0;
	
		for( uint8_t i=firstZone; i<(firstZone+numZones); i++)
		{
			if( isZoneOn(i+1) ) zonesStatus |= 1 << i;			// note: isZoneOn() uses 1-based numbering!
		}

		ReportMessage.StationFlags = ZONES_REPFLAG_STATION_ENABLED;
		ReportMessage.FirstZone = firstZone;
		ReportMessage.NumZones = numZones;
		ReportMessage.ZonesData[0] = zonesStatus;

		return _SendMessage(netAddress, &ReportMessage, sizeof(ReportMessage));	// send response with requested zones status bits
}


// Helper routine - send Sensors report

bool RProtocolSlave::SendSensorsReport(uint16_t netAddress, uint16_t transactionID, uint8_t toUnitID, uint8_t firstSensor, uint8_t numSensors)
{
	if( ((firstSensor+numSensors) > MODBUSMAP_SENSORS_MAX) || (numSensors < 1) )
	{
		trace(F("SendSensorsReport - wrong input parameters\n"));
		return false;																										// unit ID, Exception Code=2 (Illegal Data Address)
	}

	uint8_t		outbuf[sizeof(RMESSAGE_SENSORS_REPORT)+(MODBUSMAP_SENSORS_MAX-1)*2];
	RMESSAGE_SENSORS_REPORT *pReportMessage = (RMESSAGE_SENSORS_REPORT*) outbuf;

	pReportMessage->Header.ProtocolID = RPROTOCOL_ID;
	pReportMessage->Header.FCode = FCODE_SENSORS_REPORT;
	pReportMessage->Header.FromUnitID = rprotocol.myUnitID;
	pReportMessage->Header.ToUnitID = toUnitID;
	pReportMessage->Header.TransactionID = transactionID;
	pReportMessage->Header.Length = 2+numSensors*2;	

	for( uint8_t i=0; i<numSensors; i++ )
	{
		pReportMessage->SensorsData[i] = getSingleSensor(i+firstSensor);
	}

	pReportMessage->FirstSensor = firstSensor;
	pReportMessage->NumSensors = numSensors;

	return _SendMessage(netAddress, outbuf, sizeof(RMESSAGE_SENSORS_REPORT)+(numSensors-1)*2);	
}


// Helper routine - send Sensors report

bool RProtocolSlave::SendSystemRegisters(uint16_t netAddress, uint16_t transactionID, uint8_t toUnitID, uint8_t firstRegister, uint8_t numRegisters)
{
	if( (firstRegister+numRegisters) > MODBUSMAP_SYSTEM_MAX )
	{
		trace(F("SendSystemRegisters - wrong input parameters\n"));
		return false;																										// unit ID, Exception Code=2 (Illegal Data Address)
	}
	uint8_t		outbuf[sizeof(RMESSAGE_SYSREGISTERS_REPORT)+MODBUSMAP_SYSTEM_MAX*2];
	RMESSAGE_SYSREGISTERS_REPORT *pReportMessage = (RMESSAGE_SYSREGISTERS_REPORT*) outbuf;

	pReportMessage->Header.ProtocolID = RPROTOCOL_ID;
	pReportMessage->Header.FCode = FCODE_SYSREGISTERS_REPORT;
	pReportMessage->Header.FromUnitID = rprotocol.myUnitID;
	pReportMessage->Header.ToUnitID = toUnitID;
	pReportMessage->Header.TransactionID = transactionID;
	pReportMessage->Header.Length = 2+numRegisters;	

	
	for( uint8_t i=0; i<numRegisters; i++ )
	{
		pReportMessage->RegistersData[i] = getSingleSystemRegister(i+firstRegister);
	}

	pReportMessage->FirstRegister = firstRegister;
	pReportMessage->NumRegisters = numRegisters;

	return _SendMessage(netAddress, outbuf, sizeof(RMESSAGE_SYSREGISTERS_REPORT)+numRegisters*2);
}

// Helper routine - send EvtMaster registration report

bool RProtocolSlave::SendEvtMasterReport(uint16_t netAddress, uint16_t transactionID, uint8_t toUnitID)
{
	RMESSAGE_EVTMASTER_REPORT Message;

	Message.Header.ProtocolID = RPROTOCOL_ID;
	Message.Header.FCode = FCODE_EVTMASTER_REPORT;
	Message.Header.FromUnitID = rprotocol.myUnitID;
	Message.Header.ToUnitID = toUnitID;
	Message.Header.TransactionID = transactionID;
	Message.Header.Length = sizeof(RMESSAGE_EVTMASTER_REPORT)-sizeof(RMESSAGE_HEADER);	

	Message.EvtFlags = GetEvtMasterFlags();
	Message.MasterStationID = GetEvtMasterStationID();
	Message.MasterStationAddress = GetEvtMasterStationAddress();

	return _SendMessage(netAddress, &Message, sizeof(Message));
}


// Helper routine - send Ping reply

bool RProtocolSlave::SendPingReply(uint16_t netAddress, uint16_t transactionID, uint8_t toUnitID, uint32_t cookie)
{
	RMESSAGE_PING_REPLY Message;

	Message.Header.ProtocolID = RPROTOCOL_ID;
	Message.Header.FCode = FCODE_PING_REPLY;
	Message.Header.FromUnitID = rprotocol.myUnitID;
	Message.Header.ToUnitID = toUnitID;
	Message.Header.TransactionID = transactionID;
	Message.Header.Length = sizeof(RMESSAGE_PING_REPLY)-sizeof(RMESSAGE_HEADER);	

	Message.cookie = cookie;	

	return _SendMessage(netAddress, &Message, sizeof(Message));
}
	


// Helper function - send generic OK response
bool RProtocolSlave::SendOKResponse(uint16_t netAddress, uint16_t transactionID, uint8_t toUnitID, uint8_t FCode)
{
	RMESSAGE_RESPONSE_OK  ResponseMessage;

	ResponseMessage.Header.ProtocolID = RPROTOCOL_ID;
	ResponseMessage.Header.FCode = FCODE_RESPONSE_OK;
	ResponseMessage.Header.FromUnitID = rprotocol.myUnitID;
	ResponseMessage.Header.ToUnitID = toUnitID;
	ResponseMessage.Header.TransactionID = transactionID;
	ResponseMessage.Header.Length = 1;

	ResponseMessage.SuccessFCode = FCode;
	return _SendMessage(netAddress, &ResponseMessage, sizeof(ResponseMessage));	
}

//
// Common helper - send error response packet
//
bool RProtocolSlave::SendErrorResponse(uint16_t netAddress, uint16_t transactionID, uint8_t toUnitID, uint8_t fCode, uint8_t errorCode)	// send error response with the same transaction ID, 
{																									// unit ID, FCode=1 and Exception Code=2 (Illegal Data Address)
	RMESSAGE_RESPONSE_ERROR	Message;

	Message.Header.ProtocolID = RPROTOCOL_ID;
	Message.Header.TransactionID = transactionID;
	Message.Header.FCode = FCODE_RESPONSE_ERROR;
	Message.Header.FromUnitID = rprotocol.myUnitID;
	Message.Header.ToUnitID = toUnitID;
	Message.Header.TransactionID = transactionID;
	Message.Header.Length = 2;

	Message.FailedFCode = fCode;
	Message.ExceptionCode = errorCode;
	return _SendMessage(netAddress, &Message, sizeof(Message));	
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
inline void MessageEvtMasterRead( void *ptr, uint16_t netAddress )
{
	register RMESSAGE_EVTMASTER_READ	*pMessage = (RMESSAGE_EVTMASTER_READ *)ptr;

// no parameters

	rprotocol.SendEvtMasterReport(netAddress, pMessage->Header.TransactionID, pMessage->Header.FromUnitID);
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
inline void MessageEvtMasterSet( void *ptr, uint16_t netAddress )
{
	register RMESSAGE_EVTMASTER_SET	*pMessage = (RMESSAGE_EVTMASTER_SET *)ptr;

	// parameters length check
	if( pMessage->Header.Length != (sizeof(RMESSAGE_EVTMASTER_SET) - sizeof(RMESSAGE_HEADER)) )
	{
		trace(F("MessageEvtMasterSet - bad parameters length\n"));
		return;		
	}
	
// OK, parameters are valid. Execute action and send response

	SetEvtMasterFlags(pMessage->EvtFlags);
	if( pMessage->EvtFlags & EVTMASTER_FLAGS_REGISTER_SELF )	// flag indicating that we should register sender of this message as the master
	{
		SetEvtMasterStationID(pMessage->Header.FromUnitID);
		SetEvtMasterStationAddress(netAddress);
	}
	else
	{
		SetEvtMasterStationID(pMessage->MasterStationID);
		SetEvtMasterStationAddress(pMessage->MasterStationAddress);
	}

	// All done, check the type of response requested and send response.

	if( pMessage->Flags & RMESSAGE_FLAGS_ACK_BRIEF ) 
		rprotocol.SendOKResponse(netAddress, pMessage->Header.TransactionID, pMessage->Header.FromUnitID, pMessage->Header.FCode);

	if( pMessage->Flags & RMESSAGE_FLAGS_ACK_REPORT ) 
		rprotocol.SendEvtMasterReport(netAddress, pMessage->Header.TransactionID, pMessage->Header.FromUnitID);
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
inline void MessagePing( void *ptr, uint16_t netAddress )
{
	register RMESSAGE_PING	*pMessage = (RMESSAGE_PING *)ptr;

	// parameters length check
	if( pMessage->Header.Length != (sizeof(RMESSAGE_PING)-sizeof(RMESSAGE_HEADER)) )
	{
		trace(F("MessagePing - bad parameters length\n"));
		return;		
	}
	
	rprotocol.SendPingReply(netAddress, pMessage->Header.TransactionID, pMessage->Header.FromUnitID, pMessage->cookie);

	return;
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
inline void MessageZonesRead( void *ptr, uint16_t netAddress )
{
	register RMESSAGE_ZONES_READ	*pMessage = (RMESSAGE_ZONES_READ *)ptr;

	// parameters length check
	if( pMessage->Header.Length != (sizeof(RMESSAGE_ZONES_READ)-sizeof(RMESSAGE_HEADER)) )
	{
		trace(F("MessageZonesRead - bad parameters length\n"));
		return;		// Note: In RProtocol station does not respond to malformed/invalid packets, just ignore it
	}
	
	if( (pMessage->FirstZone == 0) && (pMessage->NumZones == 0x0FF) )		// check for the special "magic" number of 0x0FF, which means "read all zones"
		pMessage->NumZones =  GetNumZones();

// Parameters validity check. The station allows reading only zone status, address 0, and no more than the total number of zones

	if( ((pMessage->FirstZone+pMessage->NumZones) >  GetNumZones()) || (pMessage->NumZones < 1)  )
	{
		trace(F("MessageZonesRead - wrong data address or the number of zones to read\n"));
	
		rprotocol.SendErrorResponse(netAddress, pMessage->Header.TransactionID, pMessage->Header.FromUnitID, FCODE_ZONES_READ, 2);		// send error response with the same transaction ID, 
		return;																					// unit ID, FCode=1 and Exception Code=2 (Illegal Data Address)
	}
// OK, parameters are valid. Get the data and send response

	rprotocol.SendZonesReport(netAddress, pMessage->Header.TransactionID, pMessage->Header.FromUnitID, pMessage->FirstZone, pMessage->NumZones);

	return;
}


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
									return  DEFAULT_HARDWARE_VERSION;

		case MREGISTER_FIRMWARE_VERSION:
									return  DEFAULT_FIRMWARE_VERSION;
	
		case MREGISTER_SLAVE_ID:
									return  rprotocol.myUnitID;
	
		case MREGISTER_PAN_ID:
									return  rprotocol.PANId;
	
		case MREGISTER_STATION_STATUS:
									return   RPROTOCOL_STATUS_ENABLED;			
	
		case MREGISTER_TIMEDATE_LOW:
									return   now() & 0xFFFF;
	
		case MREGISTER_TIMEDATE_HIGH:
									return   (now()>>16) & 0xFFFF;
	
		case MREGISTER_MAX_DURATION:
									return   DEFAULT_MAX_DURATION;	// temporary
	
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
//	RProtocol packets processing routines - FCODE_SYSREGISTERS_READ
//
//	Input - pointer to the input packet 
// 			and net address of the sender
//	
//	The routine will process input, execute required action(s), and will generate output packet
//	using common helpers
//
//	FCODE_SYSREGISTERS_READ expects to receive two parameters in the Data area:
//
//		1.	FirstRegister to read 
//		2.	The total number of registers to read 
//
inline void MessageSystemRegistersRead( void *ptr, uint16_t netAddress)
{
	register RMESSAGE_SYSREGISTERS_READ	*pMessage = (RMESSAGE_SYSREGISTERS_READ *)ptr;

	// parameters length check
	if( pMessage->Header.Length != (sizeof(RMESSAGE_SYSREGISTERS_READ)-sizeof(RMESSAGE_HEADER)) )
	{
		trace(F("MessageSystemRegistersRead - bad parameters length\n"));
		return;		// Note: In RProtocol station does not respond to malformed/invalid packets, just ignore it
	}
	
// Parameters validity check. 

	if( ((pMessage->FirstRegister+pMessage->NumRegisters) > MODBUSMAP_SYSTEM_MAX) || (pMessage->NumRegisters < 1)  )
	{
		trace(F("MessageSystemRegistersRead - incorrect input parameters\n"));
	
		rprotocol.SendErrorResponse(netAddress, pMessage->Header.TransactionID, pMessage->Header.FromUnitID, pMessage->Header.FCode, 2);	// send error response with the same transaction ID, 
		return;																										// unit ID, Exception Code=2 (Illegal Data Address)
	}
// OK, parameters are valid. Get the data and send response

	rprotocol.SendSystemRegisters(netAddress, pMessage->Header.TransactionID, pMessage->Header.FromUnitID, pMessage->FirstRegister, pMessage->NumRegisters);

	return;
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
inline void MessageSensorsRead( void *ptr, uint16_t netAddress )
{
	register RMESSAGE_SENSORS_READ	*pMessage = (RMESSAGE_SENSORS_READ *)ptr;

	// parameters length check
	if( pMessage->Header.Length != (sizeof(RMESSAGE_SENSORS_READ)-sizeof(RMESSAGE_HEADER)) )
	{
		trace(F("MessageSensorsRead - bad parameters length\n"));
		return;		// Note: In RProtocol station does not respond to malformed/invalid packets, just ignore it
	}

	if( (pMessage->FirstSensor == 0) && (pMessage->NumSensors == 0x0FF) )	// check for the special "magic" value of 0xFF, which means "read all sensors"
		pMessage->NumSensors = MODBUSMAP_SENSORS_MAX;

// Parameters validity check. 

	if( (pMessage->FirstSensor+pMessage->NumSensors) > MODBUSMAP_SENSORS_MAX )
	{
		trace(F("MessageSensorsRead - wrong input parameters\n"));
	
		rprotocol.SendErrorResponse(netAddress, pMessage->Header.TransactionID, pMessage->Header.FromUnitID, pMessage->Header.FCode, 2);	// send error response with the same transaction ID, 
		return;																										// unit ID, Exception Code=2 (Illegal Data Address)
	}
// OK, parameters are valid. Get the data and send response

	rprotocol.SendSensorsReport(netAddress, pMessage->Header.TransactionID, pMessage->Header.FromUnitID, pMessage->FirstSensor, pMessage->NumSensors);

	return;
}

//
// Helper funciton - read single input register. 
// This routine is used to get actual sensors (represented by input registers).
//
//  Input - RProtocol address of the required register (addresses start from 0).
//
inline uint16_t		getSingleSensor(uint8_t regAddr)
{
	switch( regAddr )
	{
		case MREGISTER_TEMP_SENSOR1:
									return  sensorsModule.Temperature;
	
		case MREGISTER_HUMIDITY_SENSOR1:
									return  sensorsModule.Humidity;
	
	default:
			trace(F("getSingleSensorRegister - error, wrong register address %d\n"), regAddr);
			return 0;
	}
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
inline void MessageZonesSet( void *ptr, uint16_t netAddress )
{
	register RMESSAGE_ZONES_SET	*pMessage = (RMESSAGE_ZONES_SET *)ptr;

	// parameters length check
	if( pMessage->Header.Length != (sizeof(RMESSAGE_ZONES_SET) - sizeof(RMESSAGE_HEADER)) )		// we assume that this station has no more than 8 zones, so zones flags will be in one byte
	{
		trace(F("MessageZonesSet - bad parameters length\n"));
		return;		// Note: In RProtocol station does not respond to malformed/invalid packets, just ignore it
	}
	
// Parameters validity check. 

	if( ((pMessage->NumZones+pMessage->FirstZone) >  GetNumZones()) || (pMessage->NumZones < 1) )  //Check bounds
	{
		trace(F("MessageZonesSet - wrong parameters\n"));
	
		rprotocol.SendErrorResponse(netAddress, pMessage->Header.TransactionID, pMessage->Header.FromUnitID, pMessage->Header.FCode, 2);	// send error response with the same transaction ID, 
		return;																						// unit ID, Exception Code=2 (Illegal Data Address)
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
				runState.RemoteStartZone(false, pMessage->ScheduleID, i+1, pMessage->Ttr); 
		}
	}

	// All done, check the type of response requested and send response.

	if( pMessage->Flags & RMESSAGE_FLAGS_ACK_BRIEF ) 
		rprotocol.SendOKResponse(netAddress, pMessage->Header.TransactionID, pMessage->Header.FromUnitID, pMessage->Header.FCode);

	if( pMessage->Flags & RMESSAGE_FLAGS_ACK_REPORT ) 
		rprotocol.SendZonesReport(netAddress, pMessage->Header.TransactionID, pMessage->Header.FromUnitID, pMessage->FirstZone, pMessage->NumZones);
	
	return;
}


//
// Helper function - set single system register. 
// This routine is used to set system data and configuration.
//
//  Input - RProtocol address of the required register (addresses start from 0).
//
inline bool		setSingleSystemRegister(uint8_t regAddr, uint16_t value)
{
	switch( regAddr )
	{
		case MREGISTER_SLAVE_ID:
											rprotocol.myUnitID = value;
											return true;
	
		case MREGISTER_PAN_ID:
											rprotocol.PANId = value;
											return true;
	
		case MREGISTER_STATION_STATUS:

// TODO - need to implement actual status change
											return true;
		
		case MREGISTER_TIMEDATE_LOW:
											{
												time_t	t = now() & 0xFFFF0000;
												t = t | value;
												setTime(t);
											}
											return true;
	
		case MREGISTER_TIMEDATE_HIGH:
											{
												time_t	t = now() & 0xFFFF;
												t = t | (value<<16);
												setTime(t);
											}
											return true;
	
		case MREGISTER_MAX_DURATION:
// TODO - need to implement actual max duration change
											return true;
	
		default:
											return false;		
	}
}



//
//	RProtocol packets processing routines - FCODE_SYSREGISTERS_SET
//
//	Input - pointer to the input packet 
// 			and net Address of the sender
//	
//	The routine will process input, execute required action(s), and will generate output packet
//	using common helpers
//
//	FCODE_SYSREGISTERS_SET expects to receive four parameters in the Data area:
//
//		1.  Response flags
//		2.	FirstRegister
//		3.	The number of registers to write 
//		4.  RegistersData (two bytes each)
//
inline void MessageSystemRegistersSet( void *ptr, uint16_t netAddress)
{
	register RMESSAGE_SYSREGISTERS_SET	*pMessage = (RMESSAGE_SYSREGISTERS_SET *)ptr;

// parameters length check
	if( pMessage->Header.Length != (sizeof(RMESSAGE_SYSREGISTERS_SET)-sizeof(RMESSAGE_HEADER)) )
	{
		trace(F("MessageSystemRegistersSet - bad parameters length\n"));
		return;		// Note: In RProtocol station does not respond to malformed/invalid packets, just ignore it
	}
	
// Parameters validity check. 

	if( ((pMessage->FirstRegister+pMessage->NumRegisters) > MODBUSMAP_SYSTEM_MAX) || (pMessage->NumRegisters < 1)  )
	{
		trace(F("MessageSystemRegistersSet - incorrect input parameters\n"));
	
		rprotocol.SendErrorResponse(netAddress, pMessage->Header.TransactionID, pMessage->Header.FromUnitID, pMessage->Header.FCode, 2);	// send error response with the same transaction ID, 
		return;																										// unit ID, Exception Code=2 (Illegal Data Address)
	}

// OK, parameters are valid. Execute action and send response

	for( uint8_t i=0; i<pMessage->NumRegisters; i++)	
	{
		if( setSingleSystemRegister(pMessage->FirstRegister+i, pMessage->RegistersData[i]) != true)
		{
				rprotocol.SendErrorResponse(netAddress, pMessage->Header.TransactionID, pMessage->Header.FromUnitID, pMessage->Header.FCode, 4);	// send error response with the extended error code 4 (Device Slave Failure)				
				return;	// on any register failure don't continue, exit now
		}
	}

	// All done, check the type of response requested and send response.

	if( pMessage->Flags & RMESSAGE_FLAGS_ACK_BRIEF ) 
		rprotocol.SendOKResponse(netAddress, pMessage->Header.TransactionID, pMessage->Header.FromUnitID, pMessage->Header.FCode);

	if( pMessage->Flags & RMESSAGE_FLAGS_ACK_REPORT ) 
		rprotocol.SendSystemRegisters(netAddress, pMessage->Header.TransactionID, pMessage->Header.FromUnitID, pMessage->FirstRegister, pMessage->NumRegisters);
	
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
inline void MessageStationsScan( void *ptr, uint16_t netAddress)
{
	register RMESSAGE_SCAN	*pMessage = (RMESSAGE_SCAN *)ptr;

//	rprotocol.SendResponse(pMessage->Header.TransactionID, rprotocol.myUnitID, FCODE_SCAN_REPLY, 0, 0);	// send confirmation response 
	
	return;
}

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
void MessageTimeBroadcast( void *ptr, uint16_t netAddress, uint8_t rssi )
{
	register RMESSAGE_TIME_BROADCAST  *pMessage = (RMESSAGE_TIME_BROADCAST *)ptr;

//    localUI.lcd_print_line_clear_pgm(PSTR("Received Time Stamp"), 1);
//	delay(30000);

// parameters length check
	if( pMessage->Header.Length != (sizeof(RMESSAGE_TIME_BROADCAST)-sizeof(RMESSAGE_HEADER)) )
	{
		trace(F("MessageTimeBroadcast - bad parameters length\n"));
		return;		// Note: In RProtocol station does not respond to malformed/invalid packets, just ignore it
	}

// OK, message seems to be valid, set new time

	setTime((time_t) (pMessage->timeNow));
	tLastSync  = millis();

}


// Process new data frame coming from the network
// ptr - pointer to the data block,
// len - block length
//
void RProtocolSlave::ProcessNewFrame(uint8_t *ptr, uint8_t len, uint16_t netAddress, uint8_t rssi)
{
	register	RMESSAGE_GENERIC	*pMessage = (RMESSAGE_GENERIC *)ptr;	// for convenience of interpreting the packet
	
// first of all let's validate MBAP header and length constrains

	if( len < sizeof(RMESSAGE_GENERIC) )	
	{
		trace(F("Bad packet received, lenth is too small\n"));
		return;
	}

	if( pMessage->Header.ProtocolID != RPROTOCOL_ID )		
	{
		trace(F("Bad packet received, wrong ProtocolID: %d\n"), (int)(pMessage->Header.ProtocolID) );
		return;
	}
	
	if( len != (pMessage->Header.Length+sizeof(RMESSAGE_HEADER)) )		// overall packet length should equal header size + length field
	{
		trace(F("Bad packet received, length does not match\n"));
		return;
	}
	
// Check UnitID. It should be valid for all packets (except SCAN)

	if(  (pMessage->Header.ToUnitID != rprotocol.myUnitID) && (pMessage->Header.FCode != FCODE_TIME_BROADCAST) && (pMessage->Header.FCode != FCODE_SCAN))
	{
		trace(F("Bad packet received, ToUnitID not equal to this station RProtocol ID\n"));
		return;
	}
	
// Record last StationID we received packet from and rssi (for radio link monitoring)

	SetLastReceivedRssi(pMessage->Header.FromUnitID, rssi);

// OK, the packet seems to be valid, let's parse and dispatch it.
	
	switch( pMessage->Header.FCode )
	{
// Standard RProtocol FCodes

		case FCODE_TIME_BROADCAST:
						MessageTimeBroadcast( ptr, netAddress, rssi );
						break;
	
		case FCODE_ZONES_READ:	
						MessageZonesRead( ptr, netAddress );
						break;
	
		case FCODE_ZONES_SET:	
						MessageZonesSet( ptr, netAddress );
						break;
	
		case FCODE_SENSORS_READ:	
						MessageSensorsRead( ptr, netAddress );
						break;
	
		case FCODE_SYSREGISTERS_READ:	
						MessageSystemRegistersRead( ptr, netAddress );
						break;
	
		case FCODE_SYSREGISTERS_SET:	
						MessageSystemRegistersSet( ptr, netAddress );
						break;
	
		case FCODE_PING:	
						MessagePing( ptr, netAddress );
						break;

		case FCODE_SCAN:	
						MessageStationsScan( ptr, netAddress );
						break;

		case FCODE_EVTMASTER_READ:
						MessageEvtMasterRead( ptr, netAddress );
						break;

		case FCODE_EVTMASTER_SET:
						MessageEvtMasterSet( ptr, netAddress );
						break;

	}
	
	return;
}



// Local helper - register events master
//
void	registerEventsMaster( uint8_t eventsLevel, uint8_t eventsMask, uint8_t extFlags )
{

	return;
}

bool getNetworkStatus(void)
{

	if( (millis()-tLastSync) < DEFAULT_MAX_OFFLINE_TDELTA)
		return true;
	else
		return false;
}



RProtocolSlave rprotocol;