/*
        Remote Station Protocol support for Sprinklers system.

  This module is intended to be used with my SmartGarden multi-station monitoring and sprinkler control system.

This module handles remote communication protocol. Initial version operates over XBee link in API mode, but protocol itself is 
generic and can operate over multiple transports.

The protocol is using some concepts from Modbus-TCP, although it is not Modbus and is directly compatible with it. 
For details please see separate document.


Creative Commons Attribution-ShareAlike 3.0 license
Copyright 2014 tony-osp (http://tony-osp.dreamwidth.org/)

*/

#ifndef _SGRPROTOCOLDEF_h
#define _SGRPROTOCOLDEF_h

#include <inttypes.h>

// Remote protocol is using certain concepts similar to Modbus-TCP, but is not Modbus, and is not wire-compatible with it. 
// 
// Similarities:
//			1. Like Modbus, RProtocol is optimized for having simple, "dumb" devices
//				Types of protocol messages and commands are deliberately simple, based on read/write logic.
//
//			2. Like Modbus, RProtocol is optimized for Control and Monitoring tasks
//				Specifically - controlling irrigation Zones, and reading sensors (temperature/humidity/etc)
//
//			3. Like Modbus, RProtocol does not limit actual devices functionality (extensibility by defining "registers")
//
//			4. Like Modbus, RProtocol works well on low-bandwidth, slow links
//				Binary protocol, with fairly compact messages.
//
// Key differences:	
//			1. Unlike Modbus, RProtocol allows the same station to be simultaneously a Master and Slave
//			2. Unlike Modbus-TCP, RProtocol is designed for sessionless, datagram-type communication 
//				I.e. RProtocol is not relying on transport-level notion of sessions, and is not relying on the transport to determine master vs slave.
//
//			3. RProtocol allows supporting multi-network communication and even routing, while in Modbus it must be handled outside of Modbus layer
//			4. Support for unsolicited Notifications (long-standing deficiency in Modbus)
//

// RProtocol structure
//
// RProtocol uses Messages. Message always include Header, and message length is known (from transport).
// RProtocol can run over any transport that provides frame-like communication, 
//   for stream-like transports (e.g. Serial port, TCP) an additional framing layer must be provided.
// 
// RProtocol can run over both Reliable and Unreliable transports, and it includes optimizations allowing it 
//   to take advantage of the transport reliability (when available). It is achieved by allowing requester to specify 
//   desired response behaviour - positive acknowledgement of every message or negative acknowledgement.
//
// RProtocol maximum message length (including header) is XX bytes.
//
// RProtocol usually will be running over transport that guarantees message integrity (transport-level CRC).
// When running over transports that don't guarantee message integrity, validation code is appended to the message.

// RProtocol by itself does not support encryption. Communication security can be achieved by transport-level encryption, 
//   such as 802.15.4 AES encryption with shared key.
//
// If transport-level security is used (e.g. 802.15.4 AES), it will also provide basic pre-shared key authentication.
// 
// RProtocol can support optional protocol-level authentication, by adding validation suffix after the message 
//  (the same suffix will also provide message integrity check). Validation suffix is computed as a Hash of the message
//  together with the pre-configured key, providing pre-shared key authentication. 
//
// This scheme is not intended as the strong protection measure from a determined attacker 
//  (transport-level AES is recommended when there is a need in strong protection), but RProtocol-level pre-shared key authentication
//  is rather a mechanism for preventing unintentional interference with neighbouring RF networks.
//

#define RPROTOCOL_ID						1

//  function codes

// Zones query and control
#define FCODE_ZONES_READ					1
#define FCODE_ZONES_SET						2
#define FCODE_ZONES_REPORT					3

// Sensors
#define FCODE_SENSORS_READ					4
#define FCODE_SENSORS_REPORT				5

// System registers query and control
#define FCODE_SYSREGISTERS_READ				6
#define FCODE_SYSREGISTERS_SET				7
#define FCODE_SYSREGISTERS_REPORT			8

// System strings query and control
#define FCODE_SYSSTRING_READ				9
#define FCODE_SYSSTRING_SET					10
#define FCODE_SYSSTRING_REPORT				11

// Log/Master station registration (SysLog equivalent)
#define FCODE_EVTMASTER_READ				12
#define FCODE_EVTMASTER_SET					13
#define FCODE_EVTMASTER_REPORT				14


// Other
#define FCODE_SCAN							50
#define FCODE_SCAN_REPLY					51
#define FCODE_PING							52
#define FCODE_PING_REPLY					53

#define FCODE_TIME_BROADCAST				55

// Response 
#define FCODE_RESPONSE_OK					127
#define FCODE_RESPONSE_ERROR				128

//Header 

// All RProtocol messages have this header. 
// TransactionID is used to correlate request/response, and should be filled in.
// For unsolicited "inform"-type messages TransactionID should be zero.
//
// ToUnitID is the recipient address, the FromUnitID is the sender address.
//
// For Request-type messages ToUnitID is the StationID the request applies to,
// for Response/Inform-type messages information provided is about to FromUnitID.
//
//
struct RMESSAGE_HEADER
{
	uint8_t		ProtocolID;			// In the current version ProtocolID should be 1.
	uint8_t		TransactionID;		// Transaction serial number provided by the client. The server will quote this number when replying.
									// For Request messages TransactionID can be from 1 to 255, for unsolicited "inform" messages TransactionID must be zero.
	uint8_t		ToUnitID;			// Receiver Station ID
	uint8_t		FromUnitID;			// Sender Station ID
	uint8_t		Length;				// Message length (not including  header)

	uint8_t		FCode;				// Function code 
};

// ACK/Response flags that control acknowledgement/response for request messages.
// 
// Certain request messages have "Flags" field, and by setting these flags requester can ask for specific type of response.
// This allows the protocol to operate in ACK, NACK, or both modes.
//
// The RMESSAGE_FLAGS_ACK_REPORT flag also allows requester to ask for the report of the current state of the objects. 
// In this case the message recipient will send REPORT-type message for the objects in question - 
//   e.g. if the request was to SET Zones, the REPORT reply message will report status of zones after executing the SET command.
// This capability can help to reduce the number of back-and-forth messages on the RF network. 
//
// Each of these flags can be set independently. E.g. RMESSAGE_FLAGS_ACK_BRIEF can (and should) be set together with 
//  RMESSAGE_FLAGS_ACK_ERROR flag, to receive both success and failure responses.
//
#define RMESSAGE_FLAGS_ACK_NONE		0	// don't send any response
#define RMESSAGE_FLAGS_ACK_ERROR	1	// if 1, send error responses
#define RMESSAGE_FLAGS_ACK_BRIEF	2	// if 1, send brief success responses 
#define RMESSAGE_FLAGS_ACK_REPORT	4	// if 1, send info reports on success 
										// Info reports are the REPORT messages. 
										// E.g. FCODE_ZONES_REPORT will report current state 

// Standard response flags
#define RMESSAGE_FLAGS_ACK_STD		(RMESSAGE_FLAGS_ACK_ERROR | RMESSAGE_FLAGS_ACK_REPORT)

// Basic message definition, useful for header checks.
struct RMESSAGE_GENERIC
{
//  Header
	RMESSAGE_HEADER	Header;
};


//
//  FCODE_ZONES_READ - Read Zones Status
//
struct RMESSAGE_ZONES_READ
{
//  Header
	RMESSAGE_HEADER	Header;
	
// PDU

	uint8_t		FirstZone;			// First zone to read
	uint8_t		NumZones;			// Number of zones to read
};


//
//  FCODE_ZONES_SET - Set (start or stop) Zones 
//
struct RMESSAGE_ZONES_SET
{
//  Header
	RMESSAGE_HEADER	Header;
	
// PDU

	uint8_t		Flags;						// This field carries various flags for this message.
											// Most important are RMESSAGE_FLAGS_ACK_* flags that control acknowledgement/response.
	
	uint8_t		ScheduleID;					// ScheduleID - for correlation in reporting (could be 0)
	uint8_t		FirstZone;					// first zone to write
	uint8_t		Ttr;						// Time to run - number of minutes to run. If Ttr==0, then turn zone(s) Off.
											// Note1: Ttr is applied to all zones being turned On by the message
											// Note2: Ttr is subject to the Ttr cap set on the station level.
											//		  Ttr capping will not produce error, but actual time set (and reported) will be the capped time.
	uint8_t		NumZones;					// Number of zones to write
	uint8_t		ZonesData[1];				// Array of data bytes for zones bits (8 zones per byte)
											// Zones corresponding to bits set to 1 will be affected by the message (will be turned On or Off),
											// zones corresponding to bits set to 0 will not be affected.
};


//
//	FCODE_ZONES_REPORT - zones status information
//
//  This message will be sent in response to FCODE_ZONES_READ message, or in response to FCODE_ZONES_SET (if requested by the flag),
//    also this message may be sent as unsolicited information  message to the Master to inform about Zones Status values/change. 
//  When sent in response to FCODE_ZONES_READ or FCODE_ZONES_SET message, TransactionID will be the same as in the request, 
//    if sent unsolicited - it will be 0.
//
struct RMESSAGE_ZONES_REPORT
{
//  Header
	RMESSAGE_HEADER	Header;			
	
// PDU

	uint8_t		StationFlags;		// flags. Flag 1 indicates that the station is enabled
	uint8_t		FirstZone;			// first zone to report
	uint8_t		NumZones;			// Number of zones to write
	uint8_t		ZonesData[1];		// Array of data bytes for zones bits (8 zones per byte)
									// Bit value of 1 means "corresponding zone is On", 0 means "Off"
};

#define ZONES_REPFLAG_STATION_ENABLED	1

//
//  FCODE_SENSORS_READ - Read Sensors Status
//
struct RMESSAGE_SENSORS_READ
{
//  Header
	RMESSAGE_HEADER	Header;
	
// PDU

	uint8_t		FirstSensor;		// First Sensor to read
	uint8_t		NumSensors;			// Number of Sensors to read
};


//
//	FCODE_SENSORS_REPORT - Sensors status information
//
//  This message will be sent in response to FCODE_SENSORS_READ message,
//    also this message may be sent as unsolicited information  message to the Master to inform about Sensors Status values/change. 
//  When sent in response to FCODE_SENSORS_READ, TransactionID will be the same as in the request, 
//    if sent unsolicited - it will be 0.
//
struct RMESSAGE_SENSORS_REPORT
{
//  Header
	RMESSAGE_HEADER	Header;			
	
// PDU

	uint8_t		FirstSensor;		// First Sensor
	uint8_t		NumSensors;			// Number of Sensors
	uint16_t	SensorsData[1];		// Array of data words for sensors 
									// Note: the protocol does not define the number or types of sensors.
									//       Also the protocol allows combining two words into one dword for sensors that need 32bit precision
									//       E.g. SensorsData[0] and [1] can be treated as one sensor with 32bit precision.
};

//
//  FCODE_SYSREGISTERS_READ - Read System Registers 
//
struct RMESSAGE_SYSREGISTERS_READ
{
//  Header
	RMESSAGE_HEADER	Header;
	
// PDU

	uint8_t		FirstRegister;		// First register to read
	uint8_t		NumRegisters;		// Number of register to read
};


//
//  FCODE_SYSREGISTERS_SET - Set system registers
//
struct RMESSAGE_SYSREGISTERS_SET
{
//  Header
	RMESSAGE_HEADER	Header;
	
// PDU

	uint8_t		Flags;				// This field carries various flags for this message.
									// Most important are RMESSAGE_FLAGS_ACK_* flags that control acknowledgement/response.
	
	uint8_t		FirstRegister;		// First register to read
	uint8_t		NumRegisters;		// Number of register to read
	uint16_t	RegistersData[1];	// Array of data bytes for system registers
									// Note: the protocol does not define the number of system registers - it really depends on the station.
};


//
//	FCODE_SYSREGISTERS_REPORT - zones status information
//
//  This message will be sent in response to FCODE_SYSREGISTERS_READ message, or in response to FCODE_SYSREGISTERS_SET (if requested by the flag),
//    also this message may be sent as unsolicited information  message to the Master to inform about system registers values/change. 
//  When sent in response to FCODE_SYSREGISTERS_READ or FCODE_SYSREGISTERS_SET message, TransactionID will be the same as in the request, 
//    if sent unsolicited - it will be 0.
//
struct RMESSAGE_SYSREGISTERS_REPORT
{
//  Header
	RMESSAGE_HEADER	Header;			
	
// PDU

	uint8_t		FirstRegister;		// First register to read
	uint8_t		NumRegisters;		// Number of register to read
	uint16_t	RegistersData[1];	// Array of data bytes for system registers
									// Note: the protocol does not define the number of system registers - it really depends on the station.
};

//
//  FCODE_SYSSTRING_READ - Read System string (e.g. Station Name)
//
struct RMESSAGE_SYSSTRING_READ
{
//  Header
	RMESSAGE_HEADER	Header;
	
// PDU

	uint8_t		StringID;			// String ID to read
};


//
//  FCODE_SYSSTRING_SET - Set System string (e.g. Station Name)
//
struct RMESSAGE_SYSSTRING_SET
{
//  Header
	RMESSAGE_HEADER	Header;
	
// PDU

	uint8_t		StringID;			// String ID to read
	uint8_t		NumDataBytes;		// The number of bytes to follow
	uint8_t		String;				// String bytes, in ASCII. Note: the string is not terminated (its length is provided in NumDataBytes)
};

//
//  FCODE_SYSSTRING_REPORT - Report System string (e.g. Station Name)
//
//  This message will be sent in response to FCODE_SYSSTRING_READ message
//    also this message may be sent as unsolicited information  message to the Master to inform about system string value/change. 
//  When sent in response to FCODE_SYSSTRING_READ, TransactionID will be the same as in the request, 
//    if sent unsolicited - it will be 0.
//
struct RMESSAGE_SYSSTRING_REPORT
{
//  Header
	RMESSAGE_HEADER	Header;
	
// PDU

	uint8_t		StringID;			// String ID to read
	uint8_t		NumDataBytes;		// The number of bytes to follow
	uint8_t		String;				// String bytes, in ASCII. Note: the string is not terminated (its length is provided in NumDataBytes)
};



//
//  FCODE_EVTMASTER_READ - Read current EvtMaster registration
//
struct RMESSAGE_EVTMASTER_READ
{
//  Header
	RMESSAGE_HEADER	Header;
	
// PDU

};



//
//  FCODE_EVTMASTER_SET - Register EvtMaster
//
struct RMESSAGE_EVTMASTER_SET
{
//  Header
	RMESSAGE_HEADER	Header;
	
// PDU

	uint8_t		Flags;					// response flags (ACK etc)
	uint16_t	EvtFlags;				// Various flags indicating types of events to report etc
	uint8_t		MasterStationID;		// StationID to register as the Master
	uint16_t	MasterStationAddress;	// Network address of the Master station to register
};

#define EVTMASTER_FLAGS_REGISTER_SELF		1	// flag for RMESSAGE_EVTMASTER_SET, indicating that StationID and Network address of the sender should be used (values provied in the message are ignored)
#define EVTMASTER_FLAGS_REPORT_ZONES		4	// flag for RMESSAGE_EVTMASTER_SET, indicating that Zone start/stop events as well as station enable/disable events should be reported
#define EVTMASTER_FLAGS_REPORT_SENSORS		8	// flag for RMESSAGE_EVTMASTER_SET, indicating that new sensor readings should be sent (sensor polling frequency is defined by system registers)
#define EVTMASTER_FLAGS_REPORT_WATERING		16	// flag for RMESSAGE_EVTMASTER_SET, indicating that watering log records should be reported
#define EVTMASTER_FLAGS_REPORT_SYSTEM		32	// flag for RMESSAGE_EVTMASTER_SET, indicating that system changes notifications should be reported

#define EVTMASTER_FLAGS_REPORT_ALL	(EVTMASTER_FLAGS_REPORT_ZONES | EVTMASTER_FLAGS_REPORT_SENSORS | EVTMASTER_FLAGS_REPORT_WATERING | EVTMASTER_FLAGS_REPORT_SYSTEM)

//
//  FCODE_EVTMASTER_REPORT - Report System string (e.g. Station Name)
//
//  This message will be sent in response to FCODE_EVTMASTER_READ message
//
struct RMESSAGE_EVTMASTER_REPORT
{
//  Header
	RMESSAGE_HEADER	Header;
	
// PDU

	uint16_t	EvtFlags;				// Various flags indicating types of events to report 
	uint8_t		MasterStationID;		// StationID to registered as the Master
	uint16_t	MasterStationAddress;	// Network address of the Master station 
};



//
//  Response message. 
//	FCODE_RESPONSE_ERROR - failure.
//
//  This message is used as a response to any of the request messages to inform about request failure.
//
struct RMESSAGE_RESPONSE_ERROR
{
//  Header
	RMESSAGE_HEADER	Header;
	
// PDU

	uint8_t		FailedFCode;		// FCode for request that failed
	uint8_t		ExceptionCode;		// Exception code
};

//
//  Response message. 
//	FCODE_RESPONSE_OK - success.
//
//  This message is used as a response to any of the request messages to inform about request success
//  when RMESSAGE_FLAGS_ACK_BRIEF flag is set.
//
struct RMESSAGE_RESPONSE_OK
{
//  Header
	RMESSAGE_HEADER	Header;
	
// PDU

	uint8_t		SuccessFCode;		// FCode for request that was OK
};



// Extended FCode - SCAN
struct RMESSAGE_SCAN
{
//  Header
	RMESSAGE_HEADER	Header;
	
// PDU

// TODO - need to decide if this message should include any "announcement" parameters (e.g. Master name, capabilities etc).
};

// Extended FCode - SCAN_REPLY
struct RMESSAGE_SCAN_REPLY
{
//  Header
	RMESSAGE_HEADER	Header;
	
// PDU

// TODO - need to decide if this message should include any "announcement" parameters (e.g. Master name, capabilities etc).
};






//
// This message is used to test network connectivity, to get RF signal level etc
//
struct RMESSAGE_PING
{
//  Header
	RMESSAGE_HEADER	Header;

// PDU
	uint32_t	cookie;
};

// Extended FCode - PING_REPLY
struct RMESSAGE_PING_REPLY
{
//  Header
	RMESSAGE_HEADER	Header;

// PDU
	uint32_t	cookie;
};

// Time broadcast
struct RMESSAGE_TIME_BROADCAST
{
//  Header
	RMESSAGE_HEADER	Header;

// PDU

	uint32_t	timeNow;
};


// Station types
//
// Initial model - supports Sensors and Valves
#define	STATION_TYPE1	1

#define	DEFAULT_STATION_TYPE		STATION_TYPE1

// Hardware and firmware versions are defined in core.h

// Station status (RProtocol-compliant) definitions

#define	RPROTOCOL_STATUS_ENABLED	1
#define	RPROTOCOL_STATUS_DISABLED	0

//
//  registers map for Remote station 
//
// Holding registers represent system configuration and state
// Note: here we define addresses (actual holding registers are numbered from 40000)

#define MREGISTER_STATION_TYPE					0
#define MREGISTER_HARDWARE_VERSION				1
#define MREGISTER_FIRMWARE_VERSION				2
#define MREGISTER_MAX_ZONES						3
#define MREGISTER_MAX_TEMP_SENSORS				4
#define MREGISTER_MAX_HUMIDITY_SENSORS			5
#define MREGISTER_MAX_PRESSURE_SENSORS			6
#define MREGISTER_SLAVE_ID						10
#define MREGISTER_STATION_STATUS				11
#define MREGISTER_PAN_ID						12
#define MREGISTER_TIMEDATE_LOW					14
#define MREGISTER_TIMEDATE_HIGH					16
#define MREGISTER_MAX_DURATION					18
#define MREGISTER_CYCLE_TEMP					19
#define MREGISTER_CYCLE_HUMIDITY				20
#define MREGISTER_CYCLE_PRESSURE				21

#define  MREGISTER_ZONE_COUNTDOWN				(MREGISTER_CYCLE_PRESSURE+1)

// Note: MODBUSMAP_SYSTEM_MAX is defined in the Slave header file

// Sensors to input registers mappings

#define	MREGISTER_TEMP_SENSOR1					0
#define	MREGISTER_HUMIDITY_SENSOR1				1

// Note: additional sensors can be defined in subsequent registers, and the whole area will need to be extended (below)

//  number of sensors 
#define	MODBUSMAP_SENSORS_MAX	2


#endif  //_SGRPROTOCOLDEF_h
