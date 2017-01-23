Weather Station PCB
===============

OK, I'll admit it.  When I built my first Arduino-powered weather
station in late 2012 I did what many hobbyists do, namely assembled
all the electronics components and connecting them using a breadboard.
I love breadboards and have a drawer full of them. I've even sponsored
a Kickstarter project to build an Arduino inside of a working
breadboard. The breadboard was a vital component from the beginning
as it's how the indoor sensor and display were integrated, and it's
where I handled connections to all the various outdoor sensors.
With the expanded 'Weather Station Plus' version I also needed to
debounce the switch-based interface to the rain gauge and anemometer.
Eventually I decided it was time to design my own custom printed
circuit board for my weather station.  Happily I discovered a class
via Meetup.com and signed up to learn circuit board design including
use of specialized sofware involved.  I designed my board and sent
it to OSH Park to be fabricated.

As it was my first custom PCB I wasn't sure I had properly handled
all the critical aspects of PCB design that I knew could cause
problems -- routing, interference, improper connections, etc. I
therefore wanted to be able to systematically add each component
to the PCB and test that single addition to make sure everything
worked, or if not at what point I added something that caused the
whole board to not work properly.  This folder contains all the
PCB test programs I wrote, each a subset of the overall 'Home Weather
Station' software, to qualify my custom PCB and get it operational.
