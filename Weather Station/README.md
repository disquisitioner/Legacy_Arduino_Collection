Weather Station
===============

This is my second major Arduino project, though the first one to
get put into everyday use.  For years my wife and I had thought
about buying and installing a home weather station but somehow never
got around to doing it.  It didn't take me long to realize I could
easily build my own and have a lot more fun in the process.  An
Arduino manages several weather-related sensors, reading and
processing their data to generate a set of readings which can be
displayed directly and also relayed to a home computer (or similar
device) for logging, daily report generation, etc.

Once you have an Arduino actively reading from and managing a few
sensors you can expand the system in an almost infinite number of
ways.  You can add more sensors, you can incorporate more sophisticated
displays, you can enhance connectivity through on-board networking
or wireless interfaces, you can build more functionality into
upstream systems fed by the Arduino, you can go for battery-powered
portability, and pretty much anything else you can think of.

So the collection of projects here represents a variety of ways
I've explored evolving the basic weather station concept.

*NOTE:* If you're just after the latest version of my home weather
station software then you're looking for the 'Home Weather Station - Teensy'
version included here.

## Content

**LCD Weather Station (Project)**

My first weather station effort using a few simple sensors and a
16x2 LCD display.  It has all the key elements, though, including
averaging sensor readings over a configurable interval and reporting
them via the serial port for processing by some upstream system --
initially a PC that could log the data to non-volatile storage and
also upload them to web services.

**OLED Weather Station (Project)**

Adding a pixel-addresable OLED display allowed greater flexibility
in showing current sensor readings and provided a more "professional"
user interface. It also required building my own fonts, and motivated
me to implement multiple screens that could be sequenced through
to offer a variety of looks ranging from easily readable at a
distance and also diagnostic for monitoring sensor data in real-time.
This version worked well enough that I installed it for my family
and it became a reliable resource for us all in monitoring weather
conditions at home.

**OLED Weather Station Plus (Project)**

Once I had a basic home weather station in operation I naturally
wanted to expand it.  The first system only measured outdoor
temperature, indoor temperature, and barometric presure.  I wanted
to add rainfall measurement along with wind speed and direction and
found a set of sensors at Sparkfun that were perfectly suited for
that purpose. That turned into a more interesting effort that I had
expected as those sensors operated very differently -- the rain gauge
and anemometer as switches that needed to be handled via interrupts
and the wind vane as a variable resistor network.

**Home Weather Station (Project)**

The next step in the evolution of my home weather station project
was to design a customer printed circuit board (PCB) and transition away
from the breadboard approach I'd been using for all the electronics
components and sensor connections.  As I was gearing up to do that
I learned that the BMP085 sensor I had been using for indoor
temperature and barometric pressure had been retired in favor of
a newer version, the BMP180.  So I ordered
a new BMP180 and modified my weather station code to use it instead of the
BMP085.  Because I felt I was (finally) ready for a more complete project I
boldy chose the name 'Home Weather Station' for this version.  

**Home Weather Station - Teensy (Project)**

Alas, with all the changes leading up to this as my (finally)
complete home weather station, I discovered that the code I'd written
for the Arduino had grown to the point that it no longer fit
comfortably within the memory limitations of an Arduino Uno.  Luckily
I discovered Teensyduino, an Arduino-compatible system build on a
more powerful processor with much greater memory, stack space, and
performance. Teensy made other things possible, e.g. all digital
pins on Teensy are interrupt enabled so I could change how I used
those pins to simplify the code.  Going forward I plan to keep
evolving this code path, at least as the basis for use with my
custom PCB.

**Weather Station PCB (Collection)**

With the updated and expanded 'Home Weather Station' code up and running
I was at last ready to design my own custom printed circuit board.  Happily 
I discovered a class via Meetup.com and signed up to learn circuit 
board design, then used that knowledge to layout my board and send it to OSH
Park to be fabricated.  Because I couldn't be certain my first ever PCB
design would get everything right I decided I  needed a set of Arduino
projects to help me incrementally test assembly of the circuit board 
using relevant subsets/variation of my full Home Weather Station software.
This folder contains all the PCB test programs I wrote to qualify my
custom PCB and get it on-line as part of my weather station.
