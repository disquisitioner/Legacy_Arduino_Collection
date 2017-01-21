OLED Weather Station Plus
===============

My third generation weather station added new sensors for wind speed/direction and rainfall.  Reading those sensors required different approaches as the anemometer (wind 
speed) and rain gauge reported activity as a steady stream of pulses that needed to be handled through interrupts, and the wind vane (wind direction) operated as a
resistor network whose value changed depending on the orientation of the vane.  This expanded the core weather station code beyond the capacity of the Arduino Uno I'd used for previous generations so I adopted Teensyduino, an Arduino compatible device but with greater memory, stack, and processing capacity.  
