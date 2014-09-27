/*
 * Program for experimenting with seven-segment LED displays
 * Author: David Bryant (david@orangemoose.com)
 * Date: 23 March 2014
 *
 * This version uses a shift register to control the segments in the display,
 * so only three pins are required (Strobe, Clock and Data In).  There still needs to
 * be one pin for each digit, though you could drive that through a shift register too
 * (especially if you had eight digits).
 *
 * The main loop handles refreshing the display every time through, though it is
 * also incrementing a counter to be displayed every time through.  This likely wouldn't be 
 * practical for any non-trivial application, but it is fine for learning how to manipulate
 * seven-segment displays.
 *
 * Written to work with a four digit display, and though it wouldn't be hard to expand that
 * it requires more work than just changing the DIGITS value below...
 * 
 */
  
const byte SEGMENT_ON = HIGH;
const byte SEGMENT_OFF = LOW;
const byte DIGIT_ON = LOW;
const byte DIGIT_OFF = HIGH;
 
#define SEGMENTS 7
#define DIGITS   4

// const byte segPinMap[SEGMENTS] = {8,7,6,5,4,3,2}; // a, b, c, d, e, f, g

//Pin connected to STROBE of MIC5891 (pin 4)
int latchPin = 8;
//Pin connected to CLOCK of MIC5891 (pin 2)
int clockPin = 7;
////Pin connected to SERIAL DATA IN of MIC5891 (pin 3)
int dataPin = 6;

const byte digitMap[DIGITS] = {12,13,11,10};   // left to right
const byte dpPin = 9; // decimal point pin

const PROGMEM byte SevenSegMap[16] =
{
     // Hex digits
     B11111100, B01100000, B11011010, B11110010,   // 0123
     B01100110, B10110110, B10111110, B11100000,   // 4567
     B11111110, B11110110, B11101110, B00111110,   // 89AB
     B00011010, B01111010, B10011110, B10001110,   // CDEF
  
};

void setup()
{
  int i;
  
  // Initialize pins as outputs
  for(i=0;i<DIGITS;i++) {
    pinMode(digitMap[i],OUTPUT);
  }
  pinMode(latchPin,OUTPUT);
  pinMode(clockPin,OUTPUT);
  pinMode(dataPin,OUTPUT);
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
  
  // Fetch the right segment mask for this particular numeric value
  mask = pgm_read_byte(SevenSegMap + n);
  
  // take the latchPin low so 
  // the segments don't change while you're sending in bits:
  digitalWrite(latchPin, LOW);
  
  // shift out the bits
  shiftOut(dataPin, clockPin, MSBFIRST, mask);  

  // take the latch pin high to latch the values into the shift register
  // output pins
  digitalWrite(latchPin, HIGH);
  
  digitalWrite(digitMap[d],DIGIT_ON);  // Enable digit
  delay(5);
}

