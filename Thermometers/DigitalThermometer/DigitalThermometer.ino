/*
 * DTempRecorder
 *
 * Copyright (c) 2012 David Bryant <djbryant@gmail.com>
 
 * Improved Temperature Recorder designed to use the Dallas Semiconductor DS18B20
 * digital sensor and associated library from Miles Burton
 * (http://www.milesburton.com/?title=Dallas_Temperature_Control_Library)
 *
 * Author: David Bryant (david@orangemoose.com)
 * Version 1.1, 17-August-2012
 *
 * Arduino Pinouts (for 4-wire parallel interface to LCD)
 *    
 *    6 = PWM control for red backlight LED brightness -> LCD16
 *    7 = LCD RS -> LCD4
 *    8 = LCD EN -> LCD6
 *    9 = LCD DB4 (data) -> LCD11
 *   10 = LCD DB5 (data) -> LCD12
 *   11 = LCD DB6 (data) -> LCD13
 *   12 = LCD DB7 (data) -> LCD14
 *   13 = Blink on-board LED to indicate an update
 *
 * Additional LCD connections (valid for either with or without a shift register)
 *    LCD1 = GND
 *    LCD2 = VCC
 *    LCD3 = Contrast voltage (wiper of contrast pot)
 *    LCD5 = GND
 *    LCD15 = VCC
 *
 * For possible use by upstream applications and remote monitoring this application
 * produces a single line of output on the Serial port for each reported data value,
 * formatted as follows:
 *    {Sample #},{Temperature F},{Avg Temp F}
 *
 * where:
 *    {Sample #} = sequential sample number displayed in HEX
 *    {Temperature F} = average Farenheit temperature across the calculation interval
 *                      as configured below, displayed as a floating point number
 *    {Avg Temp F} = average of reported temperatures across collection interval
 */
 
 
/*
 * Include the LCD library code. The standard Arduino 1.0 LCD library works fine but does not support
 * use of a shift register and so requires 6 control lines from the Arduino to the LCD.  That number
 * can be reduced to 3 control lines through use of a latching shift register but requires a different
 * LiquidCrystal library.  (This frees up 3 pins for other purposes.) The code here uses an
 * updated library from F.Malpartida et. al. at https://bitbucket.org/fmalpartida/new-liquidcrystal/wiki/Home
 * which also has the benefit of improved performance relative to the standard Arduino 1.0 library.
 *
 * If using a shift register include LiquidCrystal_SR.h.  If using 4-wire parallel include LiquidCrystal.h
 */
#include <Wire.h>
#include <LiquidCrystal.h>

// Dallas Semiconductor DS18B20 requires Dallas Temperature Control Library, which uses the OneWire network
#include <OneWire.h>
#include <DallasTemperature.h>

/*
 * If debugging, send extra output info via the Serial monitor though NOTE that
 * this will confuse any upstream processing of reported data, e.g. via Processing
 * applications on a remote host connected via Serial/USB.  With debugging off
 * we should only output one line per reported data value as described above.
 */
boolean debugging = false;

int sensorPin = 3;     /* Pin the DS18B20 sensor output is connected to */
int redLEDPin = 6;     /* Pin that controls red display on the LCD panel */
int brightness = 100;  /* LCD display brightness, 0 = max, 255 = off. 100 works well */
int LEDPin = 13;       /* Pin with LED to indicate update */

/*
 * Need to accumulate several pieces of data to calculate ongoing temperature
 *
 * In a moving vehicle good values are a sample delay of 2 seconds and a capture
 * delay (reporting inverval) of 2 minutes.  At a stationary location such as a home
 * weather station good values are a sample delay of 5 seconds and a capture delay
 * of 5 minutes.
 *
 */
int sampleDelay = 5;           /* Interval at which temperature sensor is read (in seconds) */
int captureDelay = 5;          /* Interval at which samples are averaged & reported (in minutes) */
unsigned long captureDelayMs;  /* Calculation interval as milliseconds (calculated) */
unsigned long prevMillis = 0;  /* Timestamp for measuring elapsed time */


float totalReadingF = 0; /* Total sensor reading over capture interval */
int numReadings = 0;     /* Number of sensor readings over capture interval */
float avgReadingF;       /* Average temperature F reading over current capture interval*/
int numCaptures = 0;     /* Number of capture intervals observed */
float tempMinutes = 0;   /* Clever means of calculating average temperature by keeping a running
                          * total of the area under the temperature curve above ongoing minimum temp.
                          */
float minTempF = 1000;   /* Observed minimum temperature */
float maxTempF = -1000;  /* Observed maximum temperature */
float avgTempF = 0;      /* Overall average across all readings (all capture intervals) */

/* 
 * Initialize the OneWire library, creating an instance to communicate with
 * any OneWire devices (not just the temperature sensor). Then initialize the
 * temperature sensor on that OneWire bus
 */
OneWire oneWire(sensorPin);
DallasTemperature sensors(&oneWire);
 
/* Initialize the LCD library with the numbers of the interface pins */
LiquidCrystal lcd(7, 8, 9, 10, 11, 12);    /* if using 4-bit parallel interface */
// LiquidCrystal_SR lcd(7, 8, 9);          /* if using 3 wire shift register interface */

#define DEG_CHAR  1
#define ARR_CHAR  2
#define DEGF_CHAR 3

/* Define custom degree & arrow symbols */
/* The tool at http://www.quinapalus.com/hd44780udg.html is very useful for this... */
byte degree[8] = {
  0b01100,    /* .##.. */
  0b10010,    /* #..#. */
  0b10010,    /* #..#. */
  0b01100,    /* .##.. */
  0b00000,    /* ..... */
  0b00000,    /* ..... */
  0b00000,    /* ..... */
  0b00000     /* ..... */
};

byte arrow[8] = {
  0b00000,    /* ..... */
  0b00100,    /* ..#.. */
  0b00010,    /* ...#. */
  0b11111,    /* ##### */
  0b00010,    /* ...#. */
  0b00100,    /* ..#.. */
  0b00000,    /* ..... */
  0b00000,    /* ..... */
};

byte degf[8] = {
  0b01000,    /* .#... */
  0b10100,    /* #.#.. */
  0b01011,    /* .#.## */
  0b00010,    /* ...#. */
  0b00011,    /* ...## */
  0b00010,    /* ...#. */
  0b00010,    /* ...#. */
  0b00000,    /* ..... */
};

void setup()
{
  // Output via the Serial monitor
  Serial.begin(9600); 
  
  // Start the DS18B20
  sensors.begin();
  
  // Enable output pin for LED
  pinMode(LEDPin,OUTPUT);
  
  // Create custom characters using LCD library
  lcd.createChar(DEG_CHAR,degree);
  lcd.createChar(ARR_CHAR,arrow);
  lcd.createChar(DEGF_CHAR,degf);
  
  // Initialize LCD and set display brightness
  lcd.begin(16,2);
  analogWrite(redLEDPin,brightness);
  lcd.clear();
  lcd.setCursor(0,0);
  lcd.print("--.-"); lcd.write(DEG_CHAR); lcd.print("F");
  lcd.setCursor(0,1);
  lcd.print("Sampling...");

  // Calculate capture delay parameters
  captureDelayMs = captureDelay * 60 * 1000L;   /* convert minutes to milliseconds */
  prevMillis = millis();
  
  // Output key operating parameters to help interpret overall data set
  Serial.print("Sample interval: "); Serial.print(sampleDelay); Serial.print(", ");
  Serial.print("Capture interval: "); Serial.print(captureDelay); Serial.println(" min. ");
}
 
void loop()
{
  float temperatureC, temperatureF;
  float currentTempC, currentTempF;
  
  // Issue a global temperature request to all devices on the bus
  sensors.requestTemperatures();
  
  // Now read temperature from sensor #1 (device #0)
  currentTempC = sensors.getTempCByIndex(0);
  
  // Convert to F but could use getTempFByIndex()...
  currentTempF = (currentTempC * 9.0 / 5.0) + 32.0;
  
  totalReadingF += currentTempF;
  numReadings ++;
  avgReadingF = totalReadingF / numReadings;

  lcd.setCursor(8,0);       // Move to area reserved for current sensor value
  lcd.print("(");
  lcdprint1f(currentTempF);     // lcd.print() but with 1 decimal digit
  lcd.write(DEG_CHAR);
  lcd.print(") ");
  
  if(debugging) {
    Serial.print("Device 1: ");
    Serial.print(currentTempC); Serial.print(" C, ");
    Serial.print(currentTempF); Serial.print(" F, (");
    Serial.print(avgReadingF); Serial.println(" avg)");
  }
  
  unsigned long currentMillis = millis();
  /* Check and see if it is time to compute the running average temperature */
  if( (currentMillis - prevMillis) >= captureDelayMs) {
    
    numCaptures++;  // First, increment capture count (since it started at 0)
    
    /* If the first interval then initialize max & min temp */
    if(numCaptures == 1) {
        maxTempF = avgReadingF;
        minTempF = avgReadingF;
    }
    else {
      // Update max, min, and "temperature-minutes"
      if(avgReadingF > maxTempF) { maxTempF = avgReadingF; } /* New maximum? */
      if(avgReadingF < minTempF) {  /* New Minimum? */
        // If so, need to adjust temperature-minutes factor, as it is based on running minimum
        tempMinutes = tempMinutes + ((numCaptures-1) * (minTempF - avgReadingF));
        // And save new low temperature 
        minTempF = avgReadingF;
      }
      else {
        // Not a new minimum temperature, so just add to the temp-minutes factor
        tempMinutes = tempMinutes + (avgReadingF - minTempF);
      }
    }
    // Recalculate average reported temperature
    avgTempF = minTempF + tempMinutes/numCaptures;
    
    // Report results by printing them out the Serial port...
    Serial.print(numCaptures); Serial.print(","); Serial.print(avgReadingF);
    Serial.print(","); Serial.println(avgTempF);
    
    // Update display
    lcd.clear();
    lcd.setCursor(0,0);       // Start at top left
    lcdprint1f(avgReadingF);  // lcd.print but only 1 decimal digit
    lcd.write(DEG_CHAR);      // Custom degree symbol
    lcd.print("F");
    
    lcd.setCursor(8,0);       // Move to area reserved for current sensor value
    lcd.print("(");
    lcdprint1f(avgTempF);     // lcd.print but only 1 decimal digit
    lcd.write(DEG_CHAR);
    lcd.print(")");
   
    lcd.setCursor(0,1);       // Move to bottom left (2nd row)
    lcdprint1f(minTempF);     // lcd.print but only 1 decimal digit
    lcd.write(ARR_CHAR);      // Custom arrow character
    lcdprint1f(maxTempF);     // lcd.print but only 1 decimal digit
    lcd.write(DEG_CHAR);      // Custom degree symbol
    
    // Diplay number of captures in hex, roll over if greater than 0x999
    if(numCaptures > 0x999) numCaptures = 0;
    if(numCaptures < 0x10)       { lcd.setCursor(15,1); lcd.print(numCaptures,HEX); }
    else if(numCaptures < 0x100) { lcd.setCursor(14,1); lcd.print(numCaptures,HEX); }
    else                         { lcd.setCursor(13,1); lcd.print(numCaptures,HEX); }
    
    /* Flash LED */
    digitalWrite(LEDPin,HIGH); delay(50); digitalWrite(LEDPin,LOW);
    
    // Reset counters and accumulators
    prevMillis = currentMillis;
    totalReadingF = 0;
    numReadings = 0;
  }
  
  /* Delay base sample interval (in milliseconds) */
  delay(1000*sampleDelay);
}

/*
 * lcd.print() displays floating point values with two decimal digits, which is
 * overkill for our small 16x2 LCD display.  This function results in the displayed
 * value just having one decimal digit.
 */
void lcdprint1f(float num)
{
  int whole, frac;
  
  whole = (int) num;
  frac = (int) ((num*10) + 0.5);
  frac = frac%10;
  lcd.print(whole); lcd.print("."); lcd.print(frac);
}
