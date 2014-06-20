Seven-Segment Display Projects
==============================

Seven-segment displays present an interesting challenge.   Technically they're just a bunch of LEDs, so controlling them with an Arduino is trivial and quite familiar. However, they are a *lot* of LEDs, as a four digit seven-segment display with decimal points means you need to control 32 different LEDs.  Happily there are a variety of schemes for doing that, ranging from nearly all software to nearly all hardware.  I love LEDs, so I'm enjoying trying as many of the different schemes as I can find.

## My Preferred Approach

**Using the MAX7219/MAX7221**

The MAX7219/MAX7221 are compact serial input/output common-cathode drivers that easily interface microcontrollers to seven-segment numeric LED displays of up to eight digits, or to bar-graph displays, or to 64 individual LEDs (often in an 8x8 matrix).  Everything you need is provided on a single, 24-pin chip including multiplexing scan circuitry, segment and digit drivers using a single external resistor to control drive current for all LEDs, and an SPI-style serial interface for sending data using only three pins (plus ground).  You can chain multiple MAX7219/MAX7221 drivers together so as to drive even more digits, bar-graphs, or individual LEDs without using any addiitonal control pins.

I discovered the MAX7219 after having spent quite a bit of time experimenting with shift registers (see below), BCD decoders (see below), interrupt-control logic (see below), and directly driving using lots of Arduino pins (see below). I have to say the MAX7219 is a great solution for a number of reasons:

*  You won't need much software to get the job done as you don't need to worry about multiplexing across
   the digits to be displayed, controlling each digit separately, or figuring out which segments to light up.
*  Far fewer hardware components are needed, as one resistor sets the segment drive current for every segment,
   and the chip can source enough current directly to the segments without needing additional transistors or buffers.
*  You can drive fewer than eight digits if you like through a configurable scan-limit register,
   and can control the brightness of the display
*  There's a BCD decoder on-board with built-in Code B font (all the digits plus dash, blank, and the 
   letters 'H', 'E', 'L', and 'P') in case you don't want to control each segment individually.
*  A low-power shutdown mode retains stored data but otherewise puts the device to sleep

There's still quite a bit of wiring to be done to use the MAX7219/7221 as it must take input from the Arduino (three pins) and control the seven-segment display through one output for each digit (eight lines) and one output for each segment (eight lines). There's also the usual VCC and GND connections, plus the external resistor for controlling segment drive current.

As is so often the case with Arduino, specialized software for using the MAX7219 has already been created by other folks in the community. [LedControl](http://www.wayoda.org/arduino/ledcontrol/) is an Arduino library developed by Eberhard Fahle specifically for using the MAX7221/7219, either with seven-segment displays or an LED matrix.  It's described on the [Arduino Playground](http://playground.arduino.cc/Main/LedControl) and available via [GitHub](https://github.com/wayoda/LedControl).  The library exposes all the various capabilities of the MAX7219/MAX7221, which are quite considerable as I've briefly described above.

At the moment I have built one Arduino sketch, `LedControl7Segment.ino`, using the LedControl library and it is largely intended just to help me get a full feel for the library's API.  It only uses the MAX7219 and eight digts of attached seven-segment display to provide a simple counter, but also contains convenience functions I've written to properly format and display integers and floating point numbers on that eight-digit display.

## Other Alternatives


**Simple, Direct Control**

The most straightforward way for an Arduino to control a seven-segment display is to use one digital output pin for each segment (plus one more for the decimal point if you care), and one digital output pin for the common cathode (or anode) for each digit.  That takes twelve digital pins for a four digit display, including use of the decimal point. If you're avoiding using Arduinio digital pins 0 and 1 because they provide serial TX/RX, then you only have twelve digital pins available (2 through 13 inclusive).  Certainly it's hard to imagine using this scheme for any practical application, but it is fine developing and understanding of basic seven-segment display use.

The Arduino sketch `SevenSegTinker.ino` uses this direct scheme to explore driving a seven-segment display.  It isn't very clever about refreshing the digits, though, just looping through updating each digit in turn. That wouldn't be very useful if the sketch needed to do much else as part of its main loop, though.

**Interrupt-based Display Refresh**

A better approach to refreshing the display would be to have a timer-driven routine that gets called on a regular basis to sequence through and redraw the proper digits.  Human persistence of vision means this wouldn't have to be done at a high rate as the retina retains an image for about one twenty-fifth of a second.  The sketch could do other things the rest of the time and not have to worry about managing the display. A great way to do this on an Arduino is to use timer interrupts, and have an interrupt service routine take charge of refreshing the display.

The sketch `SevenSegInterrupt` takes this approach and is otherwise quite similar to the direct control version.

**BCD Display Interface**

One way to reduce the number of pins needed for an Arduino to drive a seven-segment display is to have the Arduino output the [binary-coded decimal](http://en.wikipedia.org/wiki/Binary-coded_decimal) (BCD) value of the digit to be displayed.  In this way you don't need one pin per each of the seven segments but instead one pin for each of the four bits required to provide a BCD representation of the digits 0 through 9 inclusive.  That saves three pins!  (A BCD decoder dosen't help you with the decimal point, alas.)  This approach does, however, require one additional component namely a BCD-to-seven-segment decoder such as the CD74HC4543.  Because the decoder is designed to drive LED displays it is capable of sourcing enough current to do so directly and therefore doesn't require an additional transistor to provide adequate current drive.

Two sketches use the BCD decoder approach - `SevenSegBCDTinker` does so via direct display control and `SevenSegBCDInterrupt` relies on a timer interrupt routine to refresh the display.

**Shift Register Control**

Another way to use fewer pins to drive the display is to incorporate a shift register in the output circuit.  The Arduino still needs to determine which segments are on and which are off, but instead of driving those segments directly can serially send that information to a shift register, which then controls the segments.  Conveniently an 8-bit shift register can drive all the segments in the display, and only requires three Arduino pins for the necessary data transfer.  This therefore saves four pins if you still manage the decimal point separately, and five pins if you drive the decimal point through the shift register.

Obviously you'll need an extra component here, namely the shift register.   Some care is required in selecting the shift register, however, as it is better to find one that can provide enough drive current to power the LEDs.  Many shift registers aren't rated for more than 4-6 mA per output pin which isn't enough and you'd therefore need a transistor to provide adequate drive for each segment.  However, shift registers like the Micrel MIC5891 have built-in output transistors that can handle as much as 500mA!

There are three sketches here that take this approach, using the MIC5891:

*  `SevenSegShiftRegTinker` - which manages refreshing the display directly
*  `SevenSegShiftRegInterrupt` - which refreshes the display through a timer interrupt routine
*  `SevenSegShiftRegInterruptThermF` - which implements a simple digital thermometer by reading data from
    a DS18B20 digital temperature sensor and displaying its value on the seven-segement display, with a
    timer interrupt routine handling the refresh.

