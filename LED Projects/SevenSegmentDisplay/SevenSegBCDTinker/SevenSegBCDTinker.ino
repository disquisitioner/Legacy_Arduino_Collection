/*
 * Program for experimenting with seven-segment LED displays using
 * a BCD to 7-Segment decoder/driver
 * Author: David Bryant (david@orangemoose.com)
 * Date: 23 March 2014
 *
 * Generally this is the same as the non-BCD version but we convert the
 * 0-9 value to be displayed into a 4-bit BCD value, then drive the four
 * lines needed to communicate that th the BCD decoder/driver.
 *
 * This version is built for use with a CD74HC4543 7-segment decoder/driver,
 * wired as follows:
 *
 *  Pin 1 (LD) = Disable Input = VCC (HIGH)
 *  Pin 2 (D2) = Digit 2 of input BCD = Arduino pin 6
 *  Pin 3 (D1) = Digit 1 of input BCD = Arduino pin 7
 *  Pin 4 (D3) = Digit 3 of input BCD = Arduino pin 5
 *  Pin 5 (D0) = Digit 0 of input BCD = Arduino pin 8
 *  Pin 6 (PH) = Phase Input = GND
 *  Pin 7 (BI) = Blanking Input = GND
 *  Pin 8 (GND) = GND
 *  Pin 9 (a) = Segment a of LED
 *  Pin 10 (b) = Segment b of LED
 *  Pin 11 (c) = Segment c of LED
 *  Pin 12 (d) = Segment d of LED
 *  Pin 13 (e) = Segment e of LED
 *  Pin 14 (g) = Segment g of LED (yes, g not f -- not in order...)
 *  Pin 15 (f) = Segment f of LED (yes, f)
 *  Pin 16 (VCC) = VCC 5V
 */
  
const byte SEGMENT_ON = HIGH;
const byte SEGMENT_OFF = LOW;
const byte DIGIT_ON = LOW;
const byte DIGIT_OFF = HIGH;

#define DIGITS   4

const byte bcdPinMap[4] = {8,7,6,5};    // BCD control: D3,D2,D1,D0
const byte digitMap[DIGITS] = {12,13,11,10};   // left to right
const byte dpPin = 9; // decimal point pin

void setup()
{
  int i;
  
  // Initialize pins as outputs
  for(i=0;i<DIGITS;i++) {
    pinMode(digitMap[i],OUTPUT);
  }
  for(i=0;i<SEGMENTS;i++) {
    pinMode(bcdPinMap[i],OUTPUT);
  }
  pinMode(dpPin,OUTPUT);
  
  // turn all digits off
  for(int i=0;i<DIGITS;i++) {
    digitalWrite(digitMap[i],DIGIT_OFF);
  }
}

int val = 0;

void loop()
{
  if(val > 9999) val = 0;
  showNumber(val);
  val++;
}

void showNumber(int n)
{
  int d1,d2,d3,d4;
  
  if(n < 0) n =0;
  if(n > 9999) n = 9999;  
  d1 = (int) n/1000;
  n  = n - (d1*1000);
  d2 = (int) n/100;
  n  = n - (d2*100);
  d3 = (int)n/10;
  d4 = n%10;

  setDigit(0,d1);
  setDigit(1,d2);
  setDigit(2,d3);
  setDigit(3,d4);
}

// Display a numeric value (0-9) on a digit (0-3)
void setDigit(int d,int n)
{
  int i, mask;
  
  // Turn all the digits off. Note that we turn all digits off
  // then just set the right segments and re-illuminate one digit
  // so this only works if the main loop is sweeping through the
  // digits fast enough such that users don't see the digits flickering
  for(int i=0;i<DIGITS;i++) {
    digitalWrite(digitMap[i],DIGIT_OFF);
  }
  
  for(i=0;i<4;i++) {
    if(n & bit(i)) {
      digitalWrite(bcdPinMap[i],SEGMENT_ON);
    }
    else {
      digitalWrite(bcdPinMap[i],SEGMENT_OFF);
    }
  }
  digitalWrite(digitMap[d],DIGIT_ON);  // Enable digit
  delay(5);
}

