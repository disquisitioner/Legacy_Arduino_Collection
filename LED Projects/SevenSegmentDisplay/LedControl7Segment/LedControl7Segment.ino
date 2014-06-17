//We always have to include the library
#include "LedControl.h"

/*
 Now we need a LedControl to work with.
 ***** These pin numbers will probably not work with your hardware *****
 pin 12 is connected to the DataIn 
 pin 11 is connected to the CLK 
 pin 10 is connected to LOAD 
 We have only a single MAX72XX.
 */
LedControl lc=LedControl(12,11,10,1);

/* we always wait a bit between updates of the display */
unsigned long delaytime=750;
unsigned long prevMillis, currentMillis;

/* Parameters key for displaying numbers (int & float) */
#define DIGITS 8         /* Number of digits we can display */
#define MAXNUM 99999999  /* Largest positive integer we can display */
#define MINNUM -9999999  /* Smallest negative integer we can display */
long numbase[DIGITS] = {10000000L,1000000L,100000L,10000L,1000L,100L,10L,1L};
byte numberToShow[DIGITS];  /* Used for more flexible formatting */
byte dpDigit = 0xFF;


void setup() {  
  /*
   The MAX72XX is in power-saving mode on startup,
   we have to do a wakeup call
   */
  lc.shutdown(0,false);
  /* Set the brightness to a medium values */
  lc.setIntensity(0,8);
  /* and clear the display */
  lc.clearDisplay(0);
  
  writeArduinoOn7Segment();
  delay(delaytime);
  
  displayFloat(-123.4);
  delay(5000);
  
  displayFloat(355.0/113.0);
  delay(5000);
  
  prevMillis = millis();
}


/*
 This method will display the characters for the
 word "Arduino" one after the other on digit 0. 
 */
void writeArduinoOn7Segment() {
  lc.clearDisplay(0);
  lc.setChar(0,7,'a',false);
  delay(delaytime);
  lc.setRow(0,6,0x05);
  delay(delaytime);
  lc.setChar(0,5,'d',false);
  delay(delaytime);
  lc.setRow(0,4,0x1c);
  delay(delaytime);
  lc.setRow(0,3,B00010000);
  delay(delaytime);
  lc.setRow(0,2,0x15);
  delay(delaytime);
  lc.setRow(0,1,0x1D);
  delay(delaytime);
  lc.clearDisplay(0);
  delay(delaytime);
} 

/*
  This method will scroll all the hexa-decimal
 numbers and letters on the display. You will need at least
 four 7-Segment digits. otherwise it won't really look that good.
 */
void scrollDigits() {
  for(int i=0;i<17;i++) {
    for(int j=0;j<DIGITS;j++){
      lc.setDigit(0,DIGITS-j-1,(i+j)%16,false);
    }
    /*  
    lc.setDigit(0,3,i,false);
    lc.setDigit(0,2,i+1,false);
    lc.setDigit(0,1,i+2,false);
    lc.setDigit(0,0,i+3,false);
    */
    delay(delaytime);
  }
  lc.clearDisplay(0);
  delay(delaytime);
}

long val = -10;
void loop() { 

  unsigned long currentMillis = millis();
  
  if( (currentMillis - prevMillis) >= 500) {
      displayInt(val);
      prevMillis = currentMillis;
      val++;
  }
  if(val > MAXNUM) {
      val = -12345;
  }
}

/*
 * Display an integral value (not necessary an 'int' data type, though,
 * as we might have more DIGITS to display than an Arduino 'int' can 
 * store = 2 bytes or -32,768 to +32,767.)
 */

void displayInt(long n)
{
  int i,d;
  boolean lzero = true;
  
  lc.clearDisplay(0);
  
  // Do the easy case :-)
  if(n == 0) {
    lc.setDigit(0,0,0,false);
    return;
  }
  
  // Handle undisplayable values
  if( (n > MAXNUM) || (n < MINNUM) ) {
    displayError();
    return;
  }
  
  i = 0;  // Start with first digit
  
  // Handle negative integral values, generate minus sign
  if(n<0) {
      n *= -1;
      lc.setChar(0,DIGITS-1,'-',false);
      i++;
  }

  for(;i<DIGITS;i++) {
    if(n >= numbase[i]) {
      d = (int) (n/numbase[i]);
      lzero = false;  // No longer need to supress leading zeros
      lc.setDigit(0,DIGITS-i-1,d,false);
      n = n % numbase[i];
    }
    else {
      if(lzero) lc.setChar(0,DIGITS-i-1,' ',false);
      else      lc.setDigit(0,DIGITS-i-1,0,false);
    }
  }
}

void displayFloat(float f)
{
  long n, b;
  int d, i, len;
  boolean lzero = true;
  
   lc.clearDisplay(0);

  // Handle undisplayable values
  n = (long) f;  // Non-decimal part
  if(n < 0 || n > MAXNUM) {
    displayError();
    return;
  }
  
  for(i=0;i<DIGITS;i++) numberToShow[i] = 0;
  
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
    for(b=numbase[i];b>=1;b/=10) {
      if(n >= b) {
        d = (int) (n/b);
        lzero = false;
        numberToShow[i] = d;
        n = n % b;
        i++;
      }
      else {
        if(!lzero) { numberToShow[i] = 0; i++; }
      }
    }
    dpDigit = i-1;
  }

  // Now handle however much of decimal part will fit
  n = (long) f;
  f = f - n;
  n = (long) ( f * numbase[i]);
  if(n == 0) {
    numberToShow[i] = 0;
  }
  else {
    for(b=numbase[i+1];i<DIGITS;i++,b/=10) {
      d = (int) (n/b);
      numberToShow[i] = d;
      n = n % b;
      if(n == 0) {i++; break;}
    }
  }
  len = i;
  for(i=0;i<len;i++) {
    if(dpDigit == i) lc.setDigit(0,len-i-1,numberToShow[i],true);
    else             lc.setDigit(0,len-i-1,numberToShow[i],false);
  }
}

void displayError()
{
  lc.clearDisplay(0);
  lc.setChar(0,DIGITS-1,'E',false);
  lc.setRow(0,DIGITS-2,0x05);
  lc.setRow(0,DIGITS-3,0x05);
  lc.setRow(0,DIGITS-4,0x1D);
  lc.setRow(0,DIGITS-5,0x05);
}
