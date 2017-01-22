Weather Station
===============

This is my second major Arduino project, though the first one to get put into everyday use.  For years my wife and I had thought about buying and installing a home weather station but somehow never got around to doing it.  It didn't take me long to realize I could easily build my own and have a lot more fun in the process.  An Arduino manages several weather-related sensors, reading and processing their data to generate a set of readings which can be displayed directly and also relayed to a home computer (or similar device) for logging, daily report generation, etc. 

Once you have an Arduino actively reading from and managing a few sensors you can expand the system in an almost infinite number of ways.  You can add more sensors, 
you can incorporate more sophisticated displays, you can enhance connectivity through on-board networking or wireless interfaces, you can build more functionality into
upstream systems fed by the Arduino, you can go for battery-powered portability, and pretty much anything else you can think of.

So the collection of projects here represents a variety of ways I've explored evolving the basic weather station concept.

## Content

**LCD Weather Station (Project)**

My first weather station effort using a few simple sensors and a 16x2 LCD display.  It has all the key elements, though, including averaging sensor readings over a
configurable interval and reporting them via the serial port for processing by some upstream system -- initially a PC that could log the data to non-volatile storage and 
also upload them to web services.

**OLED Weather Station (Project)**

Adding a pixel-addresable OLED display allowed greater flexibility in showing current sensor readings and provided a more "professional" user interface. It also required
building my own fonts, and motivated me to implement multiple screens that could be sequenced through to offer a variety of looks ranging from easily readable at a
distance and also diagnostic for monitoring sensor data in real-time.  This version worked well enough that I installed it for my family and it became a reliable
resource for us all in monitoring weather conditions at home.

**OLED Weather Station Plus (Project)**

Once I had a basic home weather station in operation I naturally wanted to expand it.  The first system only measured outdoor temperature, indoor temperature, and 
barometric presure.  I wanted to add rainfall measurement along with wind speed and direction and found a set of sensors at Sparkfun that were perfectly suited for
that purpose. However, I also discovered that the code I'd written for the Arduino had grown to the point that it no longer fit comfortably within the memory 
limitations of an Arduino Uno.  Luckily I discovered Teensyduino, an Arduino-compatible system build on a more powerful processor with much greater memory, stack 
space, and performance.  The combination of more sensors and the shift to the Teensyduino convinced me to metamorphose the project into something new, with a new
name.

**Weather Station PCB (Collection)**

OK, I'll admit it.  When I built my first Arduino-powered weather station in late 2012 I did what many hobbyists do, namely I assembled all the electronics components and
connecting them using a breadboard.  I love breadboards and have a drawer full of them. I've even sponsored a Kickstarter project to build an Arduino inside of a working 
breadboard. The breadboard was a vital component from the beginning as it's how the indoor sensor and display were integrated, and it's where 
I handled connections to the all the various outdoor sensors. With the expanded 'Weather Station Plus' version I also needed to debounce 
the switch-based interface to the rain gauge and anemometer. Eventually I decided it was time to design my own custom printed circuit board for 
my weather station.  Happily I discovered a class via Meetup.com and signed up to learn circuit board design including use of specialized sofware involved.  
I designed my board and sent it to OSH Park to be fabricated.  I then needed a set of Arduino projects to help me incrementally test assembly of the 
circuit board using an Arduino and relevant subsets/variation of my full Weather Station Plus software.  This folder
contains all the PCB test programs I wrote to qualify my custom PCB and get it on-line as part of my weather station.
