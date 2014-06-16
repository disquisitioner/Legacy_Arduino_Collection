Arduino
===============

I think the two things I most appreciate about the Arduino platform are the ease of experimenting with all manner of hardware components, and the incredibly rich collection of great work other people have done and freely contributed back to the community.  Want to experiment with sensors? Batteries? Seven-segment display? Robots? No problem!  Curious about programming with interrupts?  Motor control?  Audio?  Most likely all the components you need and great examples of how to get started are an easy web search away.

So here is both the accumulation of my own experimentation, and my modest contribution back to the community.  Much of what I've done has yet to leave my workbench, but a few creations have made it out into the real world.  You'll find both specific individual projects (like my DSLR time-lapse intervalometer) and collections of programs that have resulted from experimenting with hardware components that struck my fancy (like controllable LED strips & matrices). 

## Content

**Intervalometer (Project)**

This is the project that attracted me to Arduino in the first place.  I wanted to try my hand at time lapse photography but couldn't understand why I needed to buy an expensive camera controller (called an 'Intervalometer') when it was little more than some simple timing calculations and a switch to trigger the remote control of my digital SLR.

**Weather Station (Collection)**

This is my second major Arduino project, though the first one to get put into everyday use.  For years my wife and I had thought about buying and installing a home weather station but somehow never got around to doing it.  It didn't take me long to realize I could easily build my own and have a lot more fun in the process.  An Arduino manages several weather-related sensors, reading and processing their data to generate a set of readings which can be displayed directly and also relayed to a home computer (or similar device) for logging, daily report generation, etc. There are two versions at present - one using a 16x2 LCD character display and one using a 128x64 pixel-addressable OLED.

**OLED Name Badge (Project) **

Motivated by the "build your own name tag" activity at the Make Hardware Innovation Workshop, I decided to craft something a little less spontaneous but more elaborate. Imagine the traditional "Hello, My Name Is..." badge redone on a comparably-sized circuit board using an Arduino, OLED display, and lithium-polymer battery.

**HexBright (Collection)**

I love flashlights so was excited (delighted, even) to discover [HexBright](http://www.hexbright.com), a very bright, well-made, compact LED flashlight that had an Arduino and some sensors on board and could be programmed to behave however you wanted it to. So far I've only made a few simple modifications to the factory-installed program, but I've also had fun trying some of the novel variations developed by other folks in the HexBright community.

**Thermometers (Collection)**

As a precursor to my weather station project I spent time experimenting with various analog and digital temperature sensors, and with different ways to display temperature information including LCD and in binary with LEDs.  Even a basic thermometer is useful, and so this has come to be my standard Arduino demo program (even moreso than blinking an LED).

**Trinket (Collection)**

Adafruit's [Trinket](http://www.adafruit.com/products/1501) is a great Arduino-like microcontroller based on the Atmel ATtiny85.  It is somewhat Arduino compatible but there are some tradeoffs to get such a small size (1.2 x 0.6 inches) and low price ($7.95).  Sketches developed for Trinket won't necessarily run on a full Arduino though in some cases it is practical to use conditional compilation to use the same source code with either.  Just to be explicit about it, I've collected my Trinket-based Sketches in a separate collection.

**LED Projects (Collection)**

I can't seem to resist the appeal of LEDs of all kinds, such that it seems like I always have a couple of LED projects under way at any point.  I've kept all those sketches together in this collection, though they embrace a wide range of target hardware - RGB LED strips, 8x8 RGB LED matrices, 7-segment displays, and special LED products like the BlinkM and MaxM modules from ThingM.

