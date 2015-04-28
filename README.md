# SmartGarden
Multi-station irrigation controller and environment monitoring system. Internet-connected, has WEB UI, and uses RF links for multi-station support.

The system consists of the Master station, and multiple Remote stations. Master station is Internet-connected, and
supports WEB UI, as well as local on-device UI (simple LCD + few buttons). Remote stations have local UI only, but
Remote stations are connected to the Master via RF links, and are controlled by the Master.

Each station (both Master and Remote) can support a number of irrigation zones, as well as various sensors - Temperature,
Humidity, Air Pressure, Waterflow etc. Sensors data is collected and stored on a MicroSD card in the Master, the data
can be visualized in the WEB UI and downloaded (to an Excel) for offline analysis.

Currently the hardware is based on Arduino. Remote stations are running on Arduino Uno-level hardware,
Master station is using Arduino Mega as the core. XBee is used as RF link. The code itself is portable,
and I'm planning to move Master code to a more powerful controller later this year. In theory Master can run nicely on
RPi, but even better option could be a cheap small tablet - such as HP Stream 7, which includes decent CPU/memory/etc
as well as 7" LCD screen - the whole package is considerably cheaper than RPi + LCD + MicroSD etc.

The project is in development stage, 80% complete. The code works (including multi-station), but some parts
of funcitonality is still not enabled or missing. I'm planning to finish it up to v1.0 level this spring.

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

As well as standard Arduino libraries.