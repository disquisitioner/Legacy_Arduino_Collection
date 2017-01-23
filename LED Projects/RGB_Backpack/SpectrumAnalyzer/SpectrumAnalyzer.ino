/*
 * Simple spectrum analyzer built using the MSGEQ7 seven-band graphic equalizer IC and
 * the Logos Electromechanical RGB LED Matrix.  With just one 8x8 LED matrix we run in
 * mono mode, combining the left and right audio signals at the MSGEQ7 input.
 *
 * Author: David Bryant (djbryant@gmail.com)
 * Version: 1.0
 * Date: 14 June 2014
 *
 * Based on the excellent example by John Boxall from Tronixstuff at
 * http://tronixstuff.com/2013/01/31/tutorial-arduino-and-the-msgeq7-spectrum-analyzer/
 */
 
#include <Wire.h>
#include <RGB_Matrix_Arduino.h>

// Colors we use for the display background and foredground (dot)
#define BKG 0x000000
#define DOT 0x0000ff

uint8_t buf[192];  // Buffer for the RGB Matrix

RGB_Matrix matrix = RGB_Matrix((uint8_t)0x7f, buf);

int strobe = 4; // MSGEQ7 strobe pin on digital 4
int res = 5;    // MSGEQ7 reset pins on digital 5
int audio[7];   // store band values in this array
int band;

/*
 * Scale factor for input, somewhat like a sensitivity control. Unscaled handling would
 * map an 0-1023 analog input value to 8 LEDs, so would divide the input by 128. The default
 * value for this scale factor is therefore 128.  If the input signal isn't likely to go the
 * full 0-1023 range (because you're listening to it via headphones or speakers, for
 * example) then you can use a smaller value for the scale factor, say 64 to double the
 * display sensitivity, or 96 for a factor of 1.5.  Note that the way the display code
 * is written the display will get automatically clipped (because you just can't light up
 * more than 8 LEDs...)
 */
int scale = 48;  // A value of 128 gives direct (unscaled) display of input

void setup()
{
  Serial.begin(9600);
  
  pinMode(res, OUTPUT);    // reset
  pinMode(strobe, OUTPUT); // strobe
  
  digitalWrite(res,LOW);     // reset low
  digitalWrite(strobe,HIGH); //pin 5 is RESET on the shield
  
  matrix.begin();
}

void loop()
{
  int val, j;
  
  // Read frequency band values from MSGEQ7
  readMSGEQ7();
  
  for(band=0;band<7;band++) {
    val = audio[band]/scale;  // Scale input values
    // Serial.print(audio[band]); Serial.print(" "); Serial.print(val); Serial.print(",");
    for(j=0;j<8;j++) {
      if(j<val) matrix.setPixel(7-j,band,DOT);
      else      matrix.setPixel(7-j,band,BKG);
    } 
  }
  // Serial.println();
  matrix.transmitBuf();
  delay(10);  // pause ever so slightly to reduce fast flickering of LED matrix
}

void readMSGEQ7()
// Function to read 7 band equalizers
{
  /*
   * First, reset the MSGEQ7 by pulsing the reset pin HIGH, then LOW.
   * It will then stand by to start reporting data, beginning with the
   * first band.
   */
  digitalWrite(res, HIGH);  // Resets MSGEQ7
  digitalWrite(res, LOW);   // Ends reset, enables strobe
  for(band=0; band <7; band++) {
    digitalWrite(strobe,LOW);    // Strobe pin LOW tells MSGEQ7 to report value of current band
    delayMicroseconds(30);       // Pause to let the MSGEQ7 analog output value settle
    audio[band] = analogRead(0); // Read and store the band reading   
    digitalWrite(strobe,HIGH);   // Take strobe pin high to advance to next band
  }
}
