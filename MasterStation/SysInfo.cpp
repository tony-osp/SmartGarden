// SysInfo.cpp
/*
        Diagnostic information dump for SmartGarden

This module dumps system configuration and other information as an HTML page.



Creative Commons Attribution-ShareAlike 3.0 license
Copyright 2014 tony-osp (http://tony-osp.dreamwidth.org/)


*/

#define __STDC_FORMAT_MACROS
#include "port.h"
#include "settings.h"
#include "XBeeRF.h"


// Main SysInfo function
// 
// Input	- FILE *stream_file, which is expected to be connected to the WEB client socket.
//
// Output	-	true on success, false on failure
//
// When this function is called the socket is already open, and standard HTTP headers are already emitted.
//
bool SysInfo(FILE* stream_file)
{
	// print page header
	fprintf_P( stream_file, PSTR("<html>\n<head>\n<title>SmartGarden SysInfo</title>\n"
		"<style type=\"text/css\">\n"
			".auto-style1 {\n"
				"color: #0066CC;\n"
			"}\n"
			".auto-style2 {\n"
	            "font-weight:bold;\n"
		    "}\n"
			".auto-style3 {\n"
				"text-align: center;\n"
			"}\n"
			"table.style1 {\n"
				"text-align: center;\n"
				"align:center;\n"
				"border: medium;\n"
			"}\n"
		"</style>\n</head>\n"
	"<body>\n"
	"<h2 style=\"text-align: center\" class=\"auto-style1\">SmartGarden SysInfo</h2>\n"
	"<h3 style=\"text-align: center\" class=\"auto-style1\">System Information Dump</h3>\n"
    "<div style=\"text-align: center\">"));

	fprintf_P( stream_file, PSTR("<h3 class=\"auto-style1\">MCU</h3>\n"));
	fprintf_P( stream_file, PSTR("<p>Arduino Mega 2560</p>\n"));

	fprintf_P( stream_file, PSTR("<table align=\"center\" border=\"1\" style=\"border:medium\"><tr>\n<td width=\"200\">Free RAM</td>\n"));
	fprintf_P( stream_file, PSTR("<td>%d bytes (out of 8KB total)</td>\n"), GetFreeMemory());

	fprintf_P( stream_file, PSTR("</tr><tr>\n<td>Network</td>\n<td>Ethernet W5100 (100/10 Mbps)</td>\n</tr><tr>\n"
							     "<td>Storage</td>\n<td>MicroSD Card</td>\n</tr><tr>\n<td>Local LCD</td>\n<td>16 character X 2 lines</td>\n</tr></table>\n"));

	fprintf_P( stream_file, PSTR("<h3 class=\"auto-style1\">System</h3>\n"
						         "<table align=\"center\" border=\"1\" style=\"border:medium\"><tr>\n"));
	fprintf_P( stream_file, PSTR("<td width=\"200\">Zip</td>\n<td>%lu</td>\n"), GetZip());
	fprintf_P( stream_file, PSTR("</tr><tr>\n<td>NTPOffset</td>\n<td>%i</td>\n"), (int)GetNTPOffset());
	fprintf_P( stream_file, PSTR("</tr><tr>\n<td>SeasonalAdj</td>\n<td>%i</td>\n"), (int)GetSeasonalAdjust());
	fprintf_P( stream_file, PSTR("</tr></table>\n"));

	fprintf_P( stream_file, PSTR("<h3 class=\"auto-style1\">Network</h3>\n"
								 "<table align=\"center\" border=\"1\" style=\"border:medium\"><tr>\n<td width=\"200\">IP</td>\n"));

	IPAddress ip;
	ip = GetIP();
	fprintf_P( stream_file, PSTR("<td>%i.%i.%i.%i</td>\n"), ip[0], ip[1], ip[2], ip[3]);
	ip = GetNetmask();
	fprintf_P( stream_file, PSTR("</tr><tr>\n<td>NetMask</td>\n<td>%i.%i.%i.%i</td>\n"), ip[0], ip[1], ip[2], ip[3]);
	ip = GetGateway();
	fprintf_P( stream_file, PSTR("</tr><tr>\n<td>Gateway</td>\n<td>%i.%i.%i.%i</td>\n"), ip[0], ip[1], ip[2], ip[3]);
	fprintf_P( stream_file, PSTR("</tr><tr>\n<td>WebPort</td>\n<td>%i</td>\n"), GetWebPort());
	ip = GetNTPIP();
	fprintf_P( stream_file, PSTR("</tr><tr>\n<td>NTP Server</td>\n<td>%i.%i.%i.%i</td>\n</tr></table>"), ip[0], ip[1], ip[2], ip[3]);

	fprintf_P( stream_file, PSTR("<h3 class=\"auto-style1\">Local Channels</h3>\n"
								 "<h4>Parallel Interface</h4>\n"
								 "<table align=\"center\" border=\"1\" style=\"border:medium\"><tr>\n"));

	fprintf_P( stream_file, PSTR("<td width=\"200\">Num. Channels</td>\n<td>%i</td>\n"), (int)GetNumIOChannels());
	fprintf_P( stream_file, PSTR("</tr><tr>\n<td>Polarity</td>\n"));
	if( GetOT() == OT_DIRECT_NEG )
		fprintf_P( stream_file, PSTR("<td>Negative</td>\n"));
	else if(GetOT() == OT_DIRECT_POS )
		fprintf_P( stream_file, PSTR("<td>Positive</td>\n"));
	else
		fprintf_P( stream_file, PSTR("<td>Not defined</td>\n"));

	fprintf_P( stream_file, PSTR("</tr><tr>\n<td>PINs</td>\n<td>\n"));
	for( uint8_t i=0; i<GetNumIOChannels(); i++)
		fprintf_P( stream_file, PSTR("%S%i"), i?PSTR(","):PSTR(""), (int)GetDirectIOPin(i));

	fprintf_P( stream_file, PSTR("</td>\n</tr></table>\n"));

	fprintf_P( stream_file, PSTR("<h4>Serial Interface</h4>\n<table align=\"center\" border=\"1\" style=\"border:medium\"><tr>\n<td width=\"200\">Num. Channels</td>\n"));
	fprintf_P( stream_file, PSTR("<td>%i</td>\n"), (int)GetNumOSChannels());

	SrIOMapStruct  srIO;
	LoadSrIOMap(&srIO);
	fprintf_P( stream_file, PSTR("</tr><tr>\n<td>SrClkPin</td>\n<td>%i</td>\n"), (int)(srIO.SrClkPin));
	fprintf_P( stream_file, PSTR("</tr><tr>\n<td>SrNoePin</td>\n<td>%i</td>\n"), (int)(srIO.SrNoePin));
	fprintf_P( stream_file, PSTR("</tr><tr>\n<td>SrDatPin</td>\n<td>%i</td>\n"), (int)(srIO.SrDatPin));
	fprintf_P( stream_file, PSTR("</tr><tr>\n<td>SrLatPin</td>\n<td>%i</td>\n</tr></table>\n"), (int)(srIO.SrLatPin));

	fprintf_P( stream_file, PSTR("<h3 class=\"auto-style1\">RF Link</h3>\n"));
	if( IsXBeeEnabled() )
	{
		fprintf_P( stream_file, PSTR("<p>XBee - Enabled</p>\n"));
		fprintf_P( stream_file, PSTR("<table align=\"center\" border=\"1\" style=\"border:medium\"><tr>\n<td width=\"200\">Port</td>\n"));
		fprintf_P( stream_file, PSTR("<td>Serial%i</td>\n</tr><tr>\n<td>Port Speed</td>\n<td>%u</td>\n"), (int)GetXBeePort(), GetXBeePortSpeed());
		fprintf_P( stream_file, PSTR("</tr><tr>\n<td>PAN ID</td>\n<td>%u</td>\n</tr><tr>\n<td>Channel</td>\n<td>%i</td>\n</tr><tr>\n</tr></table>\n"), GetXBeePANID(), (int)GetXBeeChan());
	}
	else
		fprintf_P( stream_file, PSTR("<p>XBee - Disabled</p>\n"));

	fprintf_P( stream_file, PSTR("<h3 class=\"auto-style1\">Stations</h3>\n<p>Number of Stations:&nbsp; %i</p>\n"), (int)GetNumStations());
	fprintf_P( stream_file, PSTR("<table align=\"center\" border=\"1\" style=\"border:medium\"><tr class=\"auto-style2\">\n"
		"<td>&nbsp StationID&nbsp</td><td>&nbsp Name&nbsp</td><td>&nbsp Num Channels&nbsp</td><td>&nbsp NetworkID&nbsp</td><td>&nbsp NetworkAddress&nbsp</td><td>Last Contact</td>\n"
								 "</tr>\n"));

	for( int i=0; i<MAX_STATIONS; i++ )
	{
		FullStation	fStation;
		char		tmp_buf[16];
		LoadStation(i, &fStation);
		if( (fStation.stationFlags & STATION_FLAGS_VALID) && (fStation.stationFlags & STATION_FLAGS_ENABLED) )
		{
			if(      fStation.networkID == NETWORK_ID_LOCAL_PARALLEL )	strcpy_P(tmp_buf, PSTR("Parallel"));
			else if( fStation.networkID == NETWORK_ID_LOCAL_SERIAL )	strcpy_P(tmp_buf, PSTR("Serial (OS)"));
			else if( fStation.networkID == NETWORK_ID_XBEE )			strcpy_P(tmp_buf, PSTR("Remote XBee"));
			else														strcpy_P(tmp_buf, PSTR("Unknown!"));

//			fprintf_P( stream_file, PSTR("<tr class=\"auto-style3\"><td>%i</td><td>%s</td><td>%i</td><td>%s</td><td>%u</td>\n"), i, fStation.name, fStation.numZoneChannels, tmp_buf, fStation.networkAddress);
			fprintf_P( stream_file, PSTR("<tr class=\"auto-style3\"><td>%i</td><td>%s</td><td>%i</td><td>%s</td>"), i, fStation.name, fStation.numZoneChannels, tmp_buf );
			
			if(fStation.networkID == NETWORK_ID_XBEE )
			{
				fprintf_P( stream_file, PSTR("<td>%lX:%lX</td>"), XBeeRF.arpTable[i].MSB,XBeeRF.arpTable[i].LSB);

				if( runState.sLastContactTime[i] != 0 )
				{
					unsigned long c_age = (millis()-runState.sLastContactTime[i]) / (time_t)60000;
					fprintf_P( stream_file, PSTR("<td>%lu min. ago</td></tr>\n"), c_age);
				}
				else
				{
					fprintf_P( stream_file, PSTR("<td>No contact</td></tr>\n"));
				}
			}
			else
			{
				fprintf_P( stream_file, PSTR("<td>%u</td><td>N/A</td></tr>\n"), fStation.networkAddress);
			}
		}
	}

	fprintf_P( stream_file, PSTR("</table>\n"));

	fprintf_P( stream_file, PSTR("<h3 class=\"auto-style1\">Sensors</h3>\n<p>Number of Sensors:&nbsp; %i</p>\n"), (int)GetNumSensors());

	fprintf_P( stream_file, PSTR("<table align=\"center\" border=\"1\" style=\"border:medium\"><tr class=\"auto-style2\">\n"
								 "<td>&nbsp Sensor Number&nbsp</td><td>&nbsp&nbsp Sensor Type&nbsp&nbsp</td><td>&nbsp Name&nbsp</td><td>&nbsp Address&nbsp</td>\n"));
	
	for( int i=0; i<GetNumSensors(); i++ )
	{
		FullSensor	fSensor;
		char		tmp_buf[16];
		LoadSensor(i, &fSensor);

		if(      fSensor.sensorType == SENSOR_TYPE_HUMIDITY )		strcpy_P(tmp_buf, PSTR("Humidity"));
		else if( fSensor.sensorType == SENSOR_TYPE_TEMPERATURE )	strcpy_P(tmp_buf, PSTR("Temperature"));
		else if( fSensor.sensorType == SENSOR_TYPE_PRESSURE )		strcpy_P(tmp_buf, PSTR("Air Pressure"));
		else if( fSensor.sensorType == SENSOR_TYPE_WATERFLOW )		strcpy_P(tmp_buf, PSTR("Water Flow"));
		else if( fSensor.sensorType == SENSOR_TYPE_VOLTAGE )		strcpy_P(tmp_buf, PSTR("Voltage"));
		else														strcpy_P(tmp_buf, PSTR("Unknown!"));

		fprintf_P( stream_file, PSTR("</tr><tr class=\"auto-style3\">\n<td>%i</td><td>%s</td><td>%s</td><td>%i:%i</td>\n"), i, tmp_buf, fSensor.name, (int)(fSensor.sensorStationID), (int)(fSensor.sensorChannel));
	}

	fprintf_P( stream_file, PSTR("</tr></table>\n"));

	fprintf_P( stream_file, PSTR("<h3 class=\"auto-style1\">Zones</h3>\n<p>Number of Zones:&nbsp; %i</p>\n"), (int)GetNumZones());

	fprintf_P( stream_file, PSTR("<table align=\"center\" border=\"1\" style=\"border:medium\"><tr class=\"auto-style2\">\n"
								 "<td>&nbsp Zone Number&nbsp</td><td>&nbsp Zone Name&nbsp</td><td>&nbsp Address&nbsp</td><td>&nbsp Enabled&nbsp</td><td>&nbsp Pump&nbsp</td>\n"));

	for( int i=0; i<GetNumZones(); i++ )
	{
		FullZone	fZone;
		char		tmp_buf[16];
		LoadZone(i, &fZone);

		fprintf_P( stream_file, PSTR("</tr><tr class=\"auto-style3\">\n<td>%i</td><td>%s</td><td>%i:%i</td><td>%S</td><td>%S</td>\n"), i+1, fZone.name, (int)(fZone.stationID), (int)(fZone.channel), fZone.bEnabled? PSTR("Yes"):PSTR("No"), fZone.bPump? PSTR("Yes"):PSTR("No"));
	}
	fprintf_P( stream_file, PSTR("</tr></table>\n"));

	fprintf_P( stream_file, PSTR("<p><br><b>(c) 2015 Tony-osp</b></p></div>\n</body>\n</html>\n"));


}