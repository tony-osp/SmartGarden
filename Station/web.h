// web.h
// This file manages the web server for the Sprinkler System
// Author: Richard Zimmerman
// Copyright (c) 2013 Richard Zimmerman
//
//
// Modifications for multi-station and SmartGarden system by Tony-osp
//
#ifndef _WEB_h
#define _WEB_h

#include <stdlib.h>
#include <stdio.h>
#include <SdFat.h>
#include <Ethernet.h>

class EthernetServer;

#define NUM_KEY_VALUES 60
#define KEY_SIZE 10
#define VALUE_SIZE 20

struct KVPairs
{
	int num_pairs;
	char keys[NUM_KEY_VALUES][KEY_SIZE];
	char values[NUM_KEY_VALUES][VALUE_SIZE];
};

class web
{
public:
	web(void);
	~web(void);
	bool Init();
	void ProcessWebClients();
private:
	EthernetServer * m_server;
};

void ServeHeader(FILE * stream_file, int code, const char * pReason, bool cache, char * type);
void ServeHeader(FILE * stream_file, int code, const char * pReason, bool cache);
void ServeFile(FILE * stream_file, const char * fname, SdFile & theFile, EthernetClient & client);
void Serve404(FILE * stream_file);



#endif
