// web.cpp
// This file manages the web server for the Sprinkler System
// Author: Richard Zimmerman
// Copyright (c) 2013 Richard Zimmerman
//
//
// Modifications for multi-station SmartGarden system, sensors support and optimizations by Tony-osp
//
#include "web.h"
#include "settings.h"
#ifdef ARDUINO
#include "nntp.h"
#endif

#include "Weather.h"
#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include "sensors.h"


bool SysInfo(FILE* stream_file);


web::web(void)
		: m_server(0)
{
}

web::~web(void)
{
	if (m_server)
		delete m_server;
	m_server = 0;
}

bool web::Init()
{
	uint16_t port = GetWebPort();
	if ((port > 65000) || (port < 80))
		port = 80;
	TRACE_INFO(F("Listening on Port %u\n"), port);
	m_server = new EthernetServer(port);
#ifdef ARDUINO
	m_server->begin();
	return true;
#else
	return m_server->begin();
#endif
}

static char sendbuf[512];

#ifdef ARDUINO
static char * sendbufptr;
static inline void setup_sendbuf()
{
	sendbufptr = sendbuf;
}

static int flush_sendbuf(EthernetClient & client)
{
	int ret = 0;
	if (sendbufptr > sendbuf)
	{
		ret = client.write((uint8_t*)sendbuf, sendbufptr-sendbuf);
		setup_sendbuf();
	}
	return ret;
}

static int stream_putchar(char c, FILE *stream)
{
	if (sendbufptr >= sendbuf + sizeof(sendbuf))
	{
		int send_len = ((EthernetClient*)(stream->udata))->write((uint8_t*)sendbuf, sizeof(sendbuf));
		if (!send_len)
		return 0;
		setup_sendbuf();
	}
	*(sendbufptr++) = c;
	return 1;
}
#endif


void ServeHeader(FILE * stream_file, int code, const char * pReason, bool cache, char * type)
{
	fprintf_P(stream_file, PSTR("HTTP/1.1 %d %S\nContent-Type: %S\nConnection: close\n"), code, pReason, type);
	if (cache)
		fprintf_P(stream_file, PSTR("Last-Modified: Fri, 02 Jun 2006 09:46:32 GMT\nExpires: Sun, 17 Jan 2038 19:14:07 GMT\r\n\r\n"));
	else
		fprintf_P(stream_file, PSTR("Cache-Control: no-cache\r\n\r\n"));
}

void ServeHeader(FILE * stream_file, int code, const char * pReason, bool cache)
{
     ServeHeader(stream_file, code, pReason, cache, PSTR("text/html"));
}




void Serve404(FILE * stream_file)
{
	ServeHeader(stream_file, 404, PSTR("NOT FOUND"), false);
	fprintf_P(stream_file, PSTR("NOT FOUND"));
}

static void ServeError(FILE * stream_file)
{
	ServeHeader(stream_file, 405, PSTR("NOT ALLOWED"), false);
	fprintf_P(stream_file, PSTR("NOT ALLOWED"));
}


static void JSONSchedules(const KVPairs & key_value_pairs, FILE * stream_file)
{
	ServeHeader(stream_file, 200, PSTR("OK"), false, PSTR("text/plain"));
	int iNumSchedules = GetNumSchedules();
	fprintf_P(stream_file, PSTR("{\n\"Table\" : [\n"));
	Schedule sched;
	for (int i = 0; i < iNumSchedules; i++)
	{
		LoadSchedule(i, &sched);
		fprintf_P(stream_file, PSTR("%s\t{\"id\" : %d, \"name\" : \"%s\", \"e\" : \"%s\" }"), (i == 0) ? "" : ",\n", i, sched.name,
				(sched.IsEnabled()) ? "on" : "off");
	}
	fprintf_P(stream_file, PSTR("\n]}"));
}


static void JSONZones(const KVPairs & key_value_pairs, FILE * stream_file)
{
	ServeHeader(stream_file, 200, PSTR("OK"), false, PSTR("text/plain"));
	fprintf_P(stream_file, PSTR("{\n\"zones\" : [\n"));
	FullZone zone = {0};
	for (int i = 0; i < GetNumZones(); i++)
	{
		LoadZone(i, &zone);
		fprintf_P(stream_file, PSTR("%s\t{\"name\" : \"%s\", \"enabled\" : \"%s\", \"state\" : \"%s\", \"loc\" : \"%d:%d\", \"wfrate\" : \"%d\" }"), (i == 0) ? "" : ",\n", zone.name,
				zone.bEnabled ? "on" : "off", (GetZoneState(i + 1)==ZONE_STATE_OFF) ? "off" : "on", int(zone.stationID), int(zone.channel), zone.waterFlowRate);
	}
	fprintf_P(stream_file, PSTR("\n]}"));
}


static void JSONSensorsNow(FILE * stream_file)
{
	ServeHeader(stream_file, 200, PSTR("OK"), false, PSTR("text/plain"));

	fprintf_P(stream_file, PSTR("{\n"));
	sensorsModule.TableLastSensorsData(stream_file);
	fprintf_P(stream_file, PSTR("}"));

}


static void JSONWWCounters(const KVPairs & key_value_pairs, FILE * stream_file)
{
	ServeHeader(stream_file, 200, PSTR("OK"), false, PSTR("text/plain"));

	uint8_t		dow = weekday(now())-1;
	uint8_t		index = dow;
	uint16_t	cc;
	bool		bFirstRow = true;

	fprintf_P(stream_file, PSTR("{\n\"series\" : [{\n\t\"name\": \"Water usage\",\n\t\"data\": ["));

	for( uint8_t i=0; i<7; i++ )
	{
		cc = GetWWCounter(index);
		if( index > 0 ) index--;
		else			index = 6;

		fprintf_P(stream_file, PSTR("%s\n\t\t\t[-%d, %u]"), bFirstRow ? "":",", int(i), cc/100);
		bFirstRow = false;
	}

	fprintf_P(stream_file, PSTR("\n\t\t]\n\t}]\n}\n"));
}


// Query sensor readings

static void JSONSensor(const KVPairs & key_value_pairs, FILE * stream_file)
{
	ServeHeader(stream_file, 200, PSTR("OK"), false, PSTR("text/plain"));
	fprintf_P(stream_file, PSTR("{\n"));

	time_t sdate = 0;
	time_t edate = 0;
        char  sensor_type = 0;
        int     sensor_id     = 0;
        char  summary_type = LOG_SUMMARY_NONE;

	// Iterate through the kv pairs and search for the start and end dates.
	for (int i = 0; i < key_value_pairs.num_pairs; i++)
	{
		const char * key = key_value_pairs.keys[i];
		const char * value = key_value_pairs.values[i];
		if (strcmp_P(key, PSTR("sdate")) == 0)
		{
			sdate = strtol(value, 0, 10);
		}
		else if (strcmp_P(key, PSTR("edate")) == 0)
		{
			edate = strtol(value, 0, 10);
		}
		else if (strcmp_P(key, PSTR("type")) == 0)
		{
			sensor_type = atoi(value);
		}
		else if (strcmp_P(key, PSTR("id")) == 0)
		{
			sensor_id = atoi(value);
		}
		else if (strcmp_P(key, PSTR("sum")) == 0)
		{
			if (value[0] == 'd')
				summary_type = LOG_SUMMARY_DAY;
			else if (value[0] == 'h')
				summary_type = LOG_SUMMARY_HOUR;
			else if (value[0] == 'm')
				summary_type = LOG_SUMMARY_MONTH;
		}
	}

	sdlog.EmitSensorLog(stream_file, sdate, edate, sensor_type, sensor_id, summary_type);
	fprintf_P(stream_file, PSTR("}"));
}


static void JSONtLogs(const KVPairs & key_value_pairs, FILE * stream_file)
{
	ServeHeader(stream_file, 200, PSTR("OK"), false, PSTR("text/plain"));
	time_t sdate = 0;
	time_t edate = 0;
	// Iterate through the kv pairs and search for the start and end dates.
	for (int i = 0; i < key_value_pairs.num_pairs; i++)
	{
		const char * key = key_value_pairs.keys[i];
		const char * value = key_value_pairs.values[i];
		if (strcmp_P(key, PSTR("sdate")) == 0)
		{
			sdate = strtol(value, 0, 10);
		}
		else if (strcmp_P(key, PSTR("edate")) == 0)
		{
			edate = strtol(value, 0, 10);
		}
	}
	sdlog.TableZone(stream_file, sdate, edate);
}

static void JSONScheduleLogs(const KVPairs & key_value_pairs, FILE * stream_file)
{
	ServeHeader(stream_file, 200, PSTR("OK"), false, PSTR("text/plain"));
	fprintf_P(stream_file, PSTR("{\n\t\"logs\": [\n"));
	time_t sdate = 0;
	time_t edate = 0;
	// Iterate through the kv pairs and search for the start and end dates.
	for (int i = 0; i < key_value_pairs.num_pairs; i++)
	{
		const char * key = key_value_pairs.keys[i];
		const char * value = key_value_pairs.values[i];
		if (strcmp_P(key, PSTR("sdate")) == 0)
		{
			sdate = strtol(value, 0, 10);
		}
		else if (strcmp_P(key, PSTR("edate")) == 0)
		{
			edate = strtol(value, 0, 10);
		}
	}
	sdlog.TableSchedule(stream_file, sdate, edate);
	fprintf_P(stream_file, PSTR("\n\t]\n}"));
}

static void JSONSettings(const KVPairs & key_value_pairs, FILE * stream_file)
{
	ServeHeader(stream_file, 200, PSTR("OK"), false, PSTR("text/plain"));
	IPAddress ip;
	fprintf_P(stream_file, PSTR("{\n"));
#ifdef ARDUINO
	ip = GetIP();
	fprintf_P(stream_file, PSTR("\t\"ip\" : \"%d.%d.%d.%d\",\n"), ip[0], ip[1], ip[2], ip[3]);
	ip = GetNetmask();
	fprintf_P(stream_file, PSTR("\t\"netmask\" : \"%d.%d.%d.%d\",\n"), ip[0], ip[1], ip[2], ip[3]);
	ip = GetGateway();
	fprintf_P(stream_file, PSTR("\t\"gateway\" : \"%d.%d.%d.%d\",\n"), ip[0], ip[1], ip[2], ip[3]);
	ip = GetNTPIP();
	fprintf_P(stream_file, PSTR("\t\"NTPip\" : \"%d.%d.%d.%d\",\n"), ip[0], ip[1], ip[2], ip[3]);
	fprintf_P(stream_file, PSTR("\t\"NTPoffset\" : \"%d\",\n"), GetNTPOffset());
#endif
	fprintf_P(stream_file, PSTR("\t\"webport\" : \"%u\",\n"), GetWebPort());
	fprintf_P(stream_file, PSTR("\t\"ot\" : \"%d\",\n"), GetOT());
	ip = GetWUIP();
	fprintf_P(stream_file, PSTR("\t\"wuip\" : \"%d.%d.%d.%d\",\n"), ip[0], ip[1], ip[2], ip[3]);
	fprintf_P(stream_file, PSTR("\t\"wutype\" : \"%s\",\n"), GetUsePWS() ? "pws" : "zip");
	fprintf_P(stream_file, PSTR("\t\"zip\" : \"%ld\",\n"), (long) GetZip());
	fprintf_P(stream_file, PSTR("\t\"sadj\" : \"%ld\",\n"), (long) GetSeasonalAdjust());
	char ak[17];
	GetApiKey(ak);
	fprintf_P(stream_file, PSTR("\t\"apikey\" : \"%s\",\n"), ak);
	GetPWS(ak);
	ak[11] = 0;
	fprintf_P(stream_file, PSTR("\t\"pws\" : \"%s\"\n"), ak);
	fprintf_P(stream_file, PSTR("}"));
}

static void JSONwCheck(const KVPairs & key_value_pairs, FILE * stream_file)
{
	Weather w;
	ServeHeader(stream_file, 200, PSTR("OK"), false, PSTR("text/plain"));
	char key[17];
	GetApiKey(key);
	char pws[12] = {0};
	GetPWS(pws);

	const Weather::ReturnVals vals = w.GetVals(GetWUIP(), key, GetZip(), pws, GetUsePWS());
	const int scale = w.GetScale(vals);

	TRACE_VERBOSE(F("JSONwCheck - GetVals complete. Scale=%d\n"), scale);

	fprintf_P(stream_file, PSTR("{\n"));
	fprintf_P(stream_file, PSTR("\t\"valid\" : \"%s\",\n"), vals.valid ? "true" : "false");
	fprintf_P(stream_file, PSTR("\t\"keynotfound\" : \"%s\",\n"), vals.keynotfound ? "true" : "false");
	fprintf_P(stream_file, PSTR("\t\"minhumidity\" : \"%d\",\n"), vals.minhumidity);
	fprintf_P(stream_file, PSTR("\t\"maxhumidity\" : \"%d\",\n"), vals.maxhumidity);
	fprintf_P(stream_file, PSTR("\t\"meantempi\" : \"%d\",\n"), vals.meantempi);
	fprintf_P(stream_file, PSTR("\t\"precip_today\" : \"%d\",\n"), vals.precip_today);
	fprintf_P(stream_file, PSTR("\t\"precip\" : \"%d\",\n"), vals.precipi);
	fprintf_P(stream_file, PSTR("\t\"wind_mph\" : \"%d\",\n"), vals.windmph);
	fprintf_P(stream_file, PSTR("\t\"UV\" : \"%d\",\n"), vals.UV);
	fprintf_P(stream_file, PSTR("\t\"scale\" : \"%d\"\n"), scale);
	fprintf_P(stream_file, PSTR("}"));
}

static void JSONState(const KVPairs & key_value_pairs, FILE * stream_file)
{
	ServeHeader(stream_file, 200, PSTR("OK"), false, PSTR("text/plain"));

	fprintf_P(stream_file,
			PSTR("{\n\t\"version\" : \"%s\",\n\t\"run\" : \"%s\",\n\t\"zones\" : \"%d\",\n\t\"schedules\" : \"%d\",\n\t\"stations\" : \"%d\",\n\t\"timenow\" : \"%lu\",\n\t\"locationZip\" : \"%lu\","),
			VERSION, GetRunSchedules() ? "on" : "off", GetNumEnabledZones(), int(GetNumSchedules()), int(GetNumStations()), now(), GetZip());
	
	if( runState.isPaused() )
	{
		fprintf_P(stream_file, PSTR("\n\t\"paused\" : \"on\",\n\t\"remainingPauseTime\" : \"%d\""), runState.getRemainingPauseTime());
	}
	else
	{
		fprintf_P(stream_file, PSTR("\n\t\"paused\" : \"off\""));
	}
	
	if( runState.isSchedule() )
	{
		FullZone zone;
		LoadZone(runState.getZone() - 1, &zone);
        Schedule sched;
		LoadSchedule(runState.getSchedule(), &sched);

		if( runState.getSchedule() == 100 )  // manual
			strcpy_P(sched.name, PSTR("Manual"));
		fprintf_P(stream_file, PSTR(",\n\t\"onZoneName\" : \"%s\",\n\t\"offTime\" : \"%d\",\n\t\"onSchedID\" : \"%d\",\n\t\"onSchedName\" : \"%s\""), zone.name, runState.getRemainingTime(), int(runState.getSchedule()), sched.name);
	}

	uint8_t	 nextSchedID, nextZoneID;
	short	 nextTime;

	if( GetNextEvent(&nextSchedID, &nextZoneID, &nextTime) )
	{
		short nextHour, nextMinute;
		nextHour = nextTime/60;
		nextMinute = nextTime - nextHour*60;

        Schedule sched;
        LoadSchedule( nextSchedID, &sched );
		FullZone zone;
		LoadZone(nextZoneID, &zone);

		fprintf_P(stream_file, PSTR(",\n\t\"nextSchedID\" : \"%u\",\n\t\"nextSchedName\" : \"%s\",\n\t\"nextZoneID\" : \"%u\",\n\t\"nextZoneName\" : \"%s\",\n\t\"NextEventTime\" : \"%2.2u:%2.2u\""), 
									short(nextSchedID), sched.name, short(nextZoneID), zone.name, nextHour, nextMinute);
	}

	fprintf_P(stream_file, (PSTR("\n}")));
}

static void JSONSchedule(const KVPairs & key_value_pairs, FILE * stream_file)
{
	int sched_num = -1;
	freeMemory();

	// Iterate through the kv pairs and search for the id.
	for (int i = 0; i < key_value_pairs.num_pairs; i++)
	{
		const char * key = key_value_pairs.keys[i];
		const char * value = key_value_pairs.values[i];
		if (strcmp_P(key, PSTR("id")) == 0)
		{
			sched_num = atoi(value);
		}
	}

	// Now check to see if the id is in range.
	const uint8_t numSched = GetNumSchedules();
	if ((sched_num < 0) || (sched_num >= numSched))
	{
		ServeError(stream_file);
		return;
	}

	// Now construct the response and send it
	ServeHeader(stream_file, 200, PSTR("OK"), false, PSTR("text/plain"));
	Schedule sched;
	LoadSchedule(sched_num, &sched);
	fprintf_P(stream_file,
			PSTR("{\n\t\"name\" : \"%s\",\n\t\"enabled\" : \"%s\",\n\t\"wadj\" : \"%s\",\n\t\"type\" : \"%s\",\n\t\"d1\" : \"%s\",\n\t\"d2\" : \"%s\""),
			sched.name, sched.IsEnabled() ? "on" : "off", sched.IsWAdj() ? "on" : "off", sched.IsInterval() ? "off" : "on", sched.day & 0x01 ? "on" : "off",
			sched.day & 0x02 ? "on" : "off");
	fprintf_P(stream_file,
			PSTR(",\n\t\"d3\" : \"%s\",\n\t\"d4\" : \"%s\",\n\t\"d5\" : \"%s\",\n\t\"d6\" : \"%s\",\n\t\"d7\" : \"%s\",\n\t\"interval\" : \"%d\",\n\t\"times\" : [\n"),
			sched.day & 0x04 ? "on" : "off", sched.day & 0x08 ? "on" : "off", sched.day & 0x10 ? "on" : "off", sched.day & 0x20 ? "on" : "off",
			sched.day & 0x40 ? "on" : "off", sched.interval);
	for (int i = 0; i < 4; i++)
	{
		if (sched.time[i] == -1)
		{
			fprintf_P(stream_file, PSTR("%s\t\t{\"t\" : \"00:00\", \"e\" : \"off\" }"), (i == 0) ? "" : ",\n");
		}
		else
		{
			fprintf_P(stream_file, PSTR("%s\t\t{\"t\" : \"%02d:%02d\", \"e\" : \"on\" }"), (i == 0) ? "" : ",\n", sched.time[i] / 60, sched.time[i] % 60);
		}
	}
	fprintf_P(stream_file, PSTR("\n\t],\n\t\"zones\" : [\n"));
	for (int i = 0; i < GetNumZones(); i++)
	{
		FullZone zone;
		LoadZone(i, &zone);
		fprintf_P(stream_file, PSTR("%s\t\t{\"name\" : \"%s\", \"e\":\"%s\", \"duration\" : %d}"), (i == 0) ? "" : ",\n", zone.name, zone.bEnabled ? "on" : "off",
				sched.zone_duration[i]);
	}
	fprintf_P(stream_file, PSTR(" ]\n}"));
}

static bool SetQSched(const KVPairs & key_value_pairs)
{

	// So, we first end any schedule that's currently running by turning things off then on again.
	runState.StopSchedule();

	int sched = -1;

	// Iterate through the kv pairs and update the appropriate structure values.
	for (int i = 0; i < key_value_pairs.num_pairs; i++)
	{
		const char * key = key_value_pairs.keys[i];
		const char * value = key_value_pairs.values[i];
		if ((key[0] == 'z') && (key[1] > 'a') && (key[1] <= ('a' + GetNumZones())) && (key[2] == 0))
		{
			quickSchedule.zone_duration[key[1] - 'b'] = atoi(value);
		}
		if (strcmp_P(key, PSTR("sched")) == 0)
		{
			sched = atoi(value);
		}
	}

	if (sched == -1)
		runState.StartSchedule(true);
	else
		runState.StartSchedule(false, sched);

	return true;
}


static void ServeSysInfoPage(FILE * stream_file)
{
	ServeHeader(stream_file, 200, PSTR("OK"), false);
	freeMemory();
	SysInfo(stream_file);
}




static bool RunSchedules(const KVPairs & key_value_pairs)
{
	// Iterate through the kv pairs and update the appropriate structure values.
	for (int i = 0; i < key_value_pairs.num_pairs; i++)
	{
		const char * key = key_value_pairs.keys[i];
		const char * value = key_value_pairs.values[i];
		if (strcmp_P(key, PSTR("system")) == 0)
		{
			SetRunSchedules(strcmp_P(value, PSTR("on")) == 0);
		}
		else if (strcmp_P(key, PSTR("pause")) == 0)
		{
			 int  time2pause = 0;
			 time2pause = atoi(value);
			 runState.SetPause(time2pause);
		}
	}
	return true;
}


void ServeFile(FILE * stream_file, const char * fname, SdFile & theFile, EthernetClient & client)
{
	freeMemory();
	const char * ext;
	for (ext=fname + strlen(fname); ext>fname; ext--)
		if (*ext == '.')
		{
			ext++;
			break;
		}
	if (ext > fname)
	{
		if (strcmp_P(ext, PSTR("htm")) == 0)                    // accelerate checks for common case - HTML
			ServeHeader(stream_file, 200, PSTR("OK"), true);
		else if (strcmp_P(ext, PSTR("js")) == 0)
			ServeHeader(stream_file, 200, PSTR("OK"), true, PSTR("application/javascript"));
		else if (strcmp_P(ext, PSTR("jpg")) == 0)
			ServeHeader(stream_file, 200, PSTR("OK"), true, PSTR("image/jpeg"));
		else if (strcmp_P(ext, PSTR("gif")) == 0)
			ServeHeader(stream_file, 200, PSTR("OK"), true, PSTR("image/gif"));
		else if (strcmp_P(ext, PSTR("css")) == 0)
			ServeHeader(stream_file, 200, PSTR("OK"), true, PSTR("text/css"));
		else if (strcmp_P(ext, PSTR("ico")) == 0)
			ServeHeader(stream_file, 200, PSTR("OK"), true, PSTR("image/x-icon"));
		else if ( (strcmp_P(ext, PSTR("log")) == 0) || (strcmp_P(ext, PSTR("LOG")) == 0))
			ServeHeader(stream_file, 200, PSTR("OK"), false, PSTR("text/plain"));
		else if ( ext[0] >= '0' && ext[0] <= '9')
			ServeHeader(stream_file, 200, PSTR("OK"), false, PSTR("text/plain"));
		else
			ServeHeader(stream_file, 200, PSTR("OK"), true);
	}
	else
		ServeHeader(stream_file, 200, PSTR("OK"), true);


#ifdef ARDUINO
	flush_sendbuf(client);
#else
	fflush(stream_file);
#endif
	while (theFile.available())
	{
		int bytes = theFile.read(sendbuf, 512);
		if (bytes <= 0)
			break;
		client.write((uint8_t*) sendbuf, bytes);
	}
}

// change a character represented hex digit (0-9, a-f, A-F) to the numeric value
static inline char hex2int(const char ch)
{
	if (ch < 48)
		return 0;
	else if (ch <=57) // 0-9
		return (ch - 48);
	else if (ch < 65)
		return 0;
	else if (ch <=70) // A-F
		return (ch - 55);
	else if (ch < 97)
		return 0;
	else if (ch <=102) // a-f
		return ch - 87;
	else
		return 0;
}

//  Pass in a connected client, and this function will parse the HTTP header and return the requested page 
//   and a KV pairs structure for the variable assignments.
static bool ParseHTTPHeader(EthernetClient & client, KVPairs * key_value_pairs, char * sPage, int iPageSize)
{
	enum
	{
		INITIALIZED = 0, PARSING_PAGE, PARSING_KEY, PARSING_VALUE, PARSING_VALUE_PERCENT, PARSING_VALUE_PERCENT1, LOOKING_FOR_BLANKLINE, FOUND_BLANKLINE, DONE, ERROR
	} current_state = INITIALIZED;
	// an http request ends with a blank line
	static const char get_text[] = "GET /";
	const char * gettext_ptr = get_text;
	char * page_ptr = sPage;
	key_value_pairs->num_pairs = 0;
	char * key_ptr = key_value_pairs->keys[0];
	char * value_ptr = key_value_pairs->values[0];
	char recvbuf[100];  // note:  trial and error has shown that it doesn't help to increase this number.. few ms at the most.
	char * recvbufptr = recvbuf;
	char * recvbufend = recvbuf;
	while (true)
	{
		if (recvbufptr >= recvbufend)
		{
			int len = client.read((uint8_t*) recvbuf, sizeof(recvbuf));
			if (len <= 0)
			{
				if (!client.connected())
					break;
				else
					continue;
			}
			else
			{
				recvbufptr = recvbuf;
				recvbufend = recvbuf + len;
			}
		}
		char c = *(recvbufptr++);
		//Serial.print(c);

		switch (current_state)
		{
		case INITIALIZED:
			if (c == *gettext_ptr)
			{
				gettext_ptr++;
				if (gettext_ptr - get_text >= (long)sizeof(get_text) - 1)
				{
					current_state = PARSING_PAGE;
				}
			}

			break;
		case PARSING_PAGE:
			if (c == '?')
			{
				*page_ptr = 0;
				current_state = PARSING_KEY;
			}
			else if (c == ' ')
			{
				*page_ptr = 0;
				current_state = LOOKING_FOR_BLANKLINE;
			}
			else if (c == '\n')
			{
				*page_ptr = 0;
				current_state = FOUND_BLANKLINE;
			}
			else if ((c > 32) && (c < 127))
			{
				if (page_ptr - sPage >= iPageSize - 1)
				{
					current_state = ERROR;
				}
				else
					*page_ptr++ = c;
			}
			break;
		case PARSING_KEY:
			if (c == ' ')
				current_state = LOOKING_FOR_BLANKLINE;
			else if (c == '\n')
			{
				current_state = FOUND_BLANKLINE;
			}
			else if (c == '&')
			{
				current_state = ERROR;
			}
			else if (c == '=')
			{
				*key_ptr = 0;
				current_state = PARSING_VALUE;
			}
			else if ((c > 32) && (c < 127))
			{
				if (key_ptr - key_value_pairs->keys[key_value_pairs->num_pairs] >= KEY_SIZE - 1)
				{
					current_state = ERROR;
				}
				else
					*key_ptr++ = c;
			}
			break;
		case PARSING_VALUE:
		case PARSING_VALUE_PERCENT:
		case PARSING_VALUE_PERCENT1:
			if ((c == ' ') || c == '&')
			{
				*value_ptr = 0;
				TRACE_VERBOSE(F("Found a KV pair : %s -> %s\n"), key_value_pairs->keys[key_value_pairs->num_pairs], key_value_pairs->values[key_value_pairs->num_pairs]);

				if ((c == '&') && (key_value_pairs->num_pairs >= NUM_KEY_VALUES - 1))
				{

// drop KV pairs that exceed our buffers (necessary for large number of zones support)

					return true;

					current_state = ERROR;
					break;
				}
				if (c == '&')
				{
					key_value_pairs->num_pairs++;
					key_ptr = key_value_pairs->keys[key_value_pairs->num_pairs];
					value_ptr = key_value_pairs->values[key_value_pairs->num_pairs];
					current_state = PARSING_KEY;
				}
				else
				{
					key_value_pairs->num_pairs++;
					current_state = LOOKING_FOR_BLANKLINE;
				}
				break;
			}
			else if ((c > 32) && (c < 127))
			{
				if (value_ptr - key_value_pairs->values[key_value_pairs->num_pairs] >= VALUE_SIZE - 1)
				{
					current_state = ERROR;
					break;
				}
				switch (current_state)
				{
				case PARSING_VALUE_PERCENT:
					if (isxdigit(c))
					{
						*value_ptr = hex2int(c) << 4;
						current_state = PARSING_VALUE_PERCENT1;
					}
					else
						current_state = PARSING_VALUE;
					break;
				case PARSING_VALUE_PERCENT1:
					if (isxdigit(c))
					{
						*value_ptr += hex2int(c);
						// let's check this value to see if it's legal
						if (((*value_ptr >= 0 ) && (*value_ptr < 32)) || (*value_ptr == 127) || (*value_ptr == '"') || (*value_ptr == '\\'))
							*value_ptr = ' ';
						value_ptr++;
					}
					current_state = PARSING_VALUE;
					break;
				default:
					if (c == '+')
						*value_ptr++ = ' ';
					else if (c == '%')
						current_state = PARSING_VALUE_PERCENT;
					else
						*value_ptr++ = c;
					break;
				}
			}
			else
				current_state = ERROR;
			break;
		case LOOKING_FOR_BLANKLINE:
			if (c == '\n')
				current_state = FOUND_BLANKLINE;
			break;
		case FOUND_BLANKLINE:
			if (c == '\n')
				current_state = DONE;
			else if (c != '\r')
				current_state = LOOKING_FOR_BLANKLINE;
			break;
		default:
			break;
		} // switch
		if (current_state == DONE)
			return true;
		else if (current_state == ERROR)
			return false;
	} // true
	return false;
}

void web::ProcessWebClients()
{
	// listen for incoming clients
	EthernetClient client = m_server->available();
	if (client)
	{
		bool bReset = false;
#ifdef ARDUINO
		FILE stream_file;
		FILE * pFile = &stream_file;
		setup_sendbuf();
		fdev_setup_stream(pFile, stream_putchar, NULL, _FDEV_SETUP_WRITE);
		stream_file.udata = &client;
#else
		FILE * pFile = fdopen(client.GetSocket(), "w");
#endif
		 freeMemory();
		 TRACE_INFO(F("Got a client\n"));
		 //ShowSockStatus();
		 KVPairs key_value_pairs;
		 char sPage[35];

		 if (!ParseHTTPHeader(client, &key_value_pairs, sPage, sizeof(sPage)))
		 {
			SYSEVT_ERROR(F("ERROR!"));
			ServeError(pFile);
		 }
		 else
		 {

			TRACE_INFO(F("Page:%s\n"), sPage);
			//ShowSockStatus();

            if( strncmp_P(sPage, PSTR("bin/"), 4) == 0 )       // We do the check in two phases. 
                                                               // First we check that the URL starts with "bin/" to identify the block of bin requests, 
                                                               // and then we check for a specific request in the bin/ block
                                                               //
                                                               // This optimization speeds up request decoding, allowing to reduce the number of strcmp() 
															   // each request goes through as well as the string lengh to check
                                                               //
            {
                 char *xP4 = sPage + 4;

     			 if (strcmp_P(xP4, PSTR("setSched")) == 0)
			     {
				     if (SetSchedule(key_value_pairs))
				     {
					     ServeHeader(pFile, 200, PSTR("OK"), false);
				     }
				     else
					     ServeError(pFile);
			     }
			     else if (strcmp_P(xP4, PSTR("set1Zone")) == 0)
			     {
				     if (SetOneZones(key_value_pairs))
				     {
					     ServeHeader(pFile, 200, PSTR("OK"), false);
				     }
				     else
					     ServeError(pFile);
			     }
			     else if (strcmp_P(xP4, PSTR("setZones")) == 0)
			     {
				     if (SetZones(key_value_pairs))
				     {
					     ServeHeader(pFile, 200, PSTR("OK"), false);
				     }
				     else
					     ServeError(pFile);
			     }
			     else if (strcmp_P(xP4, PSTR("delSched")) == 0)
  			     {
				     if (DeleteSchedule(key_value_pairs))
				     {
					     if (GetRunSchedules()){
							 runState.StopSchedule();
							 runState.ProcessScheduledEvents();
						 }
					     ServeHeader(pFile, 200, PSTR("OK"), false);
				     }
				     else
					     ServeError(pFile);
			     }
			     else if (strcmp_P(xP4, PSTR("setQSched")) == 0)
			     {
				     if (SetQSched(key_value_pairs))
				     {
					     ServeHeader(pFile, 200, PSTR("OK"), false);
				     }
				     else
					     ServeError(pFile);
			     }
			     else if (strcmp_P(xP4, PSTR("settings")) == 0)
			     {
				     if (SetSettings(key_value_pairs))
				     {
					     if (GetRunSchedules()){
							 runState.StopSchedule();
							 runState.ProcessScheduledEvents();
						 }
					     ServeHeader(pFile, 200, PSTR("OK"), false);
				     }
				     else
					     ServeError(pFile);
			     }
			     else if (strcmp_P(xP4, PSTR("run")) == 0)
			     {
				     if (RunSchedules(key_value_pairs))
				     {
						 runState.ProcessScheduledEvents();
					     ServeHeader(pFile, 200, PSTR("OK"), false);
				     }
				     else
					     ServeError(pFile);
			     }
			     else if (strcmp_P(xP4, PSTR("factory")) == 0)
			     {
				     if (GetRunSchedules()){
						 runState.StopSchedule();
					 }
				     ResetEEPROM();
				     ServeHeader(pFile, 200, PSTR("OK"), false);
			     }
			     else if (strcmp_P(xP4, PSTR("reset")) == 0)
			     {
				     ServeHeader(pFile, 200, PSTR("OK"), false);
				     bReset = true;
			     }
              }
            else if( strncmp_P(sPage, PSTR("json/"), 5) == 0 )      // We do the check in two phases. 
                                                                    // First we check that the URL starts with "json/" to identify the block of json requests, 
                                                                    // and then each request in the block checks the rest of the URL to determine specific request
            {
                 char *xP5 = sPage + 5;
                             
			     if (strcmp_P(xP5, PSTR("schedules")) == 0)
			     {
				     JSONSchedules(key_value_pairs, pFile);
			     }
			     else if (strcmp_P(xP5, PSTR("zones")) == 0)
			     {
				     JSONZones(key_value_pairs, pFile);
			     }
			     else if (strcmp_P(xP5, PSTR("settings")) == 0)
			     {
				     JSONSettings(key_value_pairs, pFile);
			     }
			     else if (strcmp_P(xP5, PSTR("state")) == 0)
			     {
				     JSONState(key_value_pairs, pFile);
			     }
			     else if (strcmp_P(xP5, PSTR("schedule")) == 0)
			     {
				     JSONSchedule(key_value_pairs, pFile);
			     }
			     else if (strcmp_P(xP5, PSTR("wcheck")) == 0)
			     {
				     JSONwCheck(key_value_pairs, pFile);
			     }
			     else if (strcmp_P(xP5, PSTR("tlogs")) == 0)
			     {
				     JSONtLogs(key_value_pairs, pFile);
			     }
			     else if (strcmp_P(xP5, PSTR("schlogs")) == 0)
			     {
				     JSONScheduleLogs(key_value_pairs, pFile);
			     }

// Sensors 
			     else if (strcmp_P(xP5, PSTR("sens")) == 0)
			     {
			 	      JSONSensor(key_value_pairs, pFile);
			     }
			     else if (strcmp_P(xP5, PSTR("sensNow")) == 0)
			     {
					JSONSensorsNow(pFile);
			     }
			     else if (strcmp_P(xP5, PSTR("wCounters")) == 0)
			     {
					JSONWWCounters(key_value_pairs, pFile);
			     }

            }
			// Access sysinfo page
			else if (strncmp_P(sPage, PSTR("SysInfo"), 7) == 0)
			{
				ServeSysInfoPage( pFile );
			}
// access system logs directory
			else if (strncmp_P(sPage, PSTR("logs"), 4) == 0)
			{
  				freeMemory();
				sdlog.LogsHandler(sPage, pFile, client);
			}
			else
// This is the "catch all" case, that also serves static HTML files, *.js etc.
			{
  				if (strlen(sPage) == 0){
    
 			        TRACE_INFO(F("Serving: web root\n"));
					strcpy(sPage, "index.htm");
                }
				// prepend path
				memmove(sPage + 5, sPage, sizeof(sPage) - 5);
				memcpy(sPage, "/web/", 5);
				sPage[sizeof(sPage)-1] = 0;
				TRACE_INFO(F("Serving file: %s\n"), sPage);
				SdFile theFile;
				if (!theFile.open(sPage, O_READ))
					Serve404(pFile);
				else
				{
					if (theFile.isFile())
						ServeFile(pFile, sPage, theFile, client);
					else
						Serve404(pFile);
					theFile.close();
				}
			}
		}

#ifdef ARDUINO
		flush_sendbuf(client);
		// give the web browser time to receive the data
		delay(1);
#else
		fflush(pFile);
		fclose(pFile);
#endif
		// close the connection:
		client.stop();

		if (bReset)
			sysreset();
	}
}

