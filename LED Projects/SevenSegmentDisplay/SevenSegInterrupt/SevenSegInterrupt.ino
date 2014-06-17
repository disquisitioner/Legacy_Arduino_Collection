// Demonstration of LED multiplexing with Arduino
// Author: Nick Gammon
// Date: 2 December 2013

// Put a suitable resistor in series with each segment LED (eg. 180 ohm)

const byte PATTERN_COUNT = 16;
const byte SEGMENTS = 7;
const byte DIGITS = 4;

const byte columnPins [SEGMENTS] = { 6, 5, 8, 3, 7, 4, 2 };  // a, b, c, d, e, f, g
const byte digitPins [DIGITS]    = { 12, 13, 11, 10 };    // DIG1, DIG2, DIG3, DIG4
const byte dpPin = 9;  // decimal point pin
#define COMMON_ANODE false    // make false for common cathode LEDs


#if COMMON_ANODE
  // For common ANODE:
  const byte SEGMENT_ON = LOW;
  const byte SEGMENT_OFF = HIGH;
  const byte DIGIT_ON = HIGH;
  const byte DIGIT_OFF = LOW;
#else
  // For common CATHODE:
  const byte SEGMENT_ON = HIGH;
  const byte SEGMENT_OFF = LOW;
  const byte DIGIT_ON = LOW;
  const byte DIGIT_OFF = HIGH;
#endif 

// extra segment patterns (you can add more)
const byte SHOW_HYPHEN = 0x0A;
const byte SHOW_E      = 0x0B;
const byte SHOW_H      = 0x0C;
const byte SHOW_L      = 0x0D;
const byte SHOW_P      = 0x0E;
const byte SHOW_BLANK  = 0x0F;

const PROGMEM byte digitSegments [PATTERN_COUNT]  =
  {
  0b1111110,  // 0  
  0b0110000,  // 1  
  0b1101101,  // 2  
  0b1111001,  // 3  
  0b0110011,  // 4  
  0b1011011,  // 5  
  0b1011111,  // 6  
  0b1110000,  // 7  
  0b1111111,  // 8  
  0b1111011,  // 9  
  0b0000001,  // 0x0A -> -  
  0b1001111,  // 0x0B -> E  
  0b0110111,  // 0x0C -> H  
  0b0001110,  // 0x0D -> L  
  0b1100111,  // 0x0E -> P  
  0b0000000,  // 0x0F -> blank 
  };

volatile byte numberToShow [DIGITS] = { SHOW_H, SHOW_E, SHOW_L, 0 };  // HELO
volatile byte dpDigit = 0xFF;  // Decimal point digit, if > DIGITS then no DP

// timer Interrupt Service Routine (ISR) to update the LEDs
ISR (TIMER2_COMPA_vect) 
  {
  static byte digit = 0;
  byte thisDigit = numberToShow [digit];
  
  // check for out of range, if so show a blank
  if (thisDigit >= PATTERN_COUNT)
    thisDigit = SHOW_BLANK;
    
  // turn off old digits
  for (byte i = 0; i < DIGITS; i++)
    digitalWrite (digitPins[i], DIGIT_OFF);
    
  // set segments
  for (byte j = 0; j < SEGMENTS; j++)
    digitalWrite (columnPins [j],   // which segment pin
                (pgm_read_byte (digitSegments + thisDigit) // get bit pattern 
                & bit (SEGMENTS - j - 1))     // see if set or not
                ? SEGMENT_ON : SEGMENT_OFF);  // set appropriately (HIGH or LOW)
    
  // set decimal point (if needed)
  if(digit == dpDigit) {
    digitalWrite(dpPin,SEGMENT_ON);
  }
  else {
    digitalWrite(dpPin,SEGMENT_OFF);
  }
  
  // activate this digit
  digitalWrite (digitPins [digit], DIGIT_ON);
    
  // wrap if necessary
  if (++digit >= DIGITS)
    digit = 0;
  }  // end of TIMER2_COMPA_vect


void setup() 
{
  
  for (byte i = 0; i < SEGMENTS; i++)
    pinMode(columnPins[i], OUTPUT);  // make all the segment pins outputs
    
  for (byte i = 0; i < DIGITS; i++)
    pinMode(digitPins[i], OUTPUT);   // make all the digit pins outputs
  
  pinMode(dpPin,OUTPUT); // Make decimal point pin output
  
  // set up to draw the display repeatedly
  
  // Stop timer 2
  TCCR2A = 0;
  TCCR2B = 0;

  // Timer 2 - gives us a constant interrupt to refresh the LED display
  TCCR2A = bit (WGM21) ;   // CTC mode
  OCR2A  = 63;            // count up to 64  (zero relative!!!!)
  // Timer 2 - interrupt on match at about 2 kHz
  TIMSK2 = bit (OCIE2A);   // enable Timer2 Interrupt
  // start Timer 2
  TCCR2B =  bit (CS20) | bit (CS22) ;  // prescaler of 128
   
  delay (1000);  // give time to read "HELO" on the display
} // end of setup
  

int val = 0;

void loop ()
{
  unsigned long elapsedSeconds = millis () / 1000;
  char buf [10];
  
  /*
  sprintf (buf, "%04d", val);
  
  for (byte i = 0; i < DIGITS; i++)
    numberToShow [i] = buf [i] & 0x0F;
  */
  
  // displayFloat(0.12345);
  
  displayInt(val);
  delay(1000); 
  val++;
} // end of loop

void displayInt(int n)
{
  // Handle undisplayable values
  if(n < 0 || n > 9999) {
    for(byte i = 0; i < DIGITS; i++) {
      numberToShow[i] = SHOW_HYPHEN;
    }
    return;
  }
  
  numberToShow[0] = (int) n/1000;
  n = n % 1000;
  numberToShow[1] = (int) n/100;
  n = n % 100;
  numberToShow[2] = (int) n/10;
  numberToShow[3] = (int) n%10;
  
  dpDigit = 0xFF;  // no decimal point for integer
}

void displayFloat(float f)
{
  int n, d, b, i;
  
  n = (int) f;  // Non-decimal part
  
  // Handle undisplayable values
  if(n < 0 || n > 9999) {
    for(byte i = 0; i < DIGITS; i++) {
      numberToShow[i] = SHOW_HYPHEN;
    }
    return;
  }
  
  // Display non-decimal part
  if(n == 0) {
    numberToShow[0] = 0;
    dpDigit = 0;
    i = 1;
  }
  else {
    i = 0;
    for(b=1000;b>=1;b/=10) {
      if(n > b) {
        numberToShow[i] = (int) n/b;
        n = n % b;
        i++;
      }
    }
    dpDigit = i-1;
  }

  // Now handle however much of decimal part will fit
  n = (int) f;
  f = f - n;
  n = (int) ( f * 1000);
  if(n == 0) {
    numberToShow[i] = 0;
  }
  else {
    for(b=100;i<DIGITS;i++,b/=10) {
      numberToShow[i] = (int) n/b;
      n = n % b;
    }
  }
}
