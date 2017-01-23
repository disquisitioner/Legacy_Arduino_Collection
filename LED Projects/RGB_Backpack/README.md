RGB LED Matrix
==============

The Logos Electromechanical [RGB LED
Matrix](http://www.logos-electro.com/store/rgb-led-matrix-r2) is a
great device. It has a large, 60mm square display made up of an 8x8
matrix of full-color RGB LEDs, and is controllable via a built-in
I2C interface that greatly simplifies wiring and takes only two
Ardiuno pins (plus power and ground).  You can set the I2C device
address through solder jumpers on the back so it's possible to use
several simultaneously.

Logs Electromechanical has, naturally, created an Arduino library
for their RGB matrix, which is available through
[GitHub](https://github.com/logos-electromechanical/RGB-matrix-backpack).
Documentation on the board (including assembly instructions) and
the library are also part of the GitHub repository.

Additionally, there are two bits of information that will make it
easier for you to use the RGB Matrix.

First, the LED matrix itself has two identical sets of eight connector
pins on the back and so can be inserted in the backpack circuit
board either of two ways.  It's not immediately obvious which way
is the right way, however.  The secret here is to notice that there
is a small, silver-grey "1" on the back of the matrix by one of the
corner pins.  On the circuit board you'll notice that the header
solder pads is square while all the rest are round.  The pin indicated
by the square solder pad is pin 1, so should be mated with the pin
marked "1" on the back of the matrix. It is much easier to see this
*before* you solder the header in place, but if that's already
happened then another way to tell is to hold the completed backpack
circuit board with the I2C connector pointed up and facing you, as
in that orientation pin 1 is in the upper-left corner.

The second key to using the LED matrix is to understand the I2C
connector.  The manual explains that the 10 pin connector used is
intended to be compatible with a series of I2C interface boards
from Snoot Labs and tells you which pins are used.  However, it's
not obvious which pins are which, looking at the 2x5 connector on
the back of the circuit board.  The key here is to notice a small
triangle on the outside top of the 2x5 plastic connector housing.
That triangle marks pin 1 in the connector, and the other pins are
oriented as follows:

    +-----------------+
    |  9  7  5  3  1  |
    | 10  8  6  4  2  |
    +-----------------+

## Content

**Scrolling Sign**

This sketch uses the matrix to create a scrolling sign using a 5x7
font based on the one built into the common HD44780 LCD controller.
Because those characters are one row shorter than the 8x8 LED matrix
there are two variants of the font -- one with the characters'
baseline on the bottom row of the matrix and one with the baseline
on the first row of the matrix.  Each font is defined in its own
header file, both of which are here along with the sketch.  You
control which font is used by altering the `#include` directive
near the top of the sketch, and also specify the text of the message
to be displayed in a string defined there.

**Spectrum Analyzer**

Simple spectrum analyzer built using the MSGEQ7 seven-band graphic
equalizer IC and the Logos Electromechanical RGB LED Matrix.  With
just one 8x8 LED matrix we run in mono mode, combining the left and
right audio signals at the MSGEQ7 input.  While simple, this makes
for a pretty mesmerizing display alongside your audio, TV or computer
speakers.
