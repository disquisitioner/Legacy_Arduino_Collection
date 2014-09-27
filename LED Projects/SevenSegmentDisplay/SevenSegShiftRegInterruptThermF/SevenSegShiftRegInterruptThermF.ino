/*
 * Program for experimenting with seven-segment LED displays
 * Author: David Bryant (david@orangemoose.com)
 * Date: 23 March 2014
 *
 * The sketch implements a digital thermometer using a four digit seven-segment 
 * LED display.  Temperature is read from a DS18bB20 digital temperature sensor.
 * Display refresh is handled through a timer interrupt, and the output segments 
 * are handled using an 8-bit shift register.
 *
 * Configuring Arduino timers isn't difficult, but it requires some careful thought
 * about how often you want the timer to generate an interrupt, and paying close 
 * attention to some arcane configuration instructions to get the timer properly
 * programmed.  This sketch is based on an excellent examples and tutorials by Nick 
 * Gammon at http://www.gammon.com.au/forum/?id=11488 and http://www.gammon.com.au/forum/?id=12314
 * and via Instructables at
 * http://www.instructables.com/id/Arduino-Timer-Interrupts/step1/Prescalers-and-the-Compare-Match-Register/
 */
#include <OneWire.h>
#include <DallasTemperature.h>

/* The DS18B20 has three pins:
 *   Pin 1 = GND
 *   Pin 2 = DQ (Data)
 *   Pin 3 = VDD (5V, generally)
 *
 *  Need a 4.7K resistor between pins 2 & 3 (vampire power mode)
 */
int sensorPin = 3;   /* Pin the DS18B20 data lead (DQ) is connected to */
unsigned long sampleDelayMs = 10*1000L;    // Interval at which temperature sensor is read (in milliseconds)
unsigned long updateDelayMs = 1*60*1000L;  // How often to update the display (in milliseconds)
unsigned long prevSampleMs = 0;
unsigned long prevUpdateMs = 0;
int numReadings = 0;
float totalReadF = 0;

const byte PATTERN_COUNT = 16;
const byte SEGMENTS = 7;
const byte DIGITS = 4;

//Pin connected to STROBE of MIC5891 (pin 4)
int latchPin = 8;
//Pin connected to CLOCK of MIC5891 (pin 2)
int clockPin = 7;
////Pin connected to SERIAL DATA IN of MIC5891 (pin 3)
int dataPin = 6;

const byte digitPins [DIGITS]    = { 12, 13, 11, 10 };    // DIG1, DIG2, DIG3, DIG4

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
#endif hop2to@d


// extra segment patterns (you can add more)
const byte SHOW_HYPHEN = 0x0A;
const byte SHOW_E      = 0x0B;
const byte SHOW_H      = 0x0C;
const byte SHOW_L      = 0x0D;
const byte SHOW_P      = 0x0E;
const byte SHOW_BLANK  = 0x0F;

/*
 * Map digits and patterns to the seven segments in each LED. The shift
 * register gives us 8 bits to work with so we can also handle the decimal
 * point (which will be zero in map but we'll set it dynamically):
 *          +---+---+---+---+---+---+---+----+
 * segment: | a | b | c | d | e | f | g | dp |
 *          +---+---+---+---+---+---+---+----+
 *     bit:   8   7   6   5   4   3   2    1
 */ 
const PROGMEM byte digitSegments [PATTERN_COUNT]  =
  {
  0b11111100,  // 0  
  0b01100000,  // 1  
  0b11011010,  // 2  
  0b11110010,  // 3  
  0b01100110,  // 4  
  0b10110110,  // 5  
  0b10111110,  // 6  
  0b11100000,  // 7  
  0b11111110,  // 8  
  0b11110110,  // 9  
  0b00000010,  // 0x0A -> -  
  0b10011110,  // 0x0B -> E  
  0b01101110,  // 0x0C -> H  
  0b00011100,  // 0x0D -> L  
  0b11001110,  // 0x0E -> P  
  0b00000000,  // 0x0F -> blank 
  };

volatile byte numberToShow [DIGITS] = { SHOW_H, SHOW_E, SHOW_L, 0 };  // HELO
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
  
  // check for out of range, if so show a blank
  if (thisDigit >= PATTERN_COUNT)  thisDigit = SHOW_BLANK;
    
  // turn off old digits
  for (byte i = 0; i < DIGITS; i++)
    digitalWrite (digitPins[i], DIGIT_OFF);
    
  // Set segments by writing value via shift register
  // Take the latchPin low so 
  // the LEDs don't change while you're sending in bits:
  digitalWrite(latchPin, LOW);
  
  // Shift out the bits to turn on the segments, which we look up via the 
  // map.  Also set the decimal point bit if needed on this digit
  shiftOut(dataPin, clockPin, MSBFIRST, 
    (pgm_read_byte(digitSegments + thisDigit) | (digit == dpDigit)) );  
  
  //take the latch pin high so the LEDs will light up:
  digitalWrite(latchPin, HIGH);
  
  // activate this digit
  digitalWrite (digitPins [digit], DIGIT_ON);
    
  // wrap if necessary
  if (++digit >= DIGITS)
    digit = 0;
  }  // end of TIMER2_COMPA_vect

/* Create the OneWire bus & Dallas Temperature sensor (DS18B20) objects */
OneWire oneWire(sensorPin);
DallasTemperature sensors(&oneWire);

void setup() 
{
  Serial.begin(9600);  /* Initialize the serial port so we can do output */
  sensors.begin();     /* Start the DS18B20 on the OneWire bus */
  
  for (byte i = 0; i < DIGITS; i++) {
    pinMode(digitPins[i], OUTPUT);   // make all the digit pins outputs
  }
  pinMode(latchPin,OUTPUT);
  pinMode(clockPin,OUTPUT);
  pinMode(dataPin,OUTPUT);   
  
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
   
  prevSampleMs = prevUpdateMs = millis();  // Save curent clock
  Serial.print(sampleDelayMs); Serial.print(","); Serial.println(updateDelayMs);
} // end of setup
  

void loop ()
{
  float tempF;
  
  unsigned long currentMillis = millis();  // Check current timer value
  // Time to read the sensor?
  if( (currentMillis - prevSampleMs) > sampleDelayMs) {
    numReadings++;
    sensors.requestTemperatures();       /* Alert the temperature sensor(s) that we want info */
    tempF = sensors.getTempFByIndex(0);  /* Read temperature (presumes DS18B20 is device #0 on OneWire bus */
    Serial.println(tempF);
    totalReadF += tempF;
    prevSampleMs = currentMillis;
  }
  
  // Time to update the display?
  if( (currentMillis - prevUpdateMs) > updateDelayMs) {
    tempF = totalReadF / numReadings;  
    displayFloat(tempF);
    Serial.print("Avg: "); Serial.println(tempF);
    prevUpdateMs = currentMillis;
    numReadings = 0;
    totalReadF = 0;
  }

} // end of loop


/*
 * Display an integer
 */
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

/*
 * Take a floating point number and extract the best four digits to show, including the
 * decimal point.
 */
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
  
  // Display non-decimal part and in the process
  // figure out which digit needs to also have the
  // decimal point.
  if(n == 0) {
    numberToShow[0] = 0;
    dpDigit = 0;
    i = 1;
  }
  else {
    i = 0;
    if(n > 1000)         { numberToShow[i] = n/1000; n = n%1000; i++; }
    if(n > 100 || n==0)  { numberToShow[i] = n/100;  n = n%100;  i++; }
    if(n > 10  || n==0)  { numberToShow[i] = n/10;   n = n%10;   i++; }
    numberToShow[i] = n; i++;
    
    dpDigit = i-1;  // set decimal point
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
