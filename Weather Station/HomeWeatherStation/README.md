Home Weather Station
===============

At last, a complete version of my home weather station software supporting
the full set of sensors I was interested in and ready to be paired with a
custom printed circuit board designed for all the electronics. This version
includes transitioning from the BMP085 temperature/pressure sensor, which
had been retired in favor of a newer BMP180 version.  It is also designed
for use with Arduino Uno, though as I was putting the finishing touches
on this version I discovered Teensyduino, which provides full Arduino Uno
compatibility with a more powerful processor, larger stack/memory, and some
programming advantages.  I've therefore transitioned all my home weather
station work going forward to the Teensyduino platform and created a
newer version of this sketch (which you'll find one level up in this repo)
for that effort.
