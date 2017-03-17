# SmartGarden
Multi-station irrigation controller and environment monitoring system. Internet-connected, has WEB UI, and uses RF links for multi-station support.

The system consists of the Master station, and multiple Remote stations. Master station is Internet-connected, and
supports WEB UI, as well as local on-device UI (simple LCD + few buttons). Remote stations have local UI only, but
Remote stations are connected to the Master via RF links, and are controlled by the Master.

Each station (both Master and Remote) can support a number of irrigation zones, as well as various sensors - Temperature,
Humidity, Air Pressure, Waterflow etc. Sensors data is collected and stored on a MicroSD card in the Master, the data
can be visualized in the WEB UI and downloaded (to an Excel) for offline analysis.

Currently the hardware is based on Arduino. Remote stations are running on Arduino Uno-level hardware,
Master station is using Arduino MoteinoMega as the core. XBee is used as RF link. 

The project is complete, I'm using it since 2015 as my home garden irrigation and sensors monitoring system

This project was inspired by OpenSprinklers design, and by the sprinklers_pi control program created by Richard Zimmerman.

This is actually the second iteration of my project. Initially I started with OpenSprinkler, then Sprinklers_pi code
and was gradually adding new funcitonality and features. However at this stage the functionality evolved a lot, and required
complete redesign.

The code of Remote station is written from scratch, Master station currently uses a mix of my own and pieces of sprinklers_pi code.

I have a small blog dedicated to this project: http://tony-osp.dreamwidth.org, and I'm posting updates there as
the project progresses.

Note: to compile the project you need following libraries:

1. IniFile (for Master Station) - https://github.com/stevemarple/IniFile
2. BMP180 support library (if you are using BMP180 sensor) - https://github.com/sparkfun/BMP180_Breakout/tree/master/Libraries/Arduino/src
3. XBee library - https://github.com/andrewrapp/xbee-arduino
4. SdFat library - https://github.com/greiman/SdFat/tree/master/SdFat
5. Time library - https://github.com/PaulStoffregen/Time
6. DHT library (if you are using DHT sensor) - https://github.com/adafruit/DHT-sensor-library
7. W5500 Ethernet library, if you are using W5500-based shield instead of the common W5100 shield
	(W5500 is considerably faster than W5100)

As well as standard Arduino libraries.