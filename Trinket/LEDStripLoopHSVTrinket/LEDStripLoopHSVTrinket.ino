/*
 * HSV fade sketch intended for RGB LEDs or RGB LED strips.  Based on a program I found at
 * http://www.jerome-bernard.com/blog/2013/01/12/rgb-led-strip-controlled-by-an-arduino/ ,
 * which used code from Alvy Ray Smith's web site at  http://www.alvyray.com/Papers/hsv2rgb.htm
 * though in incorporating it adopted some inefficient data passing schemes which I've removed
 * here to streamline the math.
 *
 * This version has been extended so it supports the Adafruit Trinket small Arduino
 * compatible via the #define TRINKET code
 *
 * Copyright (c) 2013 David Bryant <djbryant@gmail.com>
 *
 * Author : David Bryant (david@orangemoose.com)
 * Version: 1.0
 * Date   : Dec 2013
 */

// Include header that defines our color datatypes & functions
#include "colorspace.h"

// Conditional compilation needed to handle special code for Trinket
#define TRINKET 

/*
 * LED control needs to be provided through PWM-enabled output pins,
 * which are in different places on different boards.
 *
 * Adjust the pin assignments for red, green, and blue to match your wiring.
 */
#ifdef TRINKET
  #define REDPIN   4     // Trinket PWM pin for red LED
  #define GRNPIN   1     // Trinket PWM pin for green
  #define BLUPIN   0     // Trinket PWM pin for blue
  #define setRGBLED(pin,value) analogWriteT(pin,value) 
#else  // Arduino Uno, etc.
  #define REDPIN   5     // Arduino PWM pin for red LED
  #define GRNPIN   6     // Arduino PWM pin for green
  #define BLUPIN   3     // Arduino PWM pin for blue
  #define setRGBLED(pin,value)  analogWrite(pin,value)
#endif

#define DELAY   50     // between color changes, in milliseconds

/*
 * Color math is done in HSV (hue, saturation, value) color space. Generally hue is
 * defined on the range 0-360 degrees but can also be expressed as which portion of a
 * six-sided hexacone the color point occupies and so falls on the range (0.0->6.0). That's
 * the scheme used here, with v and s both (0.0->1.0).
 */
#define HUE_DELTA 0.01 // Hue change increment each update, smaller value gives more gradual
                       // change but also make the full cycle take longer
 
long rgb[3];

// Initialize our HLS quantities
float hue=0.0, saturation=1, value=1;

/*
 * Need to calibrate for different brightness of the R, G, and B LEDs as they generally
 * don't have the same luminosity (luminance).  Check your LED or LED strip specifications
 * to get the right values.
 *
 * For example, SparkFun RGB LED sku: COM-09264 has max Luminosity (RGB): (2800, 6500, 1200)mcd,
 * so normalizing them to the dimmest (blue at 1200mcd in this case) and scaling to 256 gives
 * R  250/600  =  107/256
 * G  250/950  =   67/256
 * B  250/250  =  256/256
 */
long bright[3] = { 107, 67, 256};
// long bright[3] = { 256, 256, 256};
 
long k, temp_value;
 
void setup () {
    // Serial.begin(9600);

#ifdef TRINKET
    PWM4_init();    // Special code to enable PWM on Trinket pin 4
#endif
    
    pinMode(REDPIN, OUTPUT);
    pinMode(GRNPIN, OUTPUT);
    pinMode(BLUPIN, OUTPUT);
    rgb[0] = rgb[1] = rgb[2] = 0;   
    setRGBLED(REDPIN, 0);
    setRGBLED(GRNPIN, 0);
    setRGBLED(BLUPIN, 0);
}
 
void loop() {
  RGB rgbvalue;
  
  hue += HUE_DELTA;
  if (hue >= 6.0) {
    hue=0.0;
    // Serial.println("*****************************");
  }
  
  rgbvalue = HSVcolor_to_RGB(hue, saturation, value);
  rgb[0] = rgbvalue.r * bright[0]/256;
  rgb[1] = rgbvalue.g * bright[1]/256;
  rgb[2] = rgbvalue.b * bright[2]/256;

  /*
  Serial.print("("); Serial.print(hue); Serial.print(","); Serial.print(saturation); Serial.print(","); 
  Serial.print(value); Serial.print(") -> (");
  Serial.print(rgb[0]); Serial.print(","); Serial.print(rgb[1]);Serial.print(","); Serial.print(rgb[2]);
  Serial.println(")");
  */
  setRGBLED(REDPIN, rgb[0]);
  setRGBLED(GRNPIN, rgb[1]);
  setRGBLED(BLUPIN, rgb[2]);
  
  delay(DELAY);
}
/*
 * Convert a color defined as (hue,saturation,value) into RGB. 
 */
RGB HSVcolor_to_RGB( float h, float s, float v ) {
  int i;
  float m, n, f;
  RGB rgb;
 
  // not very elegant way of dealing with out of range: return black
  if ((s<0.0) || (s>1.0) || (v<1.0) || (v>1.0)) {
    rgb.r = rgb.g = rgb.b = 0;
    return rgb;
  }
 
  if ((h < 0.0) || (h > 6.0)) {
    rgb.r = rgb.g = rgb.b = v*255;
    return rgb;
  }
  i = floor(h);
  f = h - i;
  if ( !(i&1) ) {
    f = 1 - f; // if i is even
  }
  m = v * (1 - s);
  n = v * (1 - s * f);
  switch (i) {
    case 6:
    case 0:
      rgb.r = v*255;  rgb.g = n*255;  rgb.b = m*255;
      break;
    case 1:
      rgb.r = n*255;  rgb.g = v*255;  rgb.b = m*255; 
       break;
    case 2:
      rgb.r = m*255;  rgb.g = v*255;  rgb.b = n*255;
      break;
    case 3:
      rgb.r = m*255;  rgb.g = n*255;  rgb.b = v*255;
      break;
    case 4:
      rgb.r = n*255;  rgb.g = m*255;  rgb.b = v*255;
      break;
    case 5: 
      rgb.r = v*255;  rgb.g = m*255;  rgb.b = n*255;
      break;
  }
  return rgb;
}

// CUSTOM CODE NEEDED FOR ADAFRUIT TRINKET

#ifdef TRINKET
/*
 * Special variation of analogWrite() for Trinket so it calls
 * custom PWM-enabling version we have here on pin 4.
 */
void analogWriteT(int pin, byte value)
{
  if(pin == 4) analogWrite4(value);
  else         analogWrite(pin,value);
}

/*
 * Special functions to support PWM output on Trinket pin 4.  These are needed
 * because the Arduino IDE does not set up PWM output on pin 4 even though it
 * is supported by the ATTIny85 hardware.  Hopefully this gets fixed someday...
 */
void PWM4_init() {
  // Set up PWM on Trinket GPIO #4 (PB4, pin 3) using Timer 1
  TCCR1 = _BV (CS10);                  // no prescaler
  GTCCR = _BV (COM1B1) | _BV (PWM1B);  // clear OC1B on compare
  OCR1B = 127;                         // duty cycle initialize to 50%
  OCR1C = 255;                         // frequency
}
 
// Function to allow analogWrite on Trinket GPIO #4 
void analogWrite4(uint8_t duty_value) {  
  OCR1B = duty_value;  // duty may be 0 to 255 (0 to 100%)
}

#endif // TRINKET
