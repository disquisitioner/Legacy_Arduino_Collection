/* 

Test sketch for use in bring-up of my Weather Station PCB.  This test
presumes the OLED has been verified to be working properly, and moves
on next to test the BMP180 temperature & barometric pressure sensor.
It uses the OLED to display test status and results.

Based on the SFE-BMP180 library example sketch from Sparkfun for their
Bosch BMP180 barometric pressure sensor and breakout board
SFE_BMP180 library example sketch
https://www.sparkfun.com/products/11824

Like most pressure sensors, the BMP180 measures absolute pressure.
This is the actual ambient pressure seen by the device, which will
vary with both altitude and weather.

Before taking a pressure reading you must take a temparture reading.
This is done with startTemperature() and getTemperature().
The result is in degrees C.

Once you have a temperature reading, you can take a pressure reading.
This is done with startPressure() and getPressure().
The result is in millibar (mb) aka hectopascals (hPa).

Hardware connections:

- (GND) to GND
+ (VDD) to 3.3V

(WARNING: do not connect + to 5V or the sensor will be damaged!)

You will also need to connect the I2C pins (SCL and SDA) to your
Arduino. The pins are different on different Arduinos:

Any Arduino pins labeled:  SDA  SCL
Uno, Redboard, Pro:        A4   A5
Mega2560, Due:             20   21
Leonardo:                   2    3

Leave the IO (VDDIO) pin unconnected. This pin is for connecting
the BMP180 to systems with lower logic levels such as 1.8V

Have fun! -Your friends at SparkFun.

The SFE_BMP180 library uses floating-point equations developed by the
Weather Station Data Logger project: http://wmrx00.sourceforge.net/

Our example code uses the "beerware" license. You can do anything
you like with this code. No really, anything. If you find it useful,
buy me a beer someday.

V10 Mike Grusin, SparkFun Electronics 10/24/2013
*/

// Your sketch must #include this library, and the Wire library.
// (Wire is a standard library included with Arduino.):

#include <SPI.h>
#include <SFE_BMP180.h>
#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>

// If using software SPI (the default case):
#define OLED_MOSI   9  // Data
#define OLED_CLK   10
#define OLED_DC    11
#define OLED_CS    12
#define OLED_RESET 13
Adafruit_SSD1306 display(OLED_MOSI, OLED_CLK, OLED_DC, OLED_RESET, OLED_CS);

#if (SSD1306_LCDHEIGHT != 64)
#error("Height incorrect, please fix Adafruit_SSD1306.h!");
#endif

// The BMP180 uses I2C so is wired to A4 (SDA) and A5 (SCL)  
// You will need to create an SFE_BMP180 object, here called "pressure":
SFE_BMP180 pressure;

#define ALTITUDE 1655.0 // Altitude of SparkFun's HQ in Boulder, CO. in meters

void setup()
{ 
  // by default, we'll generate the high voltage from the 3.3v line internally! (neat!)
  display.begin(SSD1306_SWITCHCAPVCC);
  // init done
  display.clearDisplay();   // clears the screen and buffer
  display.setTextSize(1);
  display.setTextColor(WHITE);

  // Initialize the sensor (it is important to get calibration values stored on the device).

  if (pressure.begin()) {
    display.setCursor(0,0);
    display.println(F("BMP180 init success"));
    display.display();
  }
  else
  {
    // Oops, something went wrong, this is usually a connection problem,
    // see the comments at the top of this sketch for the proper connections.

    display.setCursor(0,0);
    display.println(F("BMP180 init failed!"));
    display.display();
    while(1); // Pause forever.
  }
}

unsigned int counter = 0;

void loop()
{
  char status;
  double T,P;

  // Loop here getting pressure readings every 10 seconds.

  // If you want to measure altitude, and not pressure, you will instead need
  // to provide a known baseline pressure. This is shown at the end of the sketch.

  // You must first get a temperature measurement to perform a pressure reading.
  
  // OLED layout
  // Line 1: Temperature reading (or indication of error)
  // Line 2: Pressure reading (or indication of error)
  // Line 3: Loop counter, rolls over at 9999
  //
  // Start with a clear OLED display
  display.clearDisplay();

  // Start a temperature measurement:
  // If request is successful, the number of ms to wait is returned.
  // If request is unsuccessful, 0 is returned.
  status = pressure.startTemperature();
  if (status != 0)
  {
    // Wait for the measurement to complete:
    delay(status);

    // Retrieve the completed temperature measurement:
    // Note that the measurement is stored in the variable T.
    // Function returns 1 if successful, 0 if failure.

    status = pressure.getTemperature(T);
    if (status != 0)
    { 
      // Display temperature on OLED on line 1
      display.setCursor(0,0);
      display.print(F("Temp F: "));
      display.println((9.0/5.0)*T+32.0,2);
      
      // Start a pressure measurement:
      // The parameter is the oversampling setting, from 0 to 3 (highest res, longest wait).
      // If request is successful, the number of ms to wait is returned.
      // If request is unsuccessful, 0 is returned.

      status = pressure.startPressure(3);
      if (status != 0)
      {
        // Wait for the measurement to complete:
        delay(status);

        // Retrieve the completed pressure measurement:
        // Note that the measurement is stored in the variable P.
        // Note also that the function requires the previous temperature measurement (T).
        // (If temperature is stable, you can do one temperature measurement for a number of pressure measurements.)
        // Function returns 1 if successful, 0 if failure.

        status = pressure.getPressure(P,T);
        if (status != 0)
        { 
          // Display pressure on line 2 of the OLED
          display.setCursor(0,10);
          display.print(F("P inHg: "));
          display.println(P*0.0295333727,2);
        }
        else {
          display.setCursor(0,20);
          display.println(F("P inHG: ERROR!"));
        }
      }
      else {
        display.setCursor(0,20);
        display.println(F("P inHG: ERROR!"));
      }
    }
    else {
      display.setCursor(0,10);
      display.println(F("Temp F: ERROR!"));
    }
  }
  else {
      display.setCursor(0,10);
      display.println(F("Temp F: ERROR!"));
  }
  
  // Regardless, display our loop counter
  display.setCursor(0,20);
  display.print(F("Loop #: "));
  display.println(counter);
  display.display();
  
  if(counter >= 9999) counter = 0;
  else counter++;
  
  delay(5000);  // Pause for 5 seconds.
}
