/*
 * Program for experimenting with seven-segment LED displays
 * Author: David Bryant (david@orangemoose.com)
 * Date: 23 March 2014
 *
 * The sketch just displays a steadily-incrementing counter, though there are some
 * convenience routines to handle display of integer and floating-point numbers for whenever
 * I might want to use this approach to do something interesting (like create a digital
 * thermometer with a seven-segment display).
 *
 * Display refresh is handled through a timer interrupt, abd the output segments 
 * are handled using a BCD-to-seven-segment decoder driver.
 *
 * Configuring Arduino timers isn't difficult, but it requires some careful thought
 * about how often you want the timer to generate an interrupt, and paying close 
 * attention to some arcane configuration instructions to get the timer properly
 * programmed.  This sketch is based on an excellent examples and tutorials by Nick 
 * Gammon at http://www.gammon.com.au/forum/?id=11488 and http://www.gammon.com.au/forum/?id=12314
 * and via Instructables at
 * http://www.instructables.com/id/Arduino-Timer-Interrupts/step1/Prescalers-and-the-Compare-Match-Register/
 */

const byte PATTERN_COUNT = 10;
const byte BITS = 4;
const byte DIGITS = 4;

const byte bcdPins [BITS] = { 8, 7, 6, 5 };  // BCD control: D3, D2, D1, D0
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

volatile byte numberToShow [DIGITS] = { 0, 0, 0, 0 };  // Default
volatile byte dpDigit = 0xFF;  // Decimal point digit, if > DIGITS then no DP

/*
 * Timer Interrupt Service Routine (ISR) to update the LEDs. Specifically it
 * updates one digit in the display every time it is called, and then moves on
 * to the next digit on the next invocation.
 */
ISR (TIMER2_COMPA_vect) 
  {
  static byte digit = 0;
  byte thisDigit = numberToShow [digit];
  
  // check for out of range, if so show a zero
  if (thisDigit >= PATTERN_COUNT)
    thisDigit = 0;
    
  // turn off old digits
  for (byte i = 0; i < DIGITS; i++)
    digitalWrite (digitPins[i], DIGIT_OFF);
    
  // set segments
  for (byte j = 0; j < BITS; j++)
    digitalWrite (bcdPins[j],            // which bcd value pin
                (thisDigit & bit (j))     // see if set or not
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
  
  for (byte i = 0; i < BITS; i++)
    pinMode(bcdPins[i], OUTPUT);  // make all the BCD pins outputs
    
  for (byte i = 0; i < DIGITS; i++)
    pinMode(digitPins[i], OUTPUT);   // make all the digit pins outputs
  
  pinMode(dpPin,OUTPUT); // Make decimal point pin output
  
  // set up to draw the display repeatedly
  
  // Stop timer 2
  TCCR2A = 0;
  TCCR2B = 0;

  /*
   * Timer 2 - gives us a constant interrupt to refresh the LED display
   *
   * Overall we want to refresh the display every 2ms so for a 4 digit display
   * that means each digit gets updated every 0.5ms.  That calls for a timer interrupt
   * frequency of 2KHz. With a 16MHz system clock then we need to divide that by 8000 
   * so let's use 8192 (a good approximation that's a power of 2).  Timer 2 is an 8 bit 
   * timer so we can't just have it count to 8192 but we can have it count to 64 and 
   * then use a pre-scaler value of 128 to get to 8192 (as 64 * 128 = 8192).
   */
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
  
  // displayFloat(78.36);
  
  displayInt(val);
  delay(1000); 
  val++;
} // end of loop

void displayInt(int n)
{
  // Handle undisplayable values
  if(n < 0 || n > 9999) {
    for(byte i = 0; i < DIGITS; i++) {
      numberToShow[i] = 0;
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
      numberToShow[i] = 0;
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
