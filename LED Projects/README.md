LED Projects
===============

The classic "Hello World" program everyone new to Arduino experiences first is `Blink`, which blinks an LED connected to an Arduino output pin.  I got my start that way too, and like many other geeks can't seem to stop experimenting with LEDs in general.  As a result I've accumulated a variety of LED related Arduino sketches, which are gathered here.Most are organized based on the particular bit of LED hardware involved, either discrete LEDs, seven-segment LED displays, LED strips, multi-color RGB LEDs, etc.
 

## Content

**LED Strip**

The fundamental building block of twenty-first century Christmas decorations and geeky home lighting is the LED strip, which packages a linear collection of LEDs into a single, controllable strand.  Basic ones tie all the LEDs, typically of some RGB type, together so they all will display the same color at the same time.  Advanced ones have controllers tied to each LED so every one can be controlled separately and the entire strand can be used to create elaborate, animated multi-color displays.  The large number of LEDs typically used requires more current than an Arduino or other microcontroller can source directly, so LED strip projects generally require a combination of software and high-current interface hardware.

**RGB LED Backpack**

If one LED is good and many LEDs are better then an addressable matrix of LEDs must be great.  Even greater is if those LEDs are multi-color RGB ones that can be used to display any color imaginable.  At Maker Faire 2014 I met Pierce Nichols who runs [Logos Electromechanical](http://www.logos-electro.com), a robotics and mechatronics products and consulting company in Seattle.  Pierce had several of his products on display and I couldn't resist his [RGB LED Matrix](http://www.logos-electro.com/store/rgb-led-matrix-r2), which provides an 8x8 RGB LED display in a 60mm square package with a built-in I2C controller.  Pierce has written an Arduino library so it is easy to control the RGB LED Matrix through a simple API.

**Seven-Segment Display**

I've spent quite a bit of time coming to understand how seven-segment LED displays work.  They have a particular charm for me, probably because I was around in the 1970s when they first came into the marketplace and were suddenly everwhere from wrist watches to calculators to alarm clocks to home appliances.  There are two key problems to solve in using seven-segment displays -- how to properly control all the individual segments given there are eight per digit and often four or eight digits per display, and how to keep some value visible on the display at all times given any application needs to both update/refresh the display (a fair amount of timing-critical work) and do its actual job (e.g., get environmental data from sensors, handle input, control something external, etc.). For more details on all this and my various attempts at figuring it out it check out the README file in the Seven-Segment Display subfolder.