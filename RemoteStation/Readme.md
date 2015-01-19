This is the Remote station code for SmartGarden system.

It is running on Arduino Uno (or better) hardware. XBee is used as the RF link to connect to the master.

Remote station is using local 20x2 LCD with I2C interface, and 4 control buttons. Actually any suitable LCD can be used, just need to change LCD library.

Remote station is driving up to 8 irrigation channels, and supports local sensors - Temperature, Humidity etc. Currently the code is wired to support BMP180 Temperature/Humidity sensor, other sensors can be supported as required.

The project is still in development, it is v0.3 level and not quire ready for real-life use.